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
#include "OpenVRExt.h"

enum KeyboardWin32KeystateFlags
{
    kbd_w32keystate_flag_shift_down       = 1 << 0,
    kbd_w32keystate_flag_ctrl_down        = 1 << 1,
    kbd_w32keystate_flag_alt_down         = 1 << 2
};

static CefRefPtr<DPBrowserHandler> g_DPBrowserHandler = nullptr;

DPBrowserHandler::DPBrowserHandler()
{
    DCHECK(!g_DPBrowserHandler);
    g_DPBrowserHandler = this;

    m_ContentBlocker = CefRefPtr<DPBrowserContentBlocker>(new DPBrowserContentBlocker);
    CefPostTask(TID_IO, base::BindOnce(&DPBrowserContentBlocker::ReloadEngines, m_ContentBlocker));
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
    //Scheduling an idle frame update is usually good enough. For real resizes, there's also some zoom level trickery in place. See DPBrowser_SetResolution() for that
    browser_data.IsFullCopyScheduled = true;
    browser_data.IsIdleFrameUpdateScheduled = true;
    CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::ScheduledIdleFrameUpdate, this, browser_data.BrowserPtr));
}

void DPBrowserHandler::ScheduledIdleFrameUpdate(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& data = GetBrowserData(*browser);

    if (data.IsIdleFrameUpdateScheduled)
    {
        data.IsIdleFrameUpdateScheduled = false;

        CefRenderHandler::RectList dirty_rects;
        dirty_rects.push_back(data.ViewportSize);

        m_IsPaintCallForIdleFrame = true;
        OnAcceleratedPaint2(browser, PET_VIEW, dirty_rects, nullptr, false);    //PET_VIEW is given as paint type, but it doesn't matter with new_texture being set to false
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

void DPBrowserHandler::OnAcceleratedPaint2UpdateStagingTexture(DPBrowserData& data)
{
    D3DManager::Get().GetDeviceContext()->CopyResource(data.StagingTexture.Get(), data.SharedTexture.Get());

    //Add popup widget texture on of staging texture if it's visible
    //CEF samples go the full-on compositing route for this, but there doesn't seem to any widget that actually requires this, so we make do with a simple copy
    if ((data.IsPopupWidgetVisible) && (data.PopupWidgetTexture != nullptr))
    {
        D3D11_TEXTURE2D_DESC texture_desc = {0};
        data.SharedTexture->GetDesc(&texture_desc);

        //Textures are vertically flipped still, so we need to account for that when clipping the copy regions
        UINT dst_x = clamp(data.PopupWidgetRect.x, 0, (int)texture_desc.Width);
        int idst_y = -(data.PopupWidgetRect.y + data.PopupWidgetRect.height) + data.ViewportSize.height;
        UINT dst_y = clamp(idst_y, 0, (int)texture_desc.Height);

        D3D11_BOX box = {0};
        box.left   = (data.PopupWidgetRect.x < 0) ? -data.PopupWidgetRect.x : 0;
        box.top    = (idst_y < 0)                 ? -idst_y                 : 0;
        box.front  = 0;
        box.right  = std::min((UINT)data.PopupWidgetRect.width,  texture_desc.Width  -  dst_x);
        box.bottom = std::min((UINT)data.PopupWidgetRect.height, texture_desc.Height - idst_y);
        box.back   = 1;

        D3DManager::Get().GetDeviceContext()->CopySubresourceRegion((ID3D11Texture2D*)data.StagingTexture.Get(), 0, dst_x, dst_y, 0, 
                                                                    (ID3D11Texture2D*)data.PopupWidgetTexture.Get(), 0, &box);
    }

    //Copy will not be done in time without flushing
    D3DManager::Get().GetDeviceContext()->Flush();
}

CefKeyEvent DPBrowserHandler::KeyboardGenerateWCharEvent(wchar_t wchar)
{
    CefKeyEvent key_event;
    UINT scan_code = ::MapVirtualKey(LOBYTE(::VkKeyScanW(wchar)), MAPVK_VK_TO_VSC);
    key_event.native_key_code  = MAKELPARAM(1 /*repeat count*/, scan_code); //lParam like a WM_CHAR message
    key_event.windows_key_code = wchar;
    key_event.type             = KEYEVENT_CHAR;

    return key_event;
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

        //Release shared overlay textures of all remaining overlays, though it should only be one left in this situation usually
        auto it_data = m_BrowserList.begin() + erased_id;
        for (auto& overlay : it_data->Overlays)
        {
            vr::VROverlayEx()->ReleaseSharedOverlayTexture(overlay.OverlayHandle);
        }
        
        m_BrowserList.erase(it_data);
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

void DPBrowserHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& data = GetBrowserData(*browser);
    data.IsPopupWidgetVisible = show;

    if (show)
    {
        //Set resizing state to force a few frames to not get submitted to the overlay, avoiding flicker from new initially blank textures
        data.IsResizing = true;
        data.ResizingFrameCount = 0;

        ApplyMaxFPS(*data.BrowserPtr);
    }
    else
    {
        data.PopupWidgetTexture.Reset();
    }
}

void DPBrowserHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    CEF_REQUIRE_UI_THREAD();

    GetBrowserData(*browser).PopupWidgetRect = rect;
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
    //This function does a bit of trickery to avoid texture flicker. It seems to work, but feels like there could be a better way to work around it
    DPBrowserData& data = GetBrowserData(*browser);

    if (data.Overlays.empty())
        return;

    //Use shared handle here
    if (new_texture)
    {
        if (type == PET_VIEW)
        {
            data.SharedTexture.Reset();
            data.StagingTexture.Reset();
            data.PopupWidgetTexture.Reset();

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
        else if ((type == PET_POPUP) && (data.IsPopupWidgetVisible))
        {
            data.PopupWidgetTexture.Reset();

            HRESULT res = D3DManager::Get().GetDevice()->OpenSharedResource1(shared_handle, __uuidof(ID3D11Texture2D), (void**)&data.PopupWidgetTexture);

            if (FAILED(res))
                return;
        }
        else
        {
            return;
        }
    }
    else if (data.SharedTexture == nullptr) //This shouldn't usually happen, but abort if there's neither a new texture nor an existing shared one
    {
        return;
    }

    //Default to partial copy unless the texture is new (usually from resize) or popup widget is visible
    bool do_full_copy = (data.IsFullCopyScheduled || data.IsResizing || data.IsPopupWidgetVisible);

    //To avoid flicker after a resize, the first few frames are not actually sent to OpenVR
    if (data.IsResizing)
    {
        if (data.ResizingFrameCount > 2)
        {
            data.IsResizing = false;

            //If resizing state was set by an actual resize, then the zoom level will be modified and needs to be reset. 
            //If not, then it's a no-op in respect to popups closing from zoom changes and such
            data.BrowserPtr->GetHost()->SetZoomLevel(data.ZoomLevel);

            //Reset previously enforced minimum FPS during resize
            ApplyMaxFPS(*data.BrowserPtr);
        }

        data.ResizingFrameCount++;
    }

    //If we don't have a staging texture yet/anymore, create it
    D3D11_TEXTURE2D_DESC texture_desc = {0};
    if (data.StagingTexture == nullptr)
    {
        if (texture_desc.Width == 0)
        {
            data.SharedTexture->GetDesc(&texture_desc);
        }

        data.StagingTexture = D3DManager::Get().CreateOverlayTexture(texture_desc.Width, texture_desc.Height);

        //Bail if texture creation failed
        if (data.StagingTexture == nullptr)
        {
            return;
        }
    }

    //Update all overlays using this browser
    vr::VROverlayHandle_t ovrl_shared_source = vr::k_ulOverlayHandleInvalid;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_tex_shared_source;
    for (auto& overlay : data.Overlays)
    {
        if (overlay.IsOverUnder3D)
        {
            if (texture_desc.Width == 0)
            {
                data.SharedTexture->GetDesc(&texture_desc);
            }

            //Don't set the texture while resizing to avoid flickering
            //As with the full copy code, doing this before the actual copy to avoid flicker after resizing
            if (!data.IsResizing)
            {
                vr::Texture_t vrtex_ou = {0};
                vrtex_ou.eType = vr::TextureType_DirectX;
                vrtex_ou.eColorSpace = vr::ColorSpace_Gamma;
                vrtex_ou.handle = overlay.OU3D_Converter.GetTexture();

                if (vrtex_ou.handle != nullptr)
                {
                    vr::VROverlay()->SetOverlayTexture(overlay.OverlayHandle, &vrtex_ou);
                }
            }

            //Use staging texture while popup widget is copied on top of it (guaranteed to be updated) but use the shared texture otherwise (partial updates don't write to staging)
            //Also use staging texture during resizing to avoid flickering
            ID3D11Texture2D* texture_source = ((data.IsResizing) || (data.PopupWidgetTexture != nullptr)) ? data.StagingTexture.Get() : data.SharedTexture.Get();

            //Flip the cropping rectangle vertically to match flipped input texture
            const int crop_y_flipped = -overlay.OU3D_CropY + data.ViewportSize.height - overlay.OU3D_CropHeight;

            overlay.OU3D_Converter.Convert(D3DManager::Get().GetDevice(), D3DManager::Get().GetDeviceContext(), nullptr, nullptr, texture_source,
                                           texture_desc.Width, texture_desc.Height, overlay.OU3D_CropX, crop_y_flipped, overlay.OU3D_CropWidth, overlay.OU3D_CropHeight);

            //Copy will not be done in time without flushing
            D3DManager::Get().GetDeviceContext()->Flush();
        }
        else if (ovrl_shared_source == vr::k_ulOverlayHandleInvalid) //For the first non-OU3D overlay, set the texture as normal
        {
            ovrl_shared_source = overlay.OverlayHandle;

            //Get overlay texture from OpenVR and copy directly into it if possible
            if (!do_full_copy)
            {
                //Get overlay texture from OpenVR and copy dirty rect directly into it
                ID3D11Texture2D* reference_texture = data.SharedTexture.Get();
                ID3D11ShaderResourceView* ovrl_shader_rsv;

                ovrl_shader_rsv = vr::VROverlayEx()->GetOverlayTextureEx(ovrl_shared_source, reference_texture);
                Vector2Int ovrl_size = vr::VROverlayEx()->GetOverlayTextureSizeEx(ovrl_shared_source);

                if (ovrl_shader_rsv != nullptr)
                {
                    if (texture_desc.Width == 0)
                    {
                        data.SharedTexture->GetDesc(&texture_desc);
                    }

                    if ( (texture_desc.Width == (UINT)ovrl_size.x) && (texture_desc.Height == (UINT)ovrl_size.y) )
                    {
                        Microsoft::WRL::ComPtr<ID3D11Resource> ovrl_tex;
                        ovrl_shader_rsv->GetResource(&ovrl_tex);

                        D3DManager::Get().GetDeviceContext()->CopyResource(ovrl_tex.Get(), reference_texture);
                        D3DManager::Get().GetDeviceContext()->Flush();
                    }
                    else //Texture sizes don't match for some reason, fall back to fully copy
                    {
                        do_full_copy = true;
                    }

                    //RSV is kept around by IVROverlayEx and not released here
                }
                else //Usually shouldn't fail, but fall back to full copy then
                {
                    do_full_copy = true;
                }
            }

            //Do a full copy if needed, utilizing an additional staging texture
            if (do_full_copy)
            {
                //Set the overlay texture from the staging texture
                vr::Texture_t vr_tex = {0};
                vr_tex.eColorSpace = vr::ColorSpace_Gamma;
                vr_tex.eType       = vr::TextureType_DirectX;
                vr_tex.handle      = data.StagingTexture.Get();

                //Don't set the texture while resizing to avoid flickering
                if (!data.IsResizing)
                {
                    //Note how we set the overlay texture before copying the texture. This is because the first frame on a new texture will be blank and we want to avoid flickering
                    vr::VROverlayEx()->SetOverlayTextureEx(ovrl_shared_source, &vr_tex, {(int)texture_desc.Width, (int)texture_desc.Height});

                    //Use this staging texture as a device reference for the shared overlays
                    staging_tex_shared_source = data.StagingTexture;
                }

                OnAcceleratedPaint2UpdateStagingTexture(data);

                data.IsFullCopyScheduled = false;
            }
        }
        else if ( (do_full_copy) && (!data.IsResizing) && (staging_tex_shared_source != nullptr) ) //For all others, set it shared from the normal overlay if an update is needed
        {
            vr::VROverlayEx()->SetSharedOverlayTexture(ovrl_shared_source, overlay.OverlayHandle, staging_tex_shared_source.Get());
        }
    }

    //If there was no main overlay to use as a shared source, then the staging texture wasn't updated even if a full copy was requested
    //Happens and matters rarely (basically all OU3D + widget visible) but still handle it
    if ((staging_tex_shared_source == nullptr) && (do_full_copy))
    {
        OnAcceleratedPaint2UpdateStagingTexture(data);
    }

    //Frame counter
    if (!m_IsPaintCallForIdleFrame) //Don't add idle frames as it's just more confusing when a certain frame rate is expected
    {
        data.FrameCount += (data.FrameCount == -1) ? 2 : 1;     //Add 2 frames if count is set to -1 (previously marked as stale)
    }

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

    //Idle frame handling
    //CEF textures appear to lag behind a frame or two or aren't quite ready when we get this function called
    //Idle frames make sure that we pick up the remaining updates even without getting notified for them when CEF just goes idle
    if (m_IsPaintCallForIdleFrame)
    {
        data.IdleFrameCount++;
        m_IsPaintCallForIdleFrame = false;
        data.IsIdleFrameUpdateScheduled = false;
    }
    else
    {
        data.IdleFrameCount = 0;
    }

    if ( (!data.IsIdleFrameUpdateScheduled) && ((data.IdleFrameCount < 10) || (data.IsResizing)) )
    {
        CefPostDelayedTask(TID_UI, base::BindOnce(&DPBrowserHandler::ScheduledIdleFrameUpdate, this, browser), (data.IsResizing) ? 16 : 100);
        data.IsIdleFrameUpdateScheduled = true;
    }
}

bool DPBrowserHandler::StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data, DragOperationsMask allowed_ops, int x, int y)
{
    return false;
}

void DPBrowserHandler::OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser, TextInputMode input_mode)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = GetBrowserData(*browser);

    const auto& overlay_data = browser_data.Overlays;
    if (!overlay_data.empty())
    {
        //Use last active overlay if available
        vr::VROverlayHandle_t overlay_handle = (browser_data.LastActiveOverlayHandle != vr::k_ulOverlayHandleInvalid) ? browser_data.LastActiveOverlayHandle : overlay_data[0].OverlayHandle;

        DPBrowserAPIServer::Get().NotifyKeyboardShow(overlay_handle, (input_mode != CEF_TEXT_INPUT_MODE_NONE));
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

bool DPBrowserHandler::OnJSDialog(CefRefPtr<CefBrowser> browser, const CefString& origin_url, JSDialogType dialog_type, const CefString& message_text, const CefString& default_prompt_text, 
                                  CefRefPtr<CefJSDialogCallback> callback, bool& suppress_message)
{
    //Just ignore all javascript dialogs for now
    suppress_message = true;
    return false;
}

bool DPBrowserHandler::OnBeforeUnloadDialog(CefRefPtr<CefBrowser> browser, const CefString& message_text, bool is_reload, CefRefPtr<CefJSDialogCallback> callback)
{
    //Just always allow leaving the page
    callback->Continue(true, "");
    return true;
}

CefRefPtr<CefResourceRequestHandler> DPBrowserHandler::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, 
                                                                                 const CefString& request_initiator, bool& disable_default_handling)
{
    return m_ContentBlocker;
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

    DPBrowserData* browser_data = nullptr;
    DPBrowserOverlayData& overlay_data_src = FindBrowserOverlayData(overlay_handle_src, &browser_data);

    if ((browser_data != nullptr) && (browser_data->BrowserPtr != nullptr))
    {
        DPBrowserOverlayData overlay_data = overlay_data_src;
        overlay_data.OverlayHandle = overlay_handle_dst;
        browser_data->Overlays.push_back(overlay_data);

        //To set the shared texture, we need a fresh full texture copy. This also needs to trigger a redraw even on static pages
        ForceRedraw(*browser_data);
    }
    else
    {
        DVLOG(0) << "DPBrowser_DuplicateBrowserOutput: Source overlay wasn't found in any active browser";
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
            vr::VROverlayEx()->ReleaseSharedOverlayTexture(it->OverlayHandle);
            browser_data.Overlays.erase(it);

            //The removed overlay might've been used as a shared source, so we need to force a fresh full frame
            ForceRedraw(browser_data);

            //Reset last active overlay if it's the removed one
            if (browser_data.LastActiveOverlayHandle == overlay_handle)
            {
                browser_data.LastActiveOverlayHandle = vr::k_ulOverlayHandleInvalid;
            }
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

void DPBrowserHandler::DPBrowser_SetOverUnder3D(vr::VROverlayHandle_t overlay_handle, bool is_over_under_3D, int crop_x, int crop_y, int crop_width, int crop_height)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData* browser_data = nullptr;
    DPBrowserOverlayData& overlay_data = FindBrowserOverlayData(overlay_handle, &browser_data);

    if (browser_data != nullptr)
    {
        overlay_data.IsOverUnder3D   = is_over_under_3D;
        overlay_data.OU3D_CropX      = crop_x;
        overlay_data.OU3D_CropY      = crop_y;
        overlay_data.OU3D_CropWidth  = crop_width;
        overlay_data.OU3D_CropHeight = crop_height;

        if (browser_data->BrowserPtr != nullptr)
        {
            browser_data->IsResizing = true;
            browser_data->ResizingFrameCount = 0;
            ForceRedraw(*browser_data);
        }
    }
}

void DPBrowserHandler::DPBrowser_MouseMove(vr::VROverlayHandle_t overlay_handle, int x, int y)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        m_MouseState.CefEvent.x = x;
        m_MouseState.CefEvent.y = y;

        browser_data.BrowserPtr->GetHost()->SendMouseMoveEvent(m_MouseState.CefEvent, false);
    }
}

