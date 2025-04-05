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
// $hash=c59808566c2a9f2d204b6724bb5a905aeb0e7620$
//

#include "libcef_dll/cpptoc/v8value_cpptoc.h"

#include "libcef_dll/cpptoc/v8context_cpptoc.h"
#include "libcef_dll/cpptoc/v8exception_cpptoc.h"
#include "libcef_dll/ctocpp/base_ref_counted_ctocpp.h"
#include "libcef_dll/ctocpp/v8accessor_ctocpp.h"
#include "libcef_dll/ctocpp/v8array_buffer_release_callback_ctocpp.h"
#include "libcef_dll/ctocpp/v8handler_ctocpp.h"
#include "libcef_dll/ctocpp/v8interceptor_ctocpp.h"
#include "libcef_dll/transfer_util.h"

// GLOBAL FUNCTIONS - Body may be edited by hand.

CEF_EXPORT cef_v8value_t* cef_v8value_create_undefined() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateUndefined();

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_null() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateNull();

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_bool(int value) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateBool(value ? true : false);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_int(int32_t value) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateInt(value);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_uint(uint32_t value) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateUInt(value);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_double(double value) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateDouble(value);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_date(cef_basetime_t date) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateDate(date);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_string(const cef_string_t* value) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Unverified params: value

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateString(CefString(value));

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_object(
    cef_v8accessor_t* accessor,
    cef_v8interceptor_t* interceptor) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Unverified params: accessor, interceptor

  // Execute
  CefRefPtr<CefV8Value> _retval =
      CefV8Value::CreateObject(CefV8AccessorCToCpp::Wrap(accessor),
                               CefV8InterceptorCToCpp::Wrap(interceptor));

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_array(int length) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateArray(length);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_array_buffer(
    void* buffer,
    size_t length,
    cef_v8array_buffer_release_callback_t* release_callback) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: release_callback; type: refptr_diff
  DCHECK(release_callback);
  if (!release_callback) {
    return NULL;
  }
  // Unverified params: buffer

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateArrayBuffer(
      buffer, length,
      CefV8ArrayBufferReleaseCallbackCToCpp::Wrap(release_callback));

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_function(
    const cef_string_t* name,
    cef_v8handler_t* handler) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: name; type: string_byref_const
  DCHECK(name);
  if (!name) {
    return NULL;
  }
  // Verify param: handler; type: refptr_diff
  DCHECK(handler);
  if (!handler) {
    return NULL;
  }

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreateFunction(
      CefString(name), CefV8HandlerCToCpp::Wrap(handler));

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

