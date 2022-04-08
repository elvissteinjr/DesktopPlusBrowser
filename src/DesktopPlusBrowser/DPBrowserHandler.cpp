#include "DPBrowserHandler.h"

#include <sstream>
#include <string>
#include <algorithm>

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#include "D3DManager.h"
#include "DPBrowserAPIServer.h"
#include "Util.h"

static CefRefPtr<DPBrowserHandler> g_DPBrowserHandler = nullptr;

DPBrowserHandler::DPBrowserHandler()
{
    DCHECK(!g_DPBrowserHandler);
    g_DPBrowserHandler = this;
}

DPBrowserHandler::~DPBrowserHandler()
{
    g_DPBrowserHandler = nullptr;
}

CefRefPtr<DPBrowserHandler> DPBrowserHandler::Get()
{
    return g_DPBrowserHandler;
}

std::string DPBrowserHandler::GetDataURI(const std::string& data, const std::string& mime_type)
{
    return "data:" + mime_type + ";base64," + CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

DPBrowserData& DPBrowserHandler::GetBrowserData(unsigned int id)
{
    CEF_REQUIRE_UI_THREAD();

    if (id < m_BrowserList.size())
        return m_BrowserList[id];
    else
        return m_BrowserDataNull;
}

DPBrowserData& DPBrowserHandler::GetBrowserData(CefBrowser& browser)
{
    CEF_REQUIRE_UI_THREAD();

    const auto it = m_BrowserIDToDataMap.find(browser.GetIdentifier());

    return (it != m_BrowserIDToDataMap.end()) ? GetBrowserData(it->second) : m_BrowserDataNull;
}

DPBrowserData& DPBrowserHandler::FindBrowserDataForOverlay(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    //Find browser data containing this overlay
    for (auto& browser_data : m_BrowserList)
    {
        auto it = std::find_if(browser_data.Overlays.begin(), browser_data.Overlays.end(), [&](const auto& overlay_data){ return (overlay_data.OverlayHandle == overlay_handle); });

        if (it != browser_data.Overlays.end())
        {
            return browser_data;
        }
    }

    return m_BrowserDataNull;
}

DPBrowserOverlayData& DPBrowserHandler::FindBrowserOverlayData(vr::VROverlayHandle_t overlay_handle, DPBrowserData** out_parent_browser_data)
{
    CEF_REQUIRE_UI_THREAD();

    //Find browser data containing this overlay
    for (auto& browser_data : m_BrowserList)
    {
        auto it = std::find_if(browser_data.Overlays.begin(), browser_data.Overlays.end(), [&](const auto& overlay_data){ return (overlay_data.OverlayHandle == overlay_handle); });

        if (it != browser_data.Overlays.end())
        {
            //Also set parent browser data pointer if requested
            if (out_parent_browser_data != nullptr)
            {
                *out_parent_browser_data = &browser_data;
            }

            return *it;
        }
    }

    if (out_parent_browser_data != nullptr)
    {
        *out_parent_browser_data = &m_BrowserDataNull;
    }

    return m_BrowserOverlayDataNull;
}

void DPBrowserHandler::ForceRedraw(DPBrowserData& browser_data)
{
    CEF_REQUIRE_UI_THREAD();

    if (browser_data.Overlays.empty())
        return;

    //We sometimes need force a redraw, even on static pages
    //DPBrowser_SetResolution() already does the trickery required to do this, so we just use that without resolution change unless it's already resizing anyways
    browser_data.IsFullCopyScheduled = true;
    if (!browser_data.IsResizing)
    {
        DPBrowser_SetResolution(browser_data.Overlays[0].OverlayHandle, browser_data.ViewportSize.width, browser_data.ViewportSize.height);
    }
}

void DPBrowserHandler::TryApplyPendingResolution(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        //Reset the pending viewport and call SetResolution() which will post this task again for later if the browser is still resizing
        int width  = browser_data.ViewportSizePending.width;
        int height = browser_data.ViewportSizePending.height;
        browser_data.ViewportSizePending.Reset();

        DPBrowser_SetResolution(overlay_handle, width, height);
    }
}

