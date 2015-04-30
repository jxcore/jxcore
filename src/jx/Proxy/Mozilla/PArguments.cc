// Copyright & License details are available under JXCORE_LICENSE file

#include "PArguments.h"
#include <limits>
#include <assert.h>
namespace jxcore {

PArguments::PArguments(JSContext *ctx, int argc, JS::Value *__jsval) {
  close_called_ = false;
  ret_val_ = true;

  ctx_ = ctx;
  argc_ = argc;
  jsval_ = __jsval;
  args_ = __jsval + 2;
}

JS_HANDLE_OBJECT PArguments::This() {
  return MozJS::Value(JS_THIS(ctx_, jsval_), ctx_);
}

void *PArguments::GetHolder() {
  MozJS::Value val(JS_THIS(ctx_, jsval_), ctx_);

  return val.GetPointerFromInternalField(0);
}

bool PArguments::IsConstructCall() {
  if ((args_ - 2)->isObjectOrNull())
    return (JS_ObjectIsFunction(ctx_, (args_ - 2)->toObjectOrNull()));

  return false;
}

int PArguments::GetUTF8Length(const unsigned index) {
  if (argc_ <= index) {
    return 0;
  }

  if (!args_[index].isString()) return 0;

  return static_cast<int>(
      JS_GetStringUTF8Length(ctx_, args_[index].toString()));
}

bool PArguments::IsInteger(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isInt32()) {
    return false;
  }

  return true;
}

bool PArguments::IsDate(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isObject()) return false;

  JS::RootedObject obj(ctx_);
  JS::RootedValue rt_value_(ctx_, val);
  if (!JS_ValueToObject(ctx_, rt_value_, &obj)) return false;
  return JS_ObjectIsDate(ctx_, obj);
}

bool PArguments::IsRegExp(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isObject()) {
    return false;
  }

  JSObject *obj = val.toObjectOrNull();
  if (obj == nullptr) return false;

  JS::RootedObject rt_obj(ctx_, obj);
  return JS_ObjectIsRegExp(ctx_, rt_obj);
}

bool PArguments::IsObject(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];

  if (val.isNullOrUndefined()) return false;
  return val.isObjectOrNull();
}

bool PArguments::IsArray(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (!val.isObjectOrNull()) return false;

  JS::RootedObject ro_val(ctx_, val.toObjectOrNull());
  if (val.isNullOrUndefined() || !JS_IsArrayObject(ctx_, ro_val)) {
    return false;
  }

  return true;
}

bool PArguments::IsNumber(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isNumber()) {
    return false;
  }

  return true;
}

JS_HANDLE_VALUE PArguments::GetItem(const unsigned index) const {
  if (argc_ <= index) {
    return JS_HANDLE_VALUE::Null(ctx_);
  } else {
    JS::Value val = args_[index];
    return JS_HANDLE_VALUE(val, ctx_);
  }
}

JS_HANDLE_FUNCTION PArguments::GetAsFunction(const unsigned index) const {
  if (argc_ <= index) {
    JS::Value val;
    val.setUndefined();
    return JS_HANDLE_OBJECT(val, ctx_);
  } else {
    JS::Value val = args_[index];
    return JS_HANDLE_OBJECT(val, ctx_);
  }
}

JS_HANDLE_OBJECT PArguments::GetAsArray(const unsigned index) const {
  if (argc_ <= index) {
    JS::Value val;
    val.setUndefined();
    return JS_HANDLE_OBJECT(val, ctx_);
  } else {
    JS::Value val = args_[index];
    return JS_HANDLE_OBJECT(val, ctx_);
  }
}

bool PArguments::IsFunction(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isObject()) {
    return false;
  }

  return JS_ObjectIsFunction(ctx_, val.toObjectOrNull());
}

bool PArguments::IsBoolean(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isBoolean()) {
    return false;
  }

  return true;
}

bool PArguments::IsString(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isString()) {
    return false;
  }

  return true;
}

bool PArguments::IsUnsigned(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  jsval val = args_[index];
  if (val.isNullOrUndefined() || !val.isNumber()) {
    return false;
  }

  int64_t xv = val.toNumber();

  // TODO(obastemur) find an efficient way
#ifdef JS_CPU_ARM
  if (xv == 0)
    return val.toInt32() >= 0;
#endif

  return xv >= 0;
}

bool PArguments::IsBooleanOrNull(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  return args_[index].isNull() || args_[index].isBoolean();
}

