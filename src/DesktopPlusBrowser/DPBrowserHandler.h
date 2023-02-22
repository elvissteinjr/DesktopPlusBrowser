#pragma once

#include "include/cef_client.h"

#include <list>
#include <unordered_map>

#include "D3DManager.h"
#include "DPBrowserAPI.h"
#include "DPBrowserContentBlocker.h"
#include "OUtoSBSConverter.h"

struct DPBrowserOverlayData
{
    vr::VROverlayHandle_t OverlayHandle = vr::k_ulOverlayHandleInvalid;
    bool IsHidden = false;
    bool IsOverUnder3D = false;
    int MaxFPS = -1;
    int OU3D_CropX = 0;
    int OU3D_CropY = 0;
    int OU3D_CropWidth  = 1;
    int OU3D_CropHeight = 1;
    OUtoSBSConverter OU3D_Converter;
};

struct DPBrowserData
{
    //Properties
    std::vector<DPBrowserOverlayData> Overlays;
    CefRect ViewportSize = {0, 0, 1280, 720};
    CefRect ViewportSizePending;
    float ZoomLevel = 1.0f;

    //State
    CefRefPtr<CefBrowser> BrowserPtr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> SharedTexture;      //Texture shared by CEF
    Microsoft::WRL::ComPtr<ID3D11Texture2D> StagingTexture;     //Texture sent to OpenVR
    Microsoft::WRL::ComPtr<ID3D11Texture2D> PopupWidgetTexture; //Texture shared by CEF, used for popup widgets (i.e. form dropdowns)
    std::string LastNotifiedURL;
    bool IsFullCopyScheduled = true;
    bool IsResizing = false;
    bool IsPopupWidgetVisible = false;
    CefRect PopupWidgetRect;
    int ResizingFrameCount = 0;
    int IdleFrameCount = 0;
    bool IsIdleFrameUpdateScheduled = false;
    int FrameCount = 0;
    ULONGLONG FrameCountStartTick = 0;
    bool KeyboardToggledKeys[256] = {false};                                        //Tracks state of toggled keys between API calls (only DPBrowser_KeyboardToggleKey())
    vr::VROverlayHandle_t LastActiveOverlayHandle = vr::k_ulOverlayHandleInvalid;   //Last overlay used as target for keyboard or mouse input (excluding mouse move/scroll)
};

struct DPBrowserMouseState
{
    CefMouseEvent CefEvent;
    cef_mouse_button_type_t LastButton = MBT_LEFT;
    int LastButtonClickCount = 0;                   //Track click count within double-click time-window to trigger double-click events
    ULONGLONG LastButtonClickTick = 0;
};

