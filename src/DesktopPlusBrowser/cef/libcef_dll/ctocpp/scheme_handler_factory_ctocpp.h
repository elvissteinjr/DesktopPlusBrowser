// Copyright (c) 2024 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//
// $hash=80d4f216d16784b9ee485be512196700797a91f4$
//

#ifndef CEF_LIBCEF_DLL_CTOCPP_SCHEME_HANDLER_FACTORY_CTOCPP_H_
#define CEF_LIBCEF_DLL_CTOCPP_SCHEME_HANDLER_FACTORY_CTOCPP_H_
#pragma once

#if !defined(BUILDING_CEF_SHARED)
#error This file can be included DLL-side only
#endif

#include "include/capi/cef_scheme_capi.h"
#include "include/cef_scheme.h"
#include "libcef_dll/ctocpp/ctocpp_ref_counted.h"

// Wrap a C structure with a C++ class.
// This class may be instantiated and accessed DLL-side only.
class CefSchemeHandlerFactoryCToCpp
    : public CefCToCppRefCounted<CefSchemeHandlerFactoryCToCpp,
                                 CefSchemeHandlerFactory,
                                 cef_scheme_handler_factory_t> {
 public:
  CefSchemeHandlerFactoryCToCpp();
  virtual ~CefSchemeHandlerFactoryCToCpp();

  // CefSchemeHandlerFactory methods.
  CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame,
                                       const CefString& scheme_name,
                                       CefRefPtr<CefRequest> request) override;
};

#endif  // CEF_LIBCEF_DLL_CTOCPP_SCHEME_HANDLER_FACTORY_CTOCPP_H_