bool PArguments::IsStringOrNull(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  return args_[index].isNull() || args_[index].isString();
}

bool PArguments::IsNull(const unsigned index) const {
  if (argc_ <= index) {
    return false;
  }

  return args_[index].isNull();
}

bool PArguments::IsUndefined(const unsigned index) const {
  if (argc_ <= index) {
    return true;
  }

  return args_[index].isUndefined();
}

bool PArguments::GetBoolean(const unsigned index) {
  if (index >= argc_) {
    return 0;
  }

  if (args_[index].isBoolean())
    return args_[index].toBoolean();
  else if (args_[index].isNumber())
    return args_[index].toInt32() != 0;

  return !(args_[index].isNullOrUndefined());
}

int64_t PArguments::GetInteger(const unsigned index) {
  if (index >= argc_) {
    return 0;
  }

  if (args_[index].isNumber()) {
	int64_t xv = (int64_t)args_[index].toNumber();
#ifdef JS_CPU_ARM
	if (xv == 0)
	  xv = args_[index].toInt32();
#endif
	return xv;
  }

  if (args_[index].isBoolean()) return args_[index].toBoolean() ? 1 : 0;

  if (args_[index].isString()) {
    JXString str;
    GetString(index, &str);
    return atol(*str);
  }

  if (args_[index].isNullOrUndefined()) return 0;

  return std::numeric_limits<int32_t>::quiet_NaN();
}

int32_t PArguments::GetInt32(const unsigned index) { return GetInteger(index); }

double PArguments::GetNumber(const unsigned index) {
  if (index >= argc_) {
    return 0;
  }

  if (args_[index].isNumber()) return args_[index].toNumber();

  if (args_[index].isBoolean()) return args_[index].toBoolean() ? 1 : 0;

  if (args_[index].isString()) {
    JXString str;
    GetString(index, &str);
    return atof(*str);
  }

  return std::numeric_limits<double>::quiet_NaN();
}

JS_HANDLE_STRING PArguments::GetAsString(const unsigned index) const {
  if (index >= argc_) {
    return MozJS::String();
  }

  return MozJS::String(args_[index], ctx_);
}

unsigned PArguments::GetUInteger(const unsigned index) {
  if (index >= argc_) {
    return 0;
  }

  if (args_[index].isNumber()) {
    int64_t val64 = (int64_t)args_[index].toNumber();
#ifdef JS_CPU_ARM
	if (val64 == 0)
	  val64 = -1;
#endif
    if (val64 >= 0) return (uint32_t)val64;

    return args_[index].toPrivateUint32();
  } else {
    return (unsigned)GetInteger(index);
  }
}

int PArguments::GetString(const unsigned index, jxcore::JXString *jxs) {
  if (index >= argc_) {
    jxs->SetFromSTD("", ctx_);
    return 0;
  }

  if (!args_[index].isNullOrUndefined()) {
    JS::RootedValue harg(ctx_, args_[index]);
    JSString *str = JS::ToString(ctx_, harg);
    jxs->SetFromHandle(str, ctx_);
  } else {
    if (args_[index].isUndefined())
      jxs->SetFromSTD("undefined", ctx_);
    else
      jxs->SetFromSTD("null", ctx_);
  }

  return jxs->Utf8Length();
}

void PArguments::close(MozJS::Value val) { close(&val); }

void PArguments::close(MozJS::Value *val) {
  close_called_ = true;

  if (val->is_exception_) {
    MozJS::ThrowException(*val);
    ret_val_ = false;
  } else {
    *jsval_ = val->GetRawValue();
    if (JS_IsExceptionPending(ctx_)) ret_val_ = false;
  }
}

void PArguments::close(JS::Value val) {
  close_called_ = true;

  *jsval_ = val;
}

void PArguments::close(MozJS::ThrowException ex) {
  assert(!close_called_);
  close_called_ = true;

  if (ret_val_)
    if (JS_IsExceptionPending(ctx_)) ret_val_ = false;
}

void PArguments::close(bool ret_val) {
  ret_val_ = ret_val;
  if (ret_val_)
    if (JS_IsExceptionPending(ctx_)) ret_val_ = false;
}

void PArguments::close() {
  if (!close_called_) {
    close_called_ = true;
    *jsval_ = JSVAL_VOID;
    if (ret_val_)
      if (JS_IsExceptionPending(ctx_)) ret_val_ = false;
  }
}
}  // namespace jxcore
