#include "DPBrowserContentBlocker.h"

#include "include/cef_parser.h"
#include "include/cef_path_util.h"
#include "include/wrapper/cef_helpers.h"

#include "DPBrowserAPIServer.h"

#include <fstream>

void DPBrowserContentBlocker::ReloadEngines()
{
    CEF_REQUIRE_IO_THREAD();

    m_AdblockEngines.clear();

    //Find all list files in the "content_block" directory and load them
    CefString exe_dir;
    CefGetPath(PK_DIR_EXE, exe_dir);

    std::wstring wpath = exe_dir.c_str();
    wpath += L"/content_block/";

    WIN32_FIND_DATA find_data;
    HANDLE handle_find = ::FindFirstFileW( (wpath + L"*.txt").c_str(), &find_data);

    if (handle_find != INVALID_HANDLE_VALUE)
    {
        do
        {
            //Map file into memory and create an Adblock Engine from it
            HANDLE handle_file = ::CreateFileW((wpath + find_data.cFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

            if (handle_file != INVALID_HANDLE_VALUE)
            {
                HANDLE handle_file_mapping = ::CreateFileMappingW(handle_file, nullptr, PAGE_READONLY, 0, 0, nullptr);

                if (handle_file_mapping != nullptr)
                {
                    void* ptr_file = ::MapViewOfFile(handle_file_mapping, FILE_MAP_READ, 0, 0, 0);

                    if (ptr_file != nullptr)
                    {
                        MEMORY_BASIC_INFORMATION mem_info = {0};

                        if (::VirtualQuery(ptr_file, &mem_info, sizeof(mem_info)) != 0)
                        {
                            //There's no feedback on validation or way to check if the list was loaded, so we just push new engines into the vector and hope for the best
                            m_AdblockEngines.push_back( std::make_unique<adblock::Engine>((const char*)ptr_file, mem_info.RegionSize) );
                        }

                        ::UnmapViewOfFile(ptr_file);
                    }

                    ::CloseHandle(handle_file_mapping);
                }

                ::CloseHandle(handle_file);
            }
        }
        while (::FindNextFileW(handle_find, &find_data) != 0);

        ::FindClose(handle_find);
    }

    //Notify about loaded list count
    DPBrowserAPIServer::Get().NotifyContentBlockListCount((int)m_AdblockEngines.size());
}

void DPBrowserContentBlocker::SetEnabled(bool enabled)
{
    CEF_REQUIRE_IO_THREAD();

    m_IsEnabled = enabled;

    //Just always reload engines when enabling for now
    if (m_IsEnabled)
    {
        ReloadEngines();
    }
}

bool DPBrowserContentBlocker::GetEnabled() const
{
    CEF_REQUIRE_IO_THREAD();

    return m_IsEnabled;
}

CefResourceRequestHandler::ReturnValue DPBrowserContentBlocker::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback)
{
    CEF_REQUIRE_IO_THREAD();

    if ((!m_IsEnabled) || (m_AdblockEngines.empty()))
        return RV_CONTINUE;

    //Get host names from URLs
    CefURLParts url_parts;
    std::string host, tab_host, request_host;

    if ( (frame != nullptr) && (!frame->GetURL().empty()) )     //CefParseURL() is supposed to just return false on empty string but asserts before that in debug builds. Weird design.
    {
        CefParseURL(frame->GetURL(), url_parts);
        host = CefStringUTF8(url_parts.host.str, url_parts.host.length);
    }

    if ( (browser != nullptr) && (!browser->GetMainFrame()->GetURL().empty()) )
    {
        CefParseURL(browser->GetMainFrame()->GetURL(), url_parts);
        tab_host = CefStringUTF8(url_parts.host.str, url_parts.host.length);
    }

    if ( (browser != nullptr) && (!request->GetURL().empty()) )
    {
        CefParseURL(request->GetURL(), url_parts);
        request_host = CefStringUTF8(url_parts.host.str, url_parts.host.length);
    }

    const bool is_third_party = ((!tab_host.empty()) && (tab_host != request_host));

    //Convert resource type value to string understood by the engine
    std::string resource_type_str;

    switch (request->GetResourceType())
    {
        case RT_MAIN_FRAME:    resource_type_str = "main_frame";  break;
        case RT_SUB_FRAME:     resource_type_str = "sub_frame";   break;
        case RT_STYLESHEET:    resource_type_str = "stylesheet";  break;
        case RT_SCRIPT:        resource_type_str = "script";      break;
        case RT_IMAGE:         resource_type_str = "image";       break;
        case RT_FONT_RESOURCE: resource_type_str = "font";        break;
        case RT_OBJECT:        resource_type_str = "object";      break;
        case RT_MEDIA:         resource_type_str = "media";       break;
        case RT_FAVICON:       resource_type_str = "image";       break;
        case RT_XHR:           resource_type_str = "xhr";         break;
        case RT_PING:          resource_type_str = "beacon";      break;
        case RT_CSP_REPORT:    resource_type_str = "csp_report";  break;
        default:               resource_type_str = "other";
    }

    bool did_match_rule = false, did_match_exception = false, did_match_important = false;
    std::string redirect;

    //Check with every loaded engine (results of matches() are cumulative)
    for (auto& engine : m_AdblockEngines)
    {
        engine->matches(request->GetURL(), host, tab_host, is_third_party, resource_type_str, &did_match_rule, &did_match_exception, &did_match_important, &redirect);
    }

    //Set redirect if there's any
    if (!redirect.empty())
    {
        VLOG(1) << "Redirected URL: \"" << request->GetURL() << "\" -> \"" << redirect << "\"";

        request->SetURL(redirect);
        return RV_CONTINUE;
    }
    else if ((did_match_rule) && (!did_match_exception))    //Block if there's no exception match
    {
        VLOG(1) << "Blocked URL: \"" << request->GetURL() << "\"";
        return RV_CANCEL;
    }

    return RV_CONTINUE;
}