void DPBrowserHandler::ApplyMaxFPS(CefBrowser& browser)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = GetBrowserData(browser);

    if (browser_data.BrowserPtr != nullptr)
    {
        //Find highest max fps of an active overlay and use that
        int highest_max_fps = (browser_data.IsResizing) ? 120 : 1;      //Use minimum 120 fps during resize to improve responisveness
        int overlay_max_fps = 1;

        for (const auto& overlay_data : browser_data.Overlays)
        {
            if (!overlay_data.IsHidden)
            {
                overlay_max_fps = (overlay_data.MaxFPS == -1) ? m_GlobalMaxFPS : overlay_data.MaxFPS;

                if (overlay_max_fps > highest_max_fps)
                {
                    highest_max_fps = overlay_max_fps;
                }
            }
        }

        browser_data.BrowserPtr->GetHost()->SetWindowlessFrameRate(highest_max_fps);
    }
}

void DPBrowserHandler::CheckStaleFPSValues()
{
    CEF_REQUIRE_UI_THREAD();

    ULONGLONG current_tick = ::GetTickCount64();
    bool is_any_browser_active = false;

    for (auto& browser_data : m_BrowserList)
    {
        if ( (!browser_data.Overlays.empty()) && (browser_data.FrameCount != -1) )
        {
            //It's been more than 2 seconds, set fps to 0
            if (browser_data.FrameCountStartTick + 2000 < current_tick)
            {
                browser_data.FrameCount = -1;   //Mark as stale
                DPBrowserAPIServer::Get().NotifyFPS(browser_data.Overlays[0].OverlayHandle, 0);   //We only need to notify the primary overlay
            }
            else   //It's not been 2 seconds since the last update, check back later
            {
                is_any_browser_active = true;
            }
        }
    }

    //If there are still browsers active, repost the task to keep checking
    if (is_any_browser_active)
    {
        CefPostDelayedTask(TID_UI, base::BindOnce(&DPBrowserHandler::CheckStaleFPSValues, this), 2100); //2100 ms to give some leeway with the execution
    }
    else
    {
        m_IsStaleFPSValueTaskPending = false;
    }
}

void DPBrowserHandler::CloseAllBrowsers(bool force_close)
{
    CEF_REQUIRE_UI_THREAD();

    for (auto it = m_BrowserList.begin(); it != m_BrowserList.end(); ++it)
    {
        it->BrowserPtr->GetHost()->CloseBrowser(force_close);
    }
}

void DPBrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
    CEF_REQUIRE_UI_THREAD();

    if (frame->IsMain())
    {
        DPBrowserData& data = GetBrowserData(*browser);

        //Don't notify on the same URL as last time
        //This is mainly to work around CEF calling OnAddressChange with the previous URL when loading a new URL for some reason
        if (url == data.LastNotifiedURL)
            return;

        const auto& overlay_data = data.Overlays;
        if (!overlay_data.empty())
        {
            DPBrowserAPIServer::Get().NotifyURLChange(overlay_data[0].OverlayHandle, url);  //Only need to notify the primary overlay
        }

        data.LastNotifiedURL = url;
    }
}

void DPBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    CEF_REQUIRE_UI_THREAD();

    const auto& overlay_data = GetBrowserData(*browser).Overlays;
    if (!overlay_data.empty())
    {
        DPBrowserAPIServer::Get().NotifyTitleChange(overlay_data[0].OverlayHandle, title);  //Only need to notify the primary overlay
    }
}

bool DPBrowserHandler::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, cef_cursor_type_t type, const CefCursorInfo& custom_cursor_info)
{
    if (type != CT_HAND)
        return false;

    //Trigger haptic vibration on the overlays using this browser
    const DPBrowserData& browser_data = GetBrowserData(*browser);
    if (!browser_data.Overlays.empty())
    {
        //Trigger directly when dashboard pointer is active as it's going to be the right device anyways
        if (vr::VROverlay()->GetPrimaryDashboardDevice() != vr::k_unTrackedDeviceIndexInvalid)
        {
            for (const auto& overlay_data : browser_data.Overlays)
            {
                //OpenVR will not like having haptics triggered on the wrong overlay, so we have to make that it's the hover target overlay
                if (vr::VROverlay()->IsHoverTargetOverlay(overlay_data.OverlayHandle))
                {
                    vr::VROverlay()->TriggerLaserMouseHapticVibration(overlay_data.OverlayHandle, 0.0f, 1.0f, 0.16f);
                    return false;
                }
            }
        }

        //Otherwise let dashboard app handle it as we'll need VR Input access
        DPBrowserAPIServer::Get().NotifyLaserPointerHaptics();
    }

    return false;
}

void DPBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    //Do nothing
}

bool DPBrowserHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    //Allow the close
    return false;
}

void DPBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    int erased_id = -1;

    //Remove browser data
    const auto it = m_BrowserIDToDataMap.find(browser->GetIdentifier());

    if ( (it != m_BrowserIDToDataMap.end()) && (it->second < m_BrowserList.size()) )
    {
        erased_id = it->second;
        m_BrowserList.erase(m_BrowserList.begin() + erased_id);
    }

    //Remove browser data id assignment
    m_BrowserIDToDataMap.erase(browser->GetIdentifier());

    //Fixup browser data id assignments for ids past the erased one
    if (erased_id != -1)
    {
        for (auto& pair : m_BrowserIDToDataMap)
        {
            if (pair.second > erased_id)
            {
                pair.second--;
            }
        }
    }
}

bool DPBrowserHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name, 
                                     cef_window_open_disposition_t target_disposition, bool user_gesture, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, 
                                     CefRefPtr<CefClient>& client, CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue>& extra_info, bool* no_javascript_access)
{
    if (user_gesture)
        frame->LoadURL(target_url);

    return true; //No popups
}

void DPBrowserHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
          "<h2>Failed to load URL " << std::string(failedUrl) << " with error " << std::string(errorText) << " (" << errorCode << ").</h2>"
          "</body></html>";

    std::string data_uri = GetDataURI(ss.str(), "text/html");

    //Avoid sending the rather confusing looking data URI to UI
    DPBrowserData& data = GetBrowserData(*browser);
    data.LastNotifiedURL = data_uri;

    frame->LoadURL(data_uri);
}

void DPBrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
    DPBrowserData& data = GetBrowserData(*browser);

    if (data.Overlays.empty())
        return;

    DPBrowserAPIServer::Get().NotifyNavigationState(data.Overlays[0].OverlayHandle, canGoBack, canGoForward, isLoading);    //We only need to notify the primary overlay
}

void DPBrowserHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    rect = GetBrowserData(*browser).ViewportSize;
}

void DPBrowserHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height)
{
    return; //Use OnAcceleratedPaint2()
}

void DPBrowserHandler::OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, void* shared_handle)
{
    return; //Use OnAcceleratedPaint2()
}

