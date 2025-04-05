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
// $hash=aadd98d24c5a0e57a832fd4c6334a71e7723996f$
//

#include "libcef_dll/ctocpp/download_image_callback_ctocpp.h"

#include "libcef_dll/cpptoc/image_cpptoc.h"
#include "libcef_dll/shutdown_checker.h"

// VIRTUAL METHODS - Body may be edited by hand.

NO_SANITIZE("cfi-icall")
void CefDownloadImageCallbackCToCpp::OnDownloadImageFinished(
    const CefString& image_url,
    int http_status_code,
    CefRefPtr<CefImage> image) {
  shutdown_checker::AssertNotShutdown();

  cef_download_image_callback_t* _struct = GetStruct();
  if (CEF_MEMBER_MISSING(_struct, on_download_image_finished)) {
    return;
  }

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: image_url; type: string_byref_const
  DCHECK(!image_url.empty());
  if (image_url.empty()) {
    return;
  }
  // Unverified params: image

  // Execute
  _struct->on_download_image_finished(_struct, image_url.GetStruct(),
                                      http_status_code,
                                      CefImageCppToC::Wrap(image));
}

// CONSTRUCTOR - Do not edit by hand.

CefDownloadImageCallbackCToCpp::CefDownloadImageCallbackCToCpp() {}

// DESTRUCTOR - Do not edit by hand.

CefDownloadImageCallbackCToCpp::~CefDownloadImageCallbackCToCpp() {
  shutdown_checker::AssertNotShutdown();
}

template <>
cef_download_image_callback_t* CefCToCppRefCounted<
    CefDownloadImageCallbackCToCpp,
    CefDownloadImageCallback,
    cef_download_image_callback_t>::UnwrapDerived(CefWrapperType type,
                                                  CefDownloadImageCallback* c) {
  DCHECK(false) << "Unexpected class type: " << type;
  return nullptr;
}

template <>
CefWrapperType
    CefCToCppRefCounted<CefDownloadImageCallbackCToCpp,
                        CefDownloadImageCallback,
                        cef_download_image_callback_t>::kWrapperType =
        WT_DOWNLOAD_IMAGE_CALLBACK;