CEF_EXPORT cef_v8value_t* cef_v8value_create_promise() {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8Value::CreatePromise();

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

namespace {

// MEMBER FUNCTIONS - Body may be edited by hand.

int CEF_CALLBACK v8value_is_valid(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsValid();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_undefined(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsUndefined();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_null(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsNull();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_bool(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsBool();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_int(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsInt();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_uint(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsUInt();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_double(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsDouble();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_date(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsDate();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_string(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsString();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_object(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsObject();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_array(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsArray();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_array_buffer(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsArrayBuffer();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_function(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsFunction();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_promise(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsPromise();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_is_same(struct _cef_v8value_t* self,
                                 struct _cef_v8value_t* that) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Verify param: that; type: refptr_same
  DCHECK(that);
  if (!that) {
    return 0;
  }

  // Execute
  bool _retval =
      CefV8ValueCppToC::Get(self)->IsSame(CefV8ValueCppToC::Unwrap(that));

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_get_bool_value(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->GetBoolValue();

  // Return type: bool
  return _retval;
}

int32_t CEF_CALLBACK v8value_get_int_value(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  int32_t _retval = CefV8ValueCppToC::Get(self)->GetIntValue();

  // Return type: simple
  return _retval;
}

uint32_t CEF_CALLBACK v8value_get_uint_value(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  uint32_t _retval = CefV8ValueCppToC::Get(self)->GetUIntValue();

  // Return type: simple
  return _retval;
}

double CEF_CALLBACK v8value_get_double_value(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  double _retval = CefV8ValueCppToC::Get(self)->GetDoubleValue();

  // Return type: simple
  return _retval;
}

cef_basetime_t CEF_CALLBACK
v8value_get_date_value(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return CefBaseTime();
  }

  // Execute
  cef_basetime_t _retval = CefV8ValueCppToC::Get(self)->GetDateValue();

  // Return type: simple
  return _retval;
}

cef_string_userfree_t CEF_CALLBACK
v8value_get_string_value(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }

  // Execute
  CefString _retval = CefV8ValueCppToC::Get(self)->GetStringValue();

  // Return type: string
  return _retval.DetachToUserFree();
}

int CEF_CALLBACK v8value_is_user_created(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->IsUserCreated();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_has_exception(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->HasException();

  // Return type: bool
  return _retval;
}

cef_v8exception_t* CEF_CALLBACK
v8value_get_exception(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }

  // Execute
  CefRefPtr<CefV8Exception> _retval =
      CefV8ValueCppToC::Get(self)->GetException();

  // Return type: refptr_same
  return CefV8ExceptionCppToC::Wrap(_retval);
}

int CEF_CALLBACK v8value_clear_exception(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->ClearException();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_will_rethrow_exceptions(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->WillRethrowExceptions();

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_set_rethrow_exceptions(struct _cef_v8value_t* self,
                                                int rethrow) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval =
      CefV8ValueCppToC::Get(self)->SetRethrowExceptions(rethrow ? true : false);

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_has_value_bykey(struct _cef_v8value_t* self,
                                         const cef_string_t* key) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Unverified params: key

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->HasValue(CefString(key));

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_has_value_byindex(struct _cef_v8value_t* self,
                                           int index) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Verify param: index; type: simple_byval
  DCHECK_GE(index, 0);
  if (index < 0) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->HasValue(index);

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_delete_value_bykey(struct _cef_v8value_t* self,
                                            const cef_string_t* key) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Unverified params: key

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->DeleteValue(CefString(key));

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_delete_value_byindex(struct _cef_v8value_t* self,
                                              int index) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Verify param: index; type: simple_byval
  DCHECK_GE(index, 0);
  if (index < 0) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->DeleteValue(index);

  // Return type: bool
  return _retval;
}

struct _cef_v8value_t* CEF_CALLBACK
v8value_get_value_bykey(struct _cef_v8value_t* self, const cef_string_t* key) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }
  // Unverified params: key

  // Execute
  CefRefPtr<CefV8Value> _retval =
      CefV8ValueCppToC::Get(self)->GetValue(CefString(key));

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

struct _cef_v8value_t* CEF_CALLBACK
v8value_get_value_byindex(struct _cef_v8value_t* self, int index) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }
  // Verify param: index; type: simple_byval
  DCHECK_GE(index, 0);
  if (index < 0) {
    return NULL;
  }

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8ValueCppToC::Get(self)->GetValue(index);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

int CEF_CALLBACK v8value_set_value_bykey(struct _cef_v8value_t* self,
                                         const cef_string_t* key,
                                         struct _cef_v8value_t* value,
                                         cef_v8_propertyattribute_t attribute) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Verify param: value; type: refptr_same
  DCHECK(value);
  if (!value) {
    return 0;
  }
  // Unverified params: key

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->SetValue(
      CefString(key), CefV8ValueCppToC::Unwrap(value), attribute);

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_set_value_byindex(struct _cef_v8value_t* self,
                                           int index,
                                           struct _cef_v8value_t* value) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Verify param: index; type: simple_byval
  DCHECK_GE(index, 0);
  if (index < 0) {
    return 0;
  }
  // Verify param: value; type: refptr_same
  DCHECK(value);
  if (!value) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->SetValue(
      index, CefV8ValueCppToC::Unwrap(value));

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK
v8value_set_value_byaccessor(struct _cef_v8value_t* self,
                             const cef_string_t* key,
                             cef_v8_propertyattribute_t attribute) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Unverified params: key

  // Execute
  bool _retval =
      CefV8ValueCppToC::Get(self)->SetValue(CefString(key), attribute);

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_get_keys(struct _cef_v8value_t* self,
                                  cef_string_list_t keys) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Verify param: keys; type: string_vec_byref
  DCHECK(keys);
  if (!keys) {
    return 0;
  }

  // Translate param: keys; type: string_vec_byref
  std::vector<CefString> keysList;
  transfer_string_list_contents(keys, keysList);

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->GetKeys(keysList);

  // Restore param: keys; type: string_vec_byref
  cef_string_list_clear(keys);
  transfer_string_list_contents(keysList, keys);

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_set_user_data(struct _cef_v8value_t* self,
                                       cef_base_ref_counted_t* user_data) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Unverified params: user_data

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->SetUserData(
      CefBaseRefCountedCToCpp::Wrap(user_data));

  // Return type: bool
  return _retval;
}

cef_base_ref_counted_t* CEF_CALLBACK
v8value_get_user_data(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }

  // Execute
  CefRefPtr<CefBaseRefCounted> _retval =
      CefV8ValueCppToC::Get(self)->GetUserData();

  // Return type: refptr_diff
  return CefBaseRefCountedCToCpp::Unwrap(_retval);
}

int CEF_CALLBACK
v8value_get_externally_allocated_memory(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  int _retval = CefV8ValueCppToC::Get(self)->GetExternallyAllocatedMemory();

  // Return type: simple
  return _retval;
}

int CEF_CALLBACK
v8value_adjust_externally_allocated_memory(struct _cef_v8value_t* self,
                                           int change_in_bytes) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  int _retval = CefV8ValueCppToC::Get(self)->AdjustExternallyAllocatedMemory(
      change_in_bytes);

  // Return type: simple
  return _retval;
}

int CEF_CALLBACK v8value_get_array_length(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  int _retval = CefV8ValueCppToC::Get(self)->GetArrayLength();

  // Return type: simple
  return _retval;
}

cef_v8array_buffer_release_callback_t* CEF_CALLBACK
v8value_get_array_buffer_release_callback(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }

  // Execute
  CefRefPtr<CefV8ArrayBufferReleaseCallback> _retval =
      CefV8ValueCppToC::Get(self)->GetArrayBufferReleaseCallback();

  // Return type: refptr_diff
  return CefV8ArrayBufferReleaseCallbackCToCpp::Unwrap(_retval);
}

int CEF_CALLBACK v8value_neuter_array_buffer(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->NeuterArrayBuffer();

  // Return type: bool
  return _retval;
}

size_t CEF_CALLBACK
v8value_get_array_buffer_byte_length(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }

  // Execute
  size_t _retval = CefV8ValueCppToC::Get(self)->GetArrayBufferByteLength();

  // Return type: simple
  return _retval;
}

void* CEF_CALLBACK v8value_get_array_buffer_data(struct _cef_v8value_t* self) {
  DCHECK(self);
  if (!self) {
    return NULL;
  }

  // This manual implementation can be removed once support for 'void*'
  // is integrated into the CEF translator tool (issue #3591).

  // Execute
  void* _retval = CefV8ValueCppToC::Get(self)->GetArrayBufferData();

  // Return type: simple_byaddr
  return _retval;
}

cef_string_userfree_t CEF_CALLBACK
v8value_get_function_name(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }

  // Execute
  CefString _retval = CefV8ValueCppToC::Get(self)->GetFunctionName();

  // Return type: string
  return _retval.DetachToUserFree();
}

cef_v8handler_t* CEF_CALLBACK
v8value_get_function_handler(struct _cef_v8value_t* self) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }

  // Execute
  CefRefPtr<CefV8Handler> _retval =
      CefV8ValueCppToC::Get(self)->GetFunctionHandler();

  // Return type: refptr_diff
  return CefV8HandlerCToCpp::Unwrap(_retval);
}

struct _cef_v8value_t* CEF_CALLBACK
v8value_execute_function(struct _cef_v8value_t* self,
                         struct _cef_v8value_t* object,
                         size_t argumentsCount,
                         struct _cef_v8value_t* const* arguments) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }
  // Verify param: arguments; type: refptr_vec_same_byref_const
  DCHECK(argumentsCount == 0 || arguments);
  if (argumentsCount > 0 && !arguments) {
    return NULL;
  }
  // Unverified params: object

  // Translate param: arguments; type: refptr_vec_same_byref_const
  std::vector<CefRefPtr<CefV8Value>> argumentsList;
  if (argumentsCount > 0) {
    for (size_t i = 0; i < argumentsCount; ++i) {
      CefRefPtr<CefV8Value> argumentsVal =
          CefV8ValueCppToC::Unwrap(arguments[i]);
      argumentsList.push_back(argumentsVal);
    }
  }

  // Execute
  CefRefPtr<CefV8Value> _retval = CefV8ValueCppToC::Get(self)->ExecuteFunction(
      CefV8ValueCppToC::Unwrap(object), argumentsList);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

struct _cef_v8value_t* CEF_CALLBACK
v8value_execute_function_with_context(struct _cef_v8value_t* self,
                                      cef_v8context_t* context,
                                      struct _cef_v8value_t* object,
                                      size_t argumentsCount,
                                      struct _cef_v8value_t* const* arguments) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return NULL;
  }
  // Verify param: context; type: refptr_same
  DCHECK(context);
  if (!context) {
    return NULL;
  }
  // Verify param: arguments; type: refptr_vec_same_byref_const
  DCHECK(argumentsCount == 0 || arguments);
  if (argumentsCount > 0 && !arguments) {
    return NULL;
  }
  // Unverified params: object

  // Translate param: arguments; type: refptr_vec_same_byref_const
  std::vector<CefRefPtr<CefV8Value>> argumentsList;
  if (argumentsCount > 0) {
    for (size_t i = 0; i < argumentsCount; ++i) {
      CefRefPtr<CefV8Value> argumentsVal =
          CefV8ValueCppToC::Unwrap(arguments[i]);
      argumentsList.push_back(argumentsVal);
    }
  }

  // Execute
  CefRefPtr<CefV8Value> _retval =
      CefV8ValueCppToC::Get(self)->ExecuteFunctionWithContext(
          CefV8ContextCppToC::Unwrap(context), CefV8ValueCppToC::Unwrap(object),
          argumentsList);

  // Return type: refptr_same
  return CefV8ValueCppToC::Wrap(_retval);
}

int CEF_CALLBACK v8value_resolve_promise(struct _cef_v8value_t* self,
                                         struct _cef_v8value_t* arg) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Unverified params: arg

  // Execute
  bool _retval = CefV8ValueCppToC::Get(self)->ResolvePromise(
      CefV8ValueCppToC::Unwrap(arg));

  // Return type: bool
  return _retval;
}

int CEF_CALLBACK v8value_reject_promise(struct _cef_v8value_t* self,
                                        const cef_string_t* errorMsg) {
  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self) {
    return 0;
  }
  // Verify param: errorMsg; type: string_byref_const
  DCHECK(errorMsg);
  if (!errorMsg) {
    return 0;
  }

  // Execute
  bool _retval =
      CefV8ValueCppToC::Get(self)->RejectPromise(CefString(errorMsg));

  // Return type: bool
  return _retval;
}

}  // namespace