void DPBrowserHandler::OnAcceleratedPaint2(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, void* shared_handle, bool new_texture)
{
    if (type != PET_VIEW)
        return;

    DPBrowserData& data = GetBrowserData(*browser);

    if (data.Overlays.empty())
        return;

    //Use shared handle here
    if (new_texture)
    {
        data.SharedTexture.Reset();
        data.StagingTexture.Reset();

        HRESULT res = D3DManager::Get().GetDevice()->OpenSharedResource1(shared_handle, __uuidof(ID3D11Texture2D), (void**)&data.SharedTexture);

        if (FAILED(res))
            return;

        //Set resize state if it's not already set for reason
        if (!data.IsResizing)
        {
            data.IsResizing = true;
            data.ResizingFrameCount = 0;
        }
    }
    else if (data.SharedTexture == nullptr) //This shouldn't usually happen, but abort if there's neither a new texture nor an existing shared one
    {
        return;
    }

    //Default to partial copy if the texture isn't new (usually from resize)
    bool do_fully_copy = (data.IsFullCopyScheduled || data.IsResizing);

    //To avoid flicker after a resize, the first few frames are not actually sent to OpenVR
    //However, we need to make sure there are going to be follow up frames so the overlay doesn't look stuck until the next user interaction
    if (data.IsResizing)
    {
        if (data.ResizingFrameCount > 2)
        {
            data.IsResizing = false;
            data.BrowserPtr->GetHost()->SetZoomLevel(data.ZoomLevel);

            //Reset previously enforced minimum FPS during resize
            ApplyMaxFPS(*data.BrowserPtr);
        }
        else //Do the zoom hack to have Chromium render more frames even if the site is static
        {
            data.BrowserPtr->GetHost()->SetZoomLevel(data.ZoomLevel + (data.ResizingFrameCount * 0.001) );
        }

        data.ResizingFrameCount++;
    }

    //Update all overlays using this browser
    size_t ou_count = 0;
    vr::VROverlayHandle_t ovrl_shared_source = vr::k_ulOverlayHandleInvalid;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_tex_shared_source;
    for (const auto& overlay : data.Overlays)
    {
        if (overlay.IsOverUnder3D)
        {
            //TODO: OU3D support
            ou_count++;
        }
        else if (ovrl_shared_source == vr::k_ulOverlayHandleInvalid) //For the first non-OU3D overlay, set the texture as normal
        {
            ovrl_shared_source = overlay.OverlayHandle;

            //Get overlay texture from OpenVR and copy dirty rect directly into it if possible
            if (!do_fully_copy)
            {
                ID3D11ShaderResourceView* ovrl_shader_res = nullptr;
                uint32_t ovrl_width;
                uint32_t ovrl_height;
                uint32_t ovrl_native_format;
                vr::ETextureType ovrl_api_type;
                vr::EColorSpace ovrl_color_space;
                vr::VRTextureBounds_t ovrl_tex_bounds;

                vr::VROverlayError ovrl_error = vr::VROverlay()->GetOverlayTexture(ovrl_shared_source, (void**)&ovrl_shader_res, data.SharedTexture.Get(), &ovrl_width, &ovrl_height, &ovrl_native_format, 
                                                                                   &ovrl_api_type, &ovrl_color_space, &ovrl_tex_bounds);

                if (ovrl_error == vr::VROverlayError_None)
                {
                    D3D11_TEXTURE2D_DESC desc;
                    data.SharedTexture->GetDesc(&desc);

                    if ( (desc.Width == ovrl_width) && (desc.Height == ovrl_height) )
                    {
                        ID3D11Resource* ovrl_tex;
                        ovrl_shader_res->GetResource(&ovrl_tex);

                        D3D11_BOX box = {0};
                        box.left   = 0;
                        box.top    = 0;
                        box.front  = 0;
                        box.right  = ovrl_width;
                        box.bottom = ovrl_height;
                        box.back   = 1;

                        D3DManager::Get().GetDeviceContext()->CopySubresourceRegion(ovrl_tex, 0, box.left, box.top, 0, (ID3D11Texture2D*)data.SharedTexture.Get(), 0, &box);
                        D3DManager::Get().GetDeviceContext()->Flush();

                        ovrl_tex->Release();
                        ovrl_tex = nullptr;
                    }
                    else //Texture sizes don't match for some reason, fall back to fully copy
                    {
                        do_fully_copy = true;
                    }

                    //Release shader resource
                    vr::VROverlay()->ReleaseNativeOverlayHandle(ovrl_shared_source, (void*)ovrl_shader_res);
                    ovrl_shader_res = nullptr;
                }
                else //Usually shouldn't fail, but fall back to full copy then
                {
                    do_fully_copy = true;
                }
            }
    
            //Do a fully copy if needed, utilizing an additional staging texture
            if (do_fully_copy)
            {
                //If we don't have a staging texture yet/anymore, create it
                if (data.StagingTexture == nullptr)
                {
                    D3D11_TEXTURE2D_DESC desc;
                    data.SharedTexture->GetDesc(&desc);

                    data.StagingTexture = D3DManager::Get().CreateOverlayTexture(desc.Width, desc.Height);

                    //Bail if texture creation failed
                    if (data.StagingTexture == nullptr)
                    {
                        return;
                    }
                }

                //Don't set the texture while resizing to avoid flickering
                if (!data.IsResizing)
                {
                    //Set the overlay texture from the staging texture
                    vr::Texture_t vr_tex = {0};
                    vr_tex.eColorSpace = vr::ColorSpace_Gamma;
                    vr_tex.eType       = vr::TextureType_DirectX;
                    vr_tex.handle      = data.StagingTexture.Get();

                    vr::VROverlay()->SetOverlayTexture(ovrl_shared_source, &vr_tex);

                    //Use this staging texture as a device reference for the shared overlays
                    staging_tex_shared_source = data.StagingTexture;
                }

                //Note how we copy the texture after setting the texture. This is because the first frame on a new texture will be blank and we want to avoid flickering
                D3DManager::Get().GetDeviceContext()->CopyResource(data.StagingTexture.Get(), data.SharedTexture.Get());
                D3DManager::Get().GetDeviceContext()->Flush();   //Copy will not be done in time without flushing

                data.IsFullCopyScheduled = false;
            }
        }
        else if ( (do_fully_copy) && (!data.IsResizing) ) //For all others, set it shared from the normal overlay if an update is needed
        {
            SetSharedOverlayTexture(ovrl_shared_source, overlay.OverlayHandle, staging_tex_shared_source.Get());
        }
    }

    //Frame counter
    data.FrameCount += (data.FrameCount == -1) ? 2 : 1;     //Add 2 frames if count is set to -1 (previously marked as stale)

    if (::GetTickCount64() >= data.FrameCountStartTick + 1000)
    {
        //A second has passed, send fps messages and reset the value
        DPBrowserAPIServer::Get().NotifyFPS(data.Overlays[0].OverlayHandle, data.FrameCount);   //We only need to notify the primary overlay

        data.FrameCountStartTick = ::GetTickCount64();
        data.FrameCount = 0;
    }

    //If the task isn't already pending, post it to check for stale FPS values in case CEF stops rendering frames
    if (!m_IsStaleFPSValueTaskPending)
    {
        CefPostDelayedTask(TID_UI, base::BindOnce(&DPBrowserHandler::CheckStaleFPSValues, this), 2100);
        m_IsStaleFPSValueTaskPending = true;
    }
}

void DPBrowserHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
{
    //Clear the menu to block it
    model->Clear();
}

void DPBrowserHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
    //Do nothing
}

void DPBrowserHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback)
{
    CEF_REQUIRE_UI_THREAD();

    //We don't allow any downloads
    // 
    //CEF documentation suggests that downloads would be blocked by default if the callback in OnBeforeDownload() isn't used, but that simply isn't true.
    //In the default case, the download happens quietly and is put in the temp folder... and never deleted. Not nice.
    callback->Cancel();
}

void DPBrowserHandler::DPBrowser_StartBrowser(vr::VROverlayHandle_t overlay_handle, const std::string& url, bool use_transparent_background)
{
    CEF_REQUIRE_UI_THREAD();

    //Default window settings
    CefWindowInfo window_info;
    window_info.SetAsWindowless(nullptr);
    window_info.shared_texture_enabled = true;

    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = m_GlobalMaxFPS;
    browser_settings.background_color = (use_transparent_background) ? 0x0 : 0xFFFFFFFF;

    DPBrowserData browser_data;

    //Create browser synchronously and store extra data for the browser
    browser_data.BrowserPtr = CefBrowserHost::CreateBrowserSync(window_info, Get(), url, browser_settings, nullptr, nullptr);
    browser_data.ViewportSize.Set(0, 0, 1280, 720);

    //Assign browser id to browser data id
    int browser_data_id = (int)m_BrowserList.size();
    m_BrowserIDToDataMap[browser_data.BrowserPtr->GetIdentifier()] = browser_data_id;

    //Add overlay data
    DPBrowserOverlayData overlay_data;
    overlay_data.OverlayHandle = overlay_handle;
    browser_data.Overlays.push_back(overlay_data);

    //Add to browser list
    m_BrowserList.push_back(browser_data);
}

