#include "DPBrowserApp.h"

#include <string>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "DPBrowserHandler.h"

#include "D3DManager.h"
#include "DPBrowserAPIServer.h"

DPBrowserApp::DPBrowserApp() 
{
    //Init OpenVR
    vr::EVRInitError init_error = vr::VRInitError_None;
    vr::VR_Init(&init_error, vr::VRApplication_Overlay);

    if (init_error != vr::VRInitError_None)
        return;

    //Init Direct3D
    if (!D3DManager::Get().CreateDevice())
        return;

    //Everything went through, set as initialized
    m_IsInitialized = true;
}

DPBrowserApp::~DPBrowserApp()
{
    D3DManager::Get().DestroyDevice();
    vr::VR_Shutdown();
}

void DPBrowserApp::RunMessageLoop()
{
    MSG msg = {0};

    //Handle window messages
    while (::GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message >= 0xC000)  //Custom message, handle in DPBrowserAPIServer
        {
            DPBrowserAPIServer::Get().HandleIPCMessage(msg);
        }
        else
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

bool DPBrowserApp::IsInitialized() const
{
    return m_IsInitialized;
}

void DPBrowserApp::OnBeforeCommandLineProcessing(CefString const& /*process_type*/, CefRefPtr<CefCommandLine> command_line)
{
    //Set default switches
    command_line->AppendSwitch("force-dark-mode");                  //Forces dark mode on chromium pages. Doesn't force it on normal web pages or scroll bars
    command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");

    //Disable media key handling
    if (command_line->HasSwitch("disable-features")) 
    {
        //Don't override existing values
        std::string disable_features = command_line->GetSwitchValue("disable-features");
        command_line->AppendSwitchWithValue("disable-features", disable_features + ",HardwareMediaKeyHandling");
    } 
    else 
    {
        command_line->AppendSwitchWithValue("disable-features", "HardwareMediaKeyHandling");
    }
}

void DPBrowserApp::OnContextInitialized()
{
    CEF_REQUIRE_UI_THREAD();

    //Create DPBrowserHandler singleton
    //DPBrowserHandler manages all browsers + their callbacks and implements the DPBrowserAPI interface called by DPBrowserAPIServer
    CefRefPtr<DPBrowserHandler> handler(new DPBrowserHandler());
}