void DPBrowserHandler::DPBrowser_MouseLeave(vr::VROverlayHandle_t overlay_handle)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.BrowserPtr->GetHost()->SendMouseMoveEvent(m_MouseState.CefEvent, true);
        m_MouseState.LastButtonClickCount = 0;
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
            case vr::VRMouseButton_Left:   cefbutton = MBT_LEFT;   m_MouseState.CefEvent.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;   break;
            case vr::VRMouseButton_Right:  cefbutton = MBT_RIGHT;  m_MouseState.CefEvent.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;  break;
            case vr::VRMouseButton_Middle: cefbutton = MBT_MIDDLE; m_MouseState.CefEvent.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON; break;
        }

        //Reset click count if different button, click count is already 2 or over double click time (we cycle after a double-click to imitate desktop browser behavior)
        if ( (cefbutton != m_MouseState.LastButton) || (m_MouseState.LastButtonClickCount == 2) || (m_MouseState.LastButtonClickTick + ::GetDoubleClickTime() < ::GetTickCount64()) )
        {
            m_MouseState.LastButtonClickCount = 0;
        }

        m_MouseState.LastButton = cefbutton;
        m_MouseState.LastButtonClickCount++;
        m_MouseState.LastButtonClickTick = ::GetTickCount64();

        browser_data.BrowserPtr->GetHost()->SendMouseClickEvent(m_MouseState.CefEvent, cefbutton, false, m_MouseState.LastButtonClickCount);

        browser_data.LastActiveOverlayHandle = overlay_handle;
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
            case vr::VRMouseButton_Left:   cefbutton = MBT_LEFT;   m_MouseState.CefEvent.modifiers &= ~EVENTFLAG_LEFT_MOUSE_BUTTON;   break;
            case vr::VRMouseButton_Right:  cefbutton = MBT_RIGHT;  m_MouseState.CefEvent.modifiers &= ~EVENTFLAG_RIGHT_MOUSE_BUTTON;  break;
            case vr::VRMouseButton_Middle: cefbutton = MBT_MIDDLE; m_MouseState.CefEvent.modifiers &= ~EVENTFLAG_MIDDLE_MOUSE_BUTTON; break;
        }

        browser_data.BrowserPtr->GetHost()->SendMouseClickEvent(m_MouseState.CefEvent, cefbutton, true, m_MouseState.LastButtonClickCount);
    }
}