void DPBrowserHandler::DPBrowser_DuplicateBrowserOutput(vr::VROverlayHandle_t overlay_handle_src, vr::VROverlayHandle_t overlay_handle_dst)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle_src);

    if (browser_data.BrowserPtr != nullptr)
    {
        DPBrowserOverlayData overlay_data;
        overlay_data.OverlayHandle = overlay_handle_dst;
        browser_data.Overlays.push_back(overlay_data);

        //To set the shared texture, we need a fresh full texture copy. This also needs to trigger a redraw even on static pages
        ForceRedraw(browser_data);
    }
}

void DPBrowserHandler::DPBrowser_PauseBrowser(vr::VROverlayHandle_t overlay_handle, bool pause)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr == nullptr)
        return;

    //Check if all overlays are hidden and apply overall state to browser
    bool all_hidden = true;
    for (DPBrowserOverlayData& overlay_data : browser_data.Overlays)
    {
        if (overlay_data.OverlayHandle == overlay_handle)
        {
            //No change, back out
            if (overlay_data.IsHidden == pause)
            {
                return;
            }

            overlay_data.IsHidden = pause;
        }

        if (!overlay_data.IsHidden)
        {
            all_hidden = false;
        }
    }

    browser_data.BrowserPtr->GetHost()->WasHidden(all_hidden);
    browser_data.BrowserPtr->GetHost()->SetAudioMuted(all_hidden);

    //Apply max fps again in case this changed anything
    ApplyMaxFPS(*browser_data.BrowserPtr);
}

void DPBrowserHandler::DPBrowser_RecreateBrowser(vr::VROverlayHandle_t overlay_handle, bool use_transparent_background)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr == nullptr)
        return;

    //Store browser data id, remove old browser id assignment and close the old browser
    const auto it = m_BrowserIDToDataMap.find(browser_data.BrowserPtr->GetIdentifier());

    if (it == m_BrowserIDToDataMap.end())
        return;

    int browser_data_id = it->second;

    m_BrowserIDToDataMap.erase(it);
    CefString current_url = browser_data.BrowserPtr->GetMainFrame()->GetURL();
    browser_data.BrowserPtr->GetHost()->CloseBrowser(true);

    //Default window settings
    CefWindowInfo window_info;
    window_info.SetAsWindowless(nullptr);
    window_info.shared_texture_enabled = true;

    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = m_GlobalMaxFPS;
    browser_settings.background_color = (use_transparent_background) ? 0x0 : 0xFFFFFFFF;

    //Create new browser synchronously
    browser_data.BrowserPtr = CefBrowserHost::CreateBrowserSync(window_info, Get(), current_url, browser_settings, nullptr, nullptr);

    //Add browser id assignment for new browser
    m_BrowserIDToDataMap[browser_data.BrowserPtr->GetIdentifier()] = browser_data_id;

    //The CreateBrowserSync() call chain will have called GetViewRect() before we attached the browser data. Notify a resize just in case
    browser_data.BrowserPtr->GetHost()->WasResized();

    //Apply FPS to the new browser
    ApplyMaxFPS(*browser_data.BrowserPtr);
}

void DPBrowserHandler::DPBrowser_StopBrowser(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    //If this is the only assigned overlay, get rid of the browser
    if (browser_data.Overlays.size() == 1)
    {
        browser_data.BrowserPtr->GetHost()->CloseBrowser(true);
        //OnBeforeClose() takes care of removing the browser data from the list
    }
    else //If more than one overlay assigned, just remove this one
    {
        auto it = std::find_if(browser_data.Overlays.begin(), browser_data.Overlays.end(), [&](const auto& overlay_data){ return (overlay_data.OverlayHandle == overlay_handle); });

        if (it != browser_data.Overlays.end())
        {
            browser_data.Overlays.erase(it);

            //The removed overlay might've been used as a shared source, so we need to force a fresh full frame
            ForceRedraw(browser_data);
        }
    }
}

void DPBrowserHandler::DPBrowser_SetURL(vr::VROverlayHandle_t overlay_handle, const std::string& url)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.BrowserPtr->GetMainFrame()->LoadURL(url);

        //Hack: If the resolved URL doesn't change from this, the texture gets stuck and will not update anymore. Input doesn't need to match for this as long as it resolves to the same page.
        //Frames do get generated, but they're blank. We force generation of a new texture by changing the resolution briefly.
        if (browser_data.Overlays.empty())
        {
            DPBrowser_SetResolution(browser_data.Overlays[0].OverlayHandle, browser_data.ViewportSize.width, browser_data.ViewportSize.height + 1);
            DPBrowser_SetResolution(browser_data.Overlays[0].OverlayHandle, browser_data.ViewportSize.width, browser_data.ViewportSize.height - 1);
        }
    }
}

void DPBrowserHandler::DPBrowser_SetResolution(vr::VROverlayHandle_t overlay_handle, int width, int height)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        //If this browser is already resizing, set the pending size for the next opportunity
        if (browser_data.IsResizing)
        {
            //Try again later if the next attempt isn't queued yet
            if (browser_data.ViewportSizePending.IsEmpty())
            {
                CefPostDelayedTask(TID_UI, base::BindOnce(&DPBrowserHandler::TryApplyPendingResolution, this, overlay_handle), 100);
            }

            browser_data.ViewportSizePending.width  = std::max(1, width);
            browser_data.ViewportSizePending.height = std::max(1, height);

            //Some sites don't seem to trigger new frames from the initial zoom change, so keep decreasing zoom level
            browser_data.BrowserPtr->GetHost()->SetZoomLevel(browser_data.BrowserPtr->GetHost()->GetZoomLevel() - 0.001);

            return;
        }

        browser_data.ViewportSize.width  = std::max(1, width);
        browser_data.ViewportSize.height = std::max(1, height);

        browser_data.IsResizing = true;
        browser_data.ResizingFrameCount = 0;
        browser_data.BrowserPtr->GetHost()->WasResized();

        //Hack: We need to get Chromium to always generate another frame, and the obvious choices don't seem to do it here
        //We instead change the zoom a bit and restore it later. These frames are not sent to OpenVR anyways, so there's no visual effect.
        browser_data.BrowserPtr->GetHost()->SetZoomLevel(browser_data.ZoomLevel - 0.001);

        //Update FPS. ApplyMaxFPS() enforces a higher frame rate during resize to stay more responsive
        ApplyMaxFPS(*browser_data.BrowserPtr);
    }
}

void DPBrowserHandler::DPBrowser_SetFPS(vr::VROverlayHandle_t overlay_handle, int fps)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData* browser_data = nullptr;
    DPBrowserOverlayData& overlay_data = FindBrowserOverlayData(overlay_handle, &browser_data);

    if (browser_data != nullptr)
    {
        overlay_data.MaxFPS = (fps == -1) ? -1 : std::max(1, fps);

        if (browser_data->BrowserPtr != nullptr)
        {
            ApplyMaxFPS(*browser_data->BrowserPtr);
        }
    }
}