// CONSTRUCTOR - Do not edit by hand.

CefV8ValueCppToC::CefV8ValueCppToC() {
  GetStruct()->is_valid = v8value_is_valid;
  GetStruct()->is_undefined = v8value_is_undefined;
  GetStruct()->is_null = v8value_is_null;
  GetStruct()->is_bool = v8value_is_bool;
  GetStruct()->is_int = v8value_is_int;
  GetStruct()->is_uint = v8value_is_uint;
  GetStruct()->is_double = v8value_is_double;
  GetStruct()->is_date = v8value_is_date;
  GetStruct()->is_string = v8value_is_string;
  GetStruct()->is_object = v8value_is_object;
  GetStruct()->is_array = v8value_is_array;
  GetStruct()->is_array_buffer = v8value_is_array_buffer;
  GetStruct()->is_function = v8value_is_function;
  GetStruct()->is_promise = v8value_is_promise;
  GetStruct()->is_same = v8value_is_same;
  GetStruct()->get_bool_value = v8value_get_bool_value;
  GetStruct()->get_int_value = v8value_get_int_value;
  GetStruct()->get_uint_value = v8value_get_uint_value;
  GetStruct()->get_double_value = v8value_get_double_value;
  GetStruct()->get_date_value = v8value_get_date_value;
  GetStruct()->get_string_value = v8value_get_string_value;
  GetStruct()->is_user_created = v8value_is_user_created;
  GetStruct()->has_exception = v8value_has_exception;
  GetStruct()->get_exception = v8value_get_exception;
  GetStruct()->clear_exception = v8value_clear_exception;
  GetStruct()->will_rethrow_exceptions = v8value_will_rethrow_exceptions;
  GetStruct()->set_rethrow_exceptions = v8value_set_rethrow_exceptions;
  GetStruct()->has_value_bykey = v8value_has_value_bykey;
  GetStruct()->has_value_byindex = v8value_has_value_byindex;
  GetStruct()->delete_value_bykey = v8value_delete_value_bykey;
  GetStruct()->delete_value_byindex = v8value_delete_value_byindex;
  GetStruct()->get_value_bykey = v8value_get_value_bykey;
  GetStruct()->get_value_byindex = v8value_get_value_byindex;
  GetStruct()->set_value_bykey = v8value_set_value_bykey;
  GetStruct()->set_value_byindex = v8value_set_value_byindex;
  GetStruct()->set_value_byaccessor = v8value_set_value_byaccessor;
  GetStruct()->get_keys = v8value_get_keys;
  GetStruct()->set_user_data = v8value_set_user_data;
  GetStruct()->get_user_data = v8value_get_user_data;
  GetStruct()->get_externally_allocated_memory =
      v8value_get_externally_allocated_memory;
  GetStruct()->adjust_externally_allocated_memory =
      v8value_adjust_externally_allocated_memory;
  GetStruct()->get_array_length = v8value_get_array_length;
  GetStruct()->get_array_buffer_release_callback =
      v8value_get_array_buffer_release_callback;
  GetStruct()->neuter_array_buffer = v8value_neuter_array_buffer;
  GetStruct()->get_array_buffer_byte_length =
      v8value_get_array_buffer_byte_length;
  GetStruct()->get_array_buffer_data = v8value_get_array_buffer_data;
  GetStruct()->get_function_name = v8value_get_function_name;
  GetStruct()->get_function_handler = v8value_get_function_handler;
  GetStruct()->execute_function = v8value_execute_function;
  GetStruct()->execute_function_with_context =
      v8value_execute_function_with_context;
  GetStruct()->resolve_promise = v8value_resolve_promise;
  GetStruct()->reject_promise = v8value_reject_promise;
}

// DESTRUCTOR - Do not edit by hand.

CefV8ValueCppToC::~CefV8ValueCppToC() {}

template <>
CefRefPtr<CefV8Value>
CefCppToCRefCounted<CefV8ValueCppToC, CefV8Value, cef_v8value_t>::UnwrapDerived(
    CefWrapperType type,
    cef_v8value_t* s) {
  DCHECK(false) << "Unexpected class type: " << type;
  return nullptr;
}

template <>
CefWrapperType CefCppToCRefCounted<CefV8ValueCppToC,
                                   CefV8Value,
                                   cef_v8value_t>::kWrapperType = WT_V8VALUE;