void DPBrowserHandler::DPBrowser_Scroll(vr::VROverlayHandle_t overlay_handle, float x_delta, float y_delta)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        browser_data.BrowserPtr->GetHost()->SendMouseWheelEvent(m_MouseState.CefEvent, x_delta * WHEEL_DELTA, y_delta * WHEEL_DELTA);
    }
}

void DPBrowserHandler::DPBrowser_KeyboardSetKeyState(vr::VROverlayHandle_t overlay_handle, DPBrowserIPCKeyboardKeystateFlags flags, unsigned char keycode)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        //Mouse buttons need to be handled differently (X1 and X2 are not really supported, but still sent as keyboard events)
        if ((keycode <= 4) && (keycode != VK_CANCEL))
        {
            vr::EVRMouseButton vr_mouse_button = vr::VRMouseButton_Left;

            switch (keycode)
            {
                case VK_LBUTTON: vr_mouse_button = vr::VRMouseButton_Left;   break;
                case VK_RBUTTON: vr_mouse_button = vr::VRMouseButton_Right;  break;
                case VK_MBUTTON: vr_mouse_button = vr::VRMouseButton_Middle; break;
            }

            if (flags & dpbrowser_ipckbd_keystate_flag_key_down)
            {
                DPBrowser_MouseDown(overlay_handle, vr_mouse_button);
            }
            else
            {
                DPBrowser_MouseUp(overlay_handle, vr_mouse_button);
            }
            return;
        }

        //CefKeyEvent::native_key_code is expected to be the lParam you get in WM_KEYDOWN/UP on Windows
        //So, it's actually scancode + keystroke flags. We don't have those, so we need to make them up in order to make things work as they should.

        //Get scancode
        UINT scancode = ::MapVirtualKey(keycode, MAPVK_VK_TO_VSC_EX);

        BYTE highbyte = HIBYTE(scancode);
        bool is_extended = ((highbyte == 0xe0) || (highbyte == 0xe1));

        //Some keys need some extra fiddling
        switch (keycode)
        {
            //Modifiers need to use their generic variant (scancode still differs though)
            case VK_LSHIFT:
            case VK_RSHIFT:
            {
                keycode = VK_SHIFT;
                break;
            }
            case VK_LCONTROL:
            case VK_RCONTROL:
            {
                keycode = VK_CONTROL;
                break;
            }
            case VK_LMENU:
            case VK_RMENU:
            {
                keycode = VK_MENU;
                break;
            }
            //Not extended but needs to be for expected results
            case VK_NUMLOCK:
            case VK_INSERT:
            case VK_DELETE:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_END:
            case VK_HOME:
            case VK_LEFT:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
            {
                is_extended = true;
                break;
            }
            //MapVirtualKey returns an unexpected scan code for VK_PAUSE and wrongly extends it (VK_NUMLOCK needs it instead)
            case VK_PAUSE:
            {
                scancode = 0x45;
                is_extended = false;
                break;
            }
            //VK_SNAPSHOT is an odd one. Scancode 0x37 (VK_MULTIPLY) is technically not correct, but it's what you get in window messages, so we force that value instead
            case VK_SNAPSHOT:
            {
                scancode = 0x37;
                is_extended = true;
                break;
            }
            default: break;
        }

        const bool is_alt_down = ( (flags & dpbrowser_ipckbd_keystate_flag_lalt_down) || (flags & dpbrowser_ipckbd_keystate_flag_ralt_down) );

        //Set keystroke flags
        WORD keystroke_flags = scancode;

        if (is_extended)
            keystroke_flags |= KF_EXTENDED;
        if (!(flags & dpbrowser_ipckbd_keystate_flag_key_down))
            keystroke_flags |= KF_REPEAT | KF_UP;
        if ( (is_alt_down) || (keycode == VK_LMENU) || (keycode == VK_RMENU) || (keycode == VK_MENU) )
            keystroke_flags |= KF_ALTDOWN;

        LPARAM scancode_with_keystroke_flags = MAKELPARAM(1 /*repeat count*/, keystroke_flags);

        //Set CefKeyEvent
        CefKeyEvent key_event;
        key_event.windows_key_code = keycode;
        key_event.native_key_code  = scancode_with_keystroke_flags;
        key_event.type             = (flags & dpbrowser_ipckbd_keystate_flag_key_down) ? KEYEVENT_RAWKEYDOWN : KEYEVENT_KEYUP;
        key_event.modifiers        = m_MouseState.CefEvent.modifiers;

        if ( (flags & dpbrowser_ipckbd_keystate_flag_lshift_down) || (flags & dpbrowser_ipckbd_keystate_flag_rshift_down) )
            key_event.modifiers |= EVENTFLAG_SHIFT_DOWN;
        if ( (flags & dpbrowser_ipckbd_keystate_flag_lctrl_down) || (flags & dpbrowser_ipckbd_keystate_flag_rctrl_down) )
            key_event.modifiers |= EVENTFLAG_CONTROL_DOWN;
        if (is_alt_down)
            key_event.modifiers |= EVENTFLAG_ALT_DOWN;
        if (flags & dpbrowser_ipckbd_keystate_flag_capslock_toggled)
            key_event.modifiers |= EVENTFLAG_CAPS_LOCK_ON;

        browser_data.BrowserPtr->GetHost()->SendKeyEvent(key_event);

        browser_data.LastActiveOverlayHandle = overlay_handle;
    }
}

