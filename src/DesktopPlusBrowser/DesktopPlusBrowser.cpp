//This code belongs to the Desktop+ Browser component, licensed under GPL 3.0

#include "include/cef_command_line.h"
#include "DPBrowserApp.h"
#include "DPBrowserAPIServer.h"
#include "Util.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
{
    //Have CEF parse the command-line arguments for us
    CefMainArgs main_args(hInstance);

    //Run the CEF sub-process if the command-line says it should be one (CefExecuteProcess() returns -1 if it's the main process)
    int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);

    if (exit_code != -1)
    {
        return exit_code;   //Sub-process execution ended, exit
    }

    //Make sure only one main instance is running
    StopProcessByWindowClass(g_WindowClassNameBrowserApp);

    //Set global CEF settings
    CefSettings settings;
    settings.windowless_rendering_enabled = true;
    settings.no_sandbox = true;
    settings.multi_threaded_message_loop = true;

    //Get working directory and use it for the cache path
    {
        std::unique_ptr<WCHAR[]> buffer;
        DWORD buffer_size = ::GetCurrentDirectoryW(0, nullptr);

        if (buffer_size > 0)
        {
            buffer = std::unique_ptr<WCHAR[]>{ new WCHAR[buffer_size] };

            if (::GetCurrentDirectoryW(buffer_size, buffer.get()) != 0)
            {
                //Append cache directory name and set the path
                std::wstring str_current_dir(buffer.get());
                str_current_dir += L"/userdata/";

                CefString(&settings.cache_path) = str_current_dir;
            }
        }
    }
    
    //DPBrowserApp handles application-level CEF callbacks and forwards window messages to the DPBrowserAPIServer
    CefRefPtr<DPBrowserApp> app(new DPBrowserApp);

    //Init CEF
    bool is_cef_ready = CefInitialize(main_args, settings, app.get(), nullptr);

    //Check if everything worked so far
    if ((!is_cef_ready) || (!app->IsInitialized()))
    {
        vr::VR_Shutdown();

        return -1;  //Something went wrong, exit
    }

    //Init DPBrowserAPIServer
    DPBrowserAPIServer::Get().Init(hInstance, app);

    //Run app message loop. This will only return once it's time to exit
    app->RunMessageLoop();

    //Shut down everything
    CefShutdown();
    app.reset();

    return 0;
}