//Browser handler for CEF and Desktop+. Implements DPBrowserAPI functions called by DPBrowserAPIServer
class DPBrowserHandler : public CefClient, public CefDisplayHandler, public CefLifeSpanHandler, public CefLoadHandler, public CefRenderHandler, public CefContextMenuHandler,
                         public CefDownloadHandler, public CefJSDialogHandler, public CefRequestHandler, public DPBrowserAPI
{
    private:
        //List of browsers and browser-global state. Only accessed on the CEF UI thread.
        std::vector<DPBrowserData> m_BrowserList;
        std::unordered_map<int, int> m_BrowserIDToDataMap;  //Maps CEF browser identifier to ID of m_BrowserList
        DPBrowserData m_BrowserDataNull;
        DPBrowserOverlayData m_BrowserOverlayDataNull;

        int m_GlobalMaxFPS = 60;

        DPBrowserMouseState m_MouseState;
        bool m_IsStaleFPSValueTaskPending = false;
        bool m_IsPaintCallForIdleFrame    = false;

        //Content blocker, only accessed on the CEF IO thread
        CefRefPtr<DPBrowserContentBlocker> m_ContentBlocker;

        //--These need to be called on the browser/CEF UI thread
        DPBrowserData& GetBrowserData(unsigned int id);
        DPBrowserData& GetBrowserData(CefBrowser& browser);
        DPBrowserData& FindBrowserDataForOverlay(vr::VROverlayHandle_t overlay_handle);     //May return m_BrowserDataNull
        DPBrowserOverlayData& FindBrowserOverlayData(vr::VROverlayHandle_t overlay_handle, DPBrowserData** out_parent_browser_data = nullptr); //May return m_BrowserOverlayDataNull

        void ForceRedraw(DPBrowserData& browser_data);
        void ScheduledIdleFrameUpdate(CefRefPtr<CefBrowser> browser);
        void TryApplyPendingResolution(vr::VROverlayHandle_t overlay_handle);
        void ApplyMaxFPS(CefBrowser& browser);
        void CheckStaleFPSValues();
        void OnAcceleratedPaint2UpdateStagingTexture(DPBrowserData& data);
        CefKeyEvent KeyboardGenerateWCharEvent(wchar_t wchar);
        //--

        IMPLEMENT_REFCOUNTING(DPBrowserHandler);

    public:
        explicit DPBrowserHandler();
        ~DPBrowserHandler();

        static CefRefPtr<DPBrowserHandler> Get();
        static std::string GetDataURI(const std::string& data, const std::string& mime_type);

        void CloseAllBrowsers(bool force_close);

        //CefClient:
        virtual CefRefPtr<CefDisplayHandler>     GetDisplayHandler()     override { return this; }
        virtual CefRefPtr<CefLifeSpanHandler>    GetLifeSpanHandler()    override { return this; }
        virtual CefRefPtr<CefLoadHandler>        GetLoadHandler()        override { return this; }
        virtual CefRefPtr<CefRenderHandler>      GetRenderHandler()      override { return this; }
        virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
        virtual CefRefPtr<CefDownloadHandler>    GetDownloadHandler()    override { return this; }
        virtual CefRefPtr<CefJSDialogHandler>    GetJSDialogHandler()    override { return this; }
        virtual CefRefPtr<CefRequestHandler>     GetRequestHandler()     override { return this; }

        //CefDisplayHandler:
        virtual void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) override;
        virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
        virtual bool OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, cef_cursor_type_t type, const CefCursorInfo& custom_cursor_info) override;

        //CefLifeSpanHandler:
        virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
        virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
        virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
        virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name,
                                   cef_window_open_disposition_t target_disposition, bool user_gesture, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, 
                                   CefRefPtr<CefClient>& client, CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue>& extra_info, bool* no_javascript_access) override;

        //CefLoadHandler:
        virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;
        virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override;

        //CefRenderHandler:
        virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;
        virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;
        virtual void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) override;
        virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) override;
        virtual void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, void *shared_handle) override;
        virtual void OnAcceleratedPaint2(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, void *shared_handle, bool new_texture) override;
        virtual bool StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data, DragOperationsMask allowed_ops, int x, int y) override;
        virtual void OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser, TextInputMode input_mode) override;

        //CefContextMenuHandler:
        virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) override;

        //CefDownloadHandler:
        virtual void OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString& suggested_name, 
                                      CefRefPtr<CefBeforeDownloadCallback> callback) override;
        virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback) override;

        //CefJSDialogHandler:
        virtual bool OnJSDialog(CefRefPtr<CefBrowser> browser, const CefString& origin_url, JSDialogType dialog_type, const CefString& message_text, const CefString& default_prompt_text,
                                CefRefPtr<CefJSDialogCallback> callback, bool& suppress_message) override;
        virtual bool OnBeforeUnloadDialog(CefRefPtr<CefBrowser> browser, const CefString& message_text, bool is_reload, CefRefPtr<CefJSDialogCallback> callback) override;

        //CefRequestHandler
        virtual CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation,
                                                                               bool is_download, const CefString& request_initiator, bool& disable_default_handling) override;

        //DPBrowserAPI:
        virtual void DPBrowser_StartBrowser(vr::VROverlayHandle_t overlay_handle, const std::string& url, bool use_transparent_background) override;
        virtual void DPBrowser_DuplicateBrowserOutput(vr::VROverlayHandle_t overlay_handle_src, vr::VROverlayHandle_t overlay_handle_dst) override;
        virtual void DPBrowser_PauseBrowser(vr::VROverlayHandle_t overlay_handle, bool pause) override;
        virtual void DPBrowser_RecreateBrowser(vr::VROverlayHandle_t overlay_handle, bool use_transparent_background) override;
        virtual void DPBrowser_StopBrowser(vr::VROverlayHandle_t overlay_handle) override;

        virtual void DPBrowser_SetURL(vr::VROverlayHandle_t overlay_handle, const std::string& url) override;
        virtual void DPBrowser_SetResolution(vr::VROverlayHandle_t overlay_handle, int width, int height) override;
        virtual void DPBrowser_SetFPS(vr::VROverlayHandle_t overlay_handle, int fps) override;
        virtual void DPBrowser_SetZoomLevel(vr::VROverlayHandle_t overlay_handle, float zoom_level) override;
        virtual void DPBrowser_SetOverUnder3D(vr::VROverlayHandle_t overlay_handle, bool is_over_under_3D, int crop_x, int crop_y, int crop_width, int crop_height) override;

        virtual void DPBrowser_MouseMove(vr::VROverlayHandle_t overlay_handle, int x, int y) override;
        virtual void DPBrowser_MouseLeave(vr::VROverlayHandle_t overlay_handle) override;
        virtual void DPBrowser_MouseDown(vr::VROverlayHandle_t overlay_handle, vr::EVRMouseButton button) override;
        virtual void DPBrowser_MouseUp(vr::VROverlayHandle_t overlay_handle, vr::EVRMouseButton button) override;
        virtual void DPBrowser_Scroll(vr::VROverlayHandle_t overlay_handle, float x_delta, float y_delta) override;

        virtual void DPBrowser_KeyboardSetKeyState(vr::VROverlayHandle_t overlay_handle, DPBrowserIPCKeyboardKeystateFlags flags, unsigned char keycode) override;
        virtual void DPBrowser_KeyboardToggleKey(vr::VROverlayHandle_t overlay_handle, unsigned char keycode) override;
        virtual void DPBrowser_KeyboardTypeWChar(vr::VROverlayHandle_t overlay_handle, wchar_t wchar, bool down) override;
        virtual void DPBrowser_KeyboardTypeString(vr::VROverlayHandle_t overlay_handle, const std::string& str) override;

        virtual void DPBrowser_GoBack(vr::VROverlayHandle_t overlay_handle) override;
        virtual void DPBrowser_GoForward(vr::VROverlayHandle_t overlay_handle) override;
        virtual void DPBrowser_Refresh(vr::VROverlayHandle_t overlay_handle) override;

        virtual void DPBrowser_GlobalSetFPS(int fps) override;
        virtual void DPBrowser_ContentBlockSetEnabled(bool enable) override;    //Call on IO thread
};