void DPBrowserHandler::DPBrowser_SetZoomLevel(vr::VROverlayHandle_t overlay_handle, float zoom_level)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.ZoomLevel = zoom_level;
        browser_data.BrowserPtr->GetHost()->SetZoomLevel(zoom_level);
    }
}

void DPBrowserHandler::DPBrowser_MouseMove(vr::VROverlayHandle_t overlay_handle, int x, int y)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        m_MouseState.x = x;
        m_MouseState.y = y;

        browser_data.BrowserPtr->GetHost()->SendMouseMoveEvent(m_MouseState, false);
    }
}

void DPBrowserHandler::DPBrowser_MouseLeave(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.BrowserPtr->GetHost()->SendMouseMoveEvent(m_MouseState, true);
    }
}

void DPBrowserHandler::DPBrowser_MouseDown(vr::VROverlayHandle_t overlay_handle, vr::EVRMouseButton button)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        CefBrowserHost::MouseButtonType cefbutton = MBT_LEFT;

        switch (button)
        {
            case vr::VRMouseButton_Left:   cefbutton = MBT_LEFT;   m_MouseState.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;   break;
            case vr::VRMouseButton_Right:  cefbutton = MBT_RIGHT;  m_MouseState.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;  break;
            case vr::VRMouseButton_Middle: cefbutton = MBT_MIDDLE; m_MouseState.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON; break;
        }

        browser_data.BrowserPtr->GetHost()->SendMouseClickEvent(m_MouseState, cefbutton, false, 1);
    }
}

void DPBrowserHandler::DPBrowser_MouseUp(vr::VROverlayHandle_t overlay_handle, vr::EVRMouseButton button)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        CefBrowserHost::MouseButtonType cefbutton = MBT_LEFT;

        switch (button)
        {
            case vr::VRMouseButton_Left:   cefbutton = MBT_LEFT;   m_MouseState.modifiers &= ~EVENTFLAG_LEFT_MOUSE_BUTTON;   break;
            case vr::VRMouseButton_Right:  cefbutton = MBT_RIGHT;  m_MouseState.modifiers &= ~EVENTFLAG_RIGHT_MOUSE_BUTTON;  break;
            case vr::VRMouseButton_Middle: cefbutton = MBT_MIDDLE; m_MouseState.modifiers &= ~EVENTFLAG_MIDDLE_MOUSE_BUTTON; break;
        }

        browser_data.BrowserPtr->GetHost()->SendMouseClickEvent(m_MouseState, cefbutton, true, 1);
    }
}

void DPBrowserHandler::DPBrowser_Scroll(vr::VROverlayHandle_t overlay_handle, float x_delta, float y_delta)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.BrowserPtr->GetHost()->SendMouseWheelEvent(m_MouseState, x_delta * WHEEL_DELTA, y_delta * WHEEL_DELTA);
    }
}

void DPBrowserHandler::DPBrowser_GoBack(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.BrowserPtr->GoBack();
    }
}

void DPBrowserHandler::DPBrowser_GoForward(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.BrowserPtr->GoForward();
    }
}

void DPBrowserHandler::DPBrowser_Refresh(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        (browser_data.BrowserPtr->IsLoading()) ? browser_data.BrowserPtr->StopLoad() : browser_data.BrowserPtr->Reload();
    }
}

void DPBrowserHandler::DPBrowser_GlobalSetFPS(int fps)
{
    CEF_REQUIRE_UI_THREAD();

    m_GlobalMaxFPS = std::max(1, fps);

    //Apply fps for every existing browser in case their fps set to auto (-1)
    for (auto& browser_data : m_BrowserList)
    {
        if (browser_data.BrowserPtr != nullptr)
        {
            ApplyMaxFPS(*browser_data.BrowserPtr);
        }
    }
}
