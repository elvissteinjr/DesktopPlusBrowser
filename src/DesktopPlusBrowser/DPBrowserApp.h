#pragma once

#include "include/cef_app.h"
#include "include/cef_client.h"

class DPBrowserApp : public CefApp, public CefBrowserProcessHandler
{    
    private:
        IMPLEMENT_REFCOUNTING(DPBrowserApp);

        bool m_IsInitialized = false;

    public:
        DPBrowserApp();
        virtual ~DPBrowserApp();

        void RunMessageLoop();
        bool IsInitialized() const;

        //CefApp:
        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
        void OnBeforeCommandLineProcessing(CefString const& process_type, CefRefPtr<CefCommandLine> command_line) override;

        //CefBrowserProcessHandler:
        void OnContextInitialized() override;
};
