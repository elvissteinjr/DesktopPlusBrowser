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
// $hash=7f28d1c7c51271082e987d17748b94ef8f4bf9d6$
//

#include "libcef_dll/ctocpp/test/translator_test_scoped_client_ctocpp.h"

#include "libcef_dll/ctocpp/test/translator_test_scoped_client_child_ctocpp.h"

// VIRTUAL METHODS - Body may be edited by hand.

NO_SANITIZE("cfi-icall") int CefTranslatorTestScopedClientCToCpp::GetValue() {
  cef_translator_test_scoped_client_t* _struct = GetStruct();
  if (CEF_MEMBER_MISSING(_struct, get_value)) {
    return 0;
  }

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  int _retval = _struct->get_value(_struct);

  // Return type: simple
  return _retval;
}

// CONSTRUCTOR - Do not edit by hand.

CefTranslatorTestScopedClientCToCpp::CefTranslatorTestScopedClientCToCpp() {}

// DESTRUCTOR - Do not edit by hand.

CefTranslatorTestScopedClientCToCpp::~CefTranslatorTestScopedClientCToCpp() {}

template <>
cef_translator_test_scoped_client_t*
CefCToCppScoped<CefTranslatorTestScopedClientCToCpp,
                CefTranslatorTestScopedClient,
                cef_translator_test_scoped_client_t>::
    UnwrapDerivedOwn(CefWrapperType type,
                     CefOwnPtr<CefTranslatorTestScopedClient> c) {
  if (type == WT_TRANSLATOR_TEST_SCOPED_CLIENT_CHILD) {
    return reinterpret_cast<cef_translator_test_scoped_client_t*>(
        CefTranslatorTestScopedClientChildCToCpp::UnwrapOwn(
            CefOwnPtr<CefTranslatorTestScopedClientChild>(
                reinterpret_cast<CefTranslatorTestScopedClientChild*>(
                    c.release()))));
  }
  DCHECK(false) << "Unexpected class type: " << type;
  return nullptr;
}

template <>
cef_translator_test_scoped_client_t*
CefCToCppScoped<CefTranslatorTestScopedClientCToCpp,
                CefTranslatorTestScopedClient,
                cef_translator_test_scoped_client_t>::
    UnwrapDerivedRaw(CefWrapperType type,
                     CefRawPtr<CefTranslatorTestScopedClient> c) {
  if (type == WT_TRANSLATOR_TEST_SCOPED_CLIENT_CHILD) {
    return reinterpret_cast<cef_translator_test_scoped_client_t*>(
        CefTranslatorTestScopedClientChildCToCpp::UnwrapRaw(
            CefRawPtr<CefTranslatorTestScopedClientChild>(
                reinterpret_cast<CefTranslatorTestScopedClientChild*>(c))));
  }
  DCHECK(false) << "Unexpected class type: " << type;
  return nullptr;
}

template <>
CefWrapperType
    CefCToCppScoped<CefTranslatorTestScopedClientCToCpp,
                    CefTranslatorTestScopedClient,
                    cef_translator_test_scoped_client_t>::kWrapperType =
        WT_TRANSLATOR_TEST_SCOPED_CLIENT;