void DPBrowserHandler::DPBrowser_KeyboardToggleKey(vr::VROverlayHandle_t overlay_handle, unsigned char keycode)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        unsigned char dpflags = (browser_data.KeyboardToggledKeys[keycode]) ? 0 : dpbrowser_ipckbd_keystate_flag_key_down;
        DPBrowser_KeyboardSetKeyState(overlay_handle, (DPBrowserIPCKeyboardKeystateFlags)dpflags, keycode);

        browser_data.KeyboardToggledKeys[keycode] = !browser_data.KeyboardToggledKeys[keycode];

        browser_data.LastActiveOverlayHandle = overlay_handle;
    }
}

void DPBrowserHandler::DPBrowser_KeyboardTypeWChar(vr::VROverlayHandle_t overlay_handle, wchar_t wchar, bool down)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        //Check if it can be pressed on the current windows keyboard layout and send key down/up events then
        SHORT w32_keystate = ::VkKeyScan(wchar);
        unsigned char keycode = LOBYTE(w32_keystate);
        unsigned char flags   = HIBYTE(w32_keystate);

        bool is_valid = ((keycode != -1) || (flags != -1));

        if (is_valid)
        {
            //Set keystate flags from win32 keystate
            unsigned char dpflags = (down) ? dpbrowser_ipckbd_keystate_flag_key_down : 0;

            if (flags & kbd_w32keystate_flag_shift_down)
                dpflags |= dpbrowser_ipckbd_keystate_flag_lshift_down;
            if (flags & kbd_w32keystate_flag_ctrl_down)
                dpflags |= dpbrowser_ipckbd_keystate_flag_lctrl_down;
            if (flags & kbd_w32keystate_flag_alt_down)
                dpflags |= dpbrowser_ipckbd_keystate_flag_lalt_down;

            //Send key down/up event
            DPBrowser_KeyboardSetKeyState(overlay_handle, (DPBrowserIPCKeyboardKeystateFlags)dpflags, keycode);
        }

        //Also send wchar event if the key is down
        if (down)
        {
            browser_data.BrowserPtr->GetHost()->SendKeyEvent(KeyboardGenerateWCharEvent(wchar));
        }

        browser_data.LastActiveOverlayHandle = overlay_handle;
    }
}

void DPBrowserHandler::DPBrowser_KeyboardTypeString(vr::VROverlayHandle_t overlay_handle, const std::string& str)
{
    CEF_REQUIRE_UI_THREAD();

    DPBrowserData& browser_data = FindBrowserDataForOverlay(overlay_handle);

    if (browser_data.BrowserPtr != nullptr)
    {
        std::wstring wstr = WStringConvertFromUTF8(str.c_str());

        for (wchar_t wchar : wstr)
        {
            browser_data.BrowserPtr->GetHost()->SendKeyEvent(KeyboardGenerateWCharEvent(wchar));
        }

        browser_data.LastActiveOverlayHandle = overlay_handle;
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

void DPBrowserHandler::DPBrowser_ContentBlockSetEnabled(bool enable)
{
    CEF_REQUIRE_IO_THREAD();

    m_ContentBlocker->SetEnabled(enable);
}
