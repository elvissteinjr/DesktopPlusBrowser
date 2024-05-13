#pragma once

#include "DPBrowserAPI.h"

#include "include/cef_app.h"

//Duplicated strings, but avoids pulling in IPCManager, which would not work as expected with this
LPCWSTR const g_WindowClassNameDashboardApp = L"elvdesktop";
LPCWSTR const g_WindowClassNameUIApp        = L"elvdesktopUI";

//This is the server part for the Desktop+Browser API
//It hosts a message window to receive API calls made in DPBrowserAPIClient.h as window messages, generally attempting to present this as if they were a direct library calls
//DPBrowserHandler hosts the CEFBrowsers along with overlay data and implements DPBrowserAPI, which this class calls into
class DPBrowserAPIServer
{
    private:
        HWND m_WindowHandle = nullptr;
        UINT m_Win32MessageID = 0;
        CefRefPtr<CefApp> m_CEFApp = nullptr;

        std::string m_IPCStrings[dpbrowser_ipcstr_MAX - dpbrowser_ipcstr_MIN];
        vr::VROverlayHandle_t m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;

        std::string& GetIPCString(DPBrowserICPStringID str_id);                                                 //Abstracts the minimum string ID away when acccessing m_IPCStrings
        void SendStringMessage(HWND target_window, DPBrowserICPStringID str_id, const std::string& str) const;

    public:
        static DPBrowserAPIServer& Get();

        bool Init(HINSTANCE hinstance, CefRefPtr<CefApp> cef_app);
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        void HandleIPCMessage(const MSG& msg);

        //Client Notifications (these send dpbrowser_ipccmd_notify_* messages to dashboard or UI app)
        void NotifyReady();
        void NotifyNavigationState(vr::VROverlayHandle_t overlay_handle, bool can_go_back, bool can_go_forward, bool is_loading);
        void NotifyURLChange(vr::VROverlayHandle_t overlay_handle, const std::string& url);
        void NotifyTitleChange(vr::VROverlayHandle_t overlay_handle, const std::string& title);
        void NotifyFPS(vr::VROverlayHandle_t overlay_handle, int fps);
        void NotifyLaserPointerHaptics();
        void NotifyKeyboardShow(vr::VROverlayHandle_t overlay_handle, bool show);
        void NotifyContentBlockListCount(int list_count);
};