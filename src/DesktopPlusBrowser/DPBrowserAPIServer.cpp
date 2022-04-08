#include "DPBrowserAPIServer.h"

#include <windowsx.h>

#include "DPBrowserHandler.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

#include "Util.h"

static DPBrowserAPIServer g_DPBrowserAPIServer;

std::string& DPBrowserAPIServer::GetIPCString(DPBrowserICPStringID str_id)
{
    return m_IPCStrings[str_id - dpbrowser_ipcstr_MIN];
}

void DPBrowserAPIServer::SendStringMessage(HWND target_window, DPBrowserICPStringID str_id, const std::string& str) const
{
    COPYDATASTRUCT cds = {0};
    cds.dwData = str_id;
    cds.cbData = (DWORD)str.length();  //We do not include the NUL byte
    cds.lpData = (void*)str.c_str();

    ::SendMessage(target_window, WM_COPYDATA, (WPARAM)m_WindowHandle, (LPARAM)(LPVOID)&cds);
}

DPBrowserAPIServer& DPBrowserAPIServer::Get()
{
    return g_DPBrowserAPIServer;
}

bool DPBrowserAPIServer::Init(HINSTANCE hinstance, CefRefPtr<CefApp> cef_app)
{
    m_CEFApp = cef_app;

    //Wait for the browser handler (and CEF) to be ready since we'll need it as soon as we take API messages
    while (DPBrowserHandler::Get() == nullptr)
    {
        //This *might* be problematic if DPBrowserApp::OnContextInitialized() never gets called for some reason, but not sure if that can happen
        //Then again, nothing would work without CEF running either
        ::Sleep(50);
    }

    DVLOG(0) << "API Server Init";

    //Register class
    WNDCLASSEXW wc = {};
    wc.cbSize           = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc      = DPBrowserAPIServer::WndProc;
    wc.hInstance        = hinstance;
    wc.lpszClassName    = g_WindowClassNameBrowserApp;

    if (!::RegisterClassExW(&wc))
    {
        return false;
    }

    //Create message window
    m_WindowHandle = ::CreateWindowW(wc.lpszClassName, L"Desktop+ Browser", 0, 0, 0, 1, 1, HWND_DESKTOP, nullptr, wc.hInstance, nullptr);

    //Register custom message ID
    m_Win32MessageID = ::RegisterWindowMessage(g_WindowMessageNameBrowserApp);

    return (m_WindowHandle != nullptr);
}

LRESULT DPBrowserAPIServer::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //Messages sent via SendMessage() requiring some result
    if (message == Get().m_Win32MessageID)
    {
        switch (wParam)
        {
            case dpbrowser_ipccmd_get_api_version:
            {
                return k_lDPBrowserAPIVersion;
            }
        }

        return 0;
    }

    switch (message)
    {
        case WM_COPYDATA:
        {
            MSG msg;
            //Process all custom window messages posted before this
            while (::PeekMessage(&msg, nullptr, 0xC000, 0xFFFF, PM_REMOVE))
            {
                DPBrowserAPIServer::Get().HandleIPCMessage(msg);
            }

            msg.hwnd = hWnd;
            msg.message = message;
            msg.wParam = wParam;
            msg.lParam = lParam;

            DPBrowserAPIServer::Get().HandleIPCMessage(msg);
            break;
        }
        case WM_DESTROY:
        {
            ::PostQuitMessage(0);
            break;
        }
        default:
            return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void DPBrowserAPIServer::HandleIPCMessage(const MSG& msg)
{
    if (msg.message == WM_COPYDATA)
    {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)msg.lParam;

        //Arbitrary size limit to prevent some malicous applications from sending bad data
        if ( (pcds->dwData < dpbrowser_ipcstr_MAX) && (pcds->cbData <= 4096) ) 
        {
            std::string copystr((char*)pcds->lpData, pcds->cbData); //We rely on the data length. The data is sent without the NUL byte

            DPBrowserICPStringID str_id = (DPBrowserICPStringID)pcds->dwData;
            GetIPCString(str_id) = copystr;

            DVLOG(0) << "WM_COPYDATA: \"" << copystr << "\" (" << str_id << ")";
        }

        return;
    }
    else if (msg.message != m_Win32MessageID)
    {
        return;
    }

    //Basic commands handled internally
    switch (msg.wParam)
    {
        case dpbrowser_ipccmd_set_overlay_target:
        {
            DVLOG(1) << "dpbrowser_ipccmd_set_overlay_target: " << std::hex << msg.lParam;

            m_IPCOverlayTarget = msg.lParam;
            return;
        }
    }

    //All other commands calling into the BrowserHandler
    if (CefRefPtr<DPBrowserHandler> handler = DPBrowserHandler::Get())
    {
        switch (msg.wParam)
        {
            case dpbrowser_ipccmd_start_browser:
            {
                DVLOG(0) << "dpbrowser_ipccmd_start_browser: " << std::hex << m_IPCOverlayTarget << " (overlay_handle) " << std::dec << msg.lParam << " (use_transparent_background)";

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_StartBrowser, handler, m_IPCOverlayTarget, GetIPCString(dpbrowser_ipcstr_url), msg.lParam) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_duplicate_browser_output:
            {
                DVLOG(0) << "dpbrowser_ipccmd_duplicate_browser_output: " << std::hex << msg.lParam << " (overlay_handle_dst)";

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_DuplicateBrowserOutput, handler, m_IPCOverlayTarget, msg.lParam) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_pause_browser:
            {
                DVLOG(0) << "dpbrowser_ipccmd_pause_browser: " << std::hex << m_IPCOverlayTarget << " (overlay_handle) " << std::dec << msg.lParam << " (pause)";

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_PauseBrowser, handler, m_IPCOverlayTarget, msg.lParam) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_recreate_browser:
            {
                DVLOG(0) << "dpbrowser_ipccmd_recreate_browser: " << std::hex << m_IPCOverlayTarget << " (overlay_handle) " << std::dec << msg.lParam << " (use_transparent_background)";

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_RecreateBrowser, handler, m_IPCOverlayTarget, msg.lParam) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_stop_browser:
            {
                DVLOG(0) << "dpbrowser_ipccmd_stop_browser: " << std::hex << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_StopBrowser, handler, msg.lParam) );
                break;
            }
            case dpbrowser_ipccmd_set_url:
            {
                DVLOG(0) << "dpbrowser_ipccmd_set_url: " << std::hex << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_SetURL, handler, msg.lParam, GetIPCString(dpbrowser_ipcstr_url)) );
                break;
            }
            case dpbrowser_ipccmd_set_resoution:
            {
                DVLOG(0) << "dpbrowser_ipccmd_set_resoution: " << std::hex << m_IPCOverlayTarget << " (overlay_handle) " << std::dec << GET_X_LPARAM(msg.lParam) << "x" << GET_Y_LPARAM(msg.lParam);

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_SetResolution, handler, m_IPCOverlayTarget, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_set_fps:
            {
                DVLOG(0) << "dpbrowser_ipccmd_set_fps: " << std::hex << m_IPCOverlayTarget << " (overlay_handle) " << std::dec << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_SetFPS, handler, m_IPCOverlayTarget, msg.lParam) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_set_zoom:
            {
                DVLOG(0) << "dpbrowser_ipccmd_set_zoom: " << std::hex << m_IPCOverlayTarget << " (overlay_handle) " << std::dec << pun_cast<float, LPARAM>(msg.lParam) * 100 << "%";

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_SetZoomLevel, handler, m_IPCOverlayTarget, pun_cast<float, LPARAM>(msg.lParam)) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_mouse_move:
            {
                DVLOG(1) << "dpbrowser_ipccmd_mouse_move: " << GET_X_LPARAM(msg.lParam) << ", " << GET_Y_LPARAM(msg.lParam);

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_MouseMove, handler, m_IPCOverlayTarget, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam)) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_mouse_leave:
            {
                DVLOG(1) << "dpbrowser_ipccmd_mouse_leave: " << std::hex << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_MouseLeave, handler, msg.lParam) );
                break;
            }
            case dpbrowser_ipccmd_mouse_down:
            {
                DVLOG(1) << "dpbrowser_ipccmd_mouse_down: " << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_MouseDown, handler, m_IPCOverlayTarget, (vr::EVRMouseButton)msg.lParam) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_mouse_up:
            {
                DVLOG(1) << "dpbrowser_ipccmd_mouse_up: " << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_MouseUp, handler, m_IPCOverlayTarget, (vr::EVRMouseButton)msg.lParam) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_scroll:
            {
                //Unpack packed floats
                float x_delta = pun_cast<float, DWORD>(LODWORD(msg.lParam));
                float y_delta = pun_cast<float, DWORD>(HIDWORD(msg.lParam));

                DVLOG(1) << "dpbrowser_ipccmd_scroll: " << x_delta << ", " << y_delta;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_Scroll, handler, m_IPCOverlayTarget, x_delta, y_delta) );
                m_IPCOverlayTarget = vr::k_ulOverlayHandleInvalid;
                break;
            }
            case dpbrowser_ipccmd_go_back:
            {
                DVLOG(0) << "dpbrowser_ipccmd_go_back: " << std::hex << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_GoBack, handler, msg.lParam) );
                break;
            }
            case dpbrowser_ipccmd_go_forward:
            {
                DVLOG(0) << "dpbrowser_ipccmd_go_forward: " << std::hex << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_GoForward, handler, msg.lParam) );
                break;
            }
            case dpbrowser_ipccmd_refresh:
            {
                DVLOG(0) << "dpbrowser_ipccmd_refresh: " << std::hex << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_Refresh, handler, msg.lParam) );
                break;
            }
            case dpbrowser_ipccmd_global_set_fps:
            {
                DVLOG(0) << "dpbrowser_ipccmd_global_set_fps: " << msg.lParam;

                CefPostTask(TID_UI, base::BindOnce(&DPBrowserHandler::DPBrowser_GlobalSetFPS, handler, msg.lParam) );
            }
        }
    }
}

void DPBrowserAPIServer::NotifyNavigationState(vr::VROverlayHandle_t overlay_handle, bool can_go_back, bool can_go_forward, bool is_loading)
{
    //Send notifcation to UI app
    if (HWND window = ::FindWindow(g_WindowClassNameUIApp, nullptr))
    {
        unsigned char flags = 0;
        if (can_go_back)    { flags |= dpbrowser_ipcnavstate_flag_can_go_back;    }
        if (can_go_forward) { flags |= dpbrowser_ipcnavstate_flag_can_go_forward; }
        if (is_loading)     { flags |= dpbrowser_ipcnavstate_flag_is_loading;     }

        ::PostMessage(window, m_Win32MessageID, dpbrowser_ipccmd_set_overlay_target, overlay_handle);
        ::PostMessage(window, m_Win32MessageID, dpbrowser_ipccmd_notify_nav_state, flags);
    }
}

void DPBrowserAPIServer::NotifyURLChange(vr::VROverlayHandle_t overlay_handle, const std::string& url)
{
    //Send notifcation to UI app
    if (HWND window = ::FindWindow(g_WindowClassNameUIApp, nullptr))
    {
        SendStringMessage(window, dpbrowser_ipcstr_url, url);
        ::PostMessage(window, m_Win32MessageID, dpbrowser_ipccmd_notify_url_changed, overlay_handle);
    }
}

void DPBrowserAPIServer::NotifyTitleChange(vr::VROverlayHandle_t overlay_handle, const std::string& title)
{
    //Send notifcation to UI app
    if (HWND window = ::FindWindow(g_WindowClassNameUIApp, nullptr))
    {
        SendStringMessage(window, dpbrowser_ipcstr_title, title);
        ::PostMessage(window, m_Win32MessageID, dpbrowser_ipccmd_notify_title_changed, overlay_handle);
    }
}

void DPBrowserAPIServer::NotifyFPS(vr::VROverlayHandle_t overlay_handle, int fps)
{
    //Send notifcation to UI app
    if (HWND window = ::FindWindow(g_WindowClassNameUIApp, nullptr))
    {
        ::PostMessage(window, m_Win32MessageID, dpbrowser_ipccmd_set_overlay_target, overlay_handle);
        ::PostMessage(window, m_Win32MessageID, dpbrowser_ipccmd_notify_fps, fps);
    }
}

void DPBrowserAPIServer::NotifyLaserPointerHaptics()
{
    //Send notifcation to dashboard app
    if (HWND window = ::FindWindow(g_WindowClassNameDashboardApp, nullptr))
    {
        ::PostMessage(window, m_Win32MessageID, dpbrowser_ipccmd_notify_lpointer_haptics, 0);
    }
}
