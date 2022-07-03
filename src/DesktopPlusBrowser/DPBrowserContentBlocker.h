#pragma once

#include "include/cef_client.h"
#include "adblock_rust_ffi/src/wrapper.h"

//Methods of this class need to be called on the IO thread
class DPBrowserContentBlocker : public CefResourceRequestHandler
{
    private:
        bool m_IsEnabled = false;
        std::vector< std::unique_ptr<adblock::Engine> > m_AdblockEngines;

        IMPLEMENT_REFCOUNTING(DPBrowserContentBlocker);

    public:
        void ReloadEngines();
        void SetEnabled(bool enabled);
        bool GetEnabled() const;

        //CefResourceRequestHandler:
        virtual CefResourceRequestHandler::ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
                                                                            CefRefPtr<CefCallback> callback) override;
};