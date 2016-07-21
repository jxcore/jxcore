Object.defineProperty = null;
var std_Array_indexOf = ArrayIndexOf;
var std_Array_iterator = Array.prototype.iterator;
var std_Array_join = Array.prototype.join;
var std_Array_push = Array.prototype.push;
var std_Array_pop = Array.prototype.pop;
var std_Array_shift = Array.prototype.shift;
var std_Array_slice = Array.prototype.slice;
var std_Array_sort = Array.prototype.sort;
var std_Array_unshift = Array.prototype.unshift;
var std_Boolean_toString = Boolean.prototype.toString;
var Std_Date = Date;
var std_Date_now = Date.now;
var std_Date_valueOf = Date.prototype.valueOf;
var std_Function_bind = Function.prototype.bind;
var std_Function_apply = Function.prototype.apply;
var std_Math_floor = Math.floor;
var std_Math_max = Math.max;
var std_Math_min = Math.min;
var std_Math_abs = Math.abs;
var std_Math_imul = Math.imul;
var std_Math_log2 = Math.log2;
var std_Number_valueOf = Number.prototype.valueOf;
var std_Number_POSITIVE_INFINITY = Number.POSITIVE_INFINITY;
var std_Object_create = Object.create;
var std_Object_getOwnPropertyNames = Object.getOwnPropertyNames;
var std_Object_hasOwnProperty = Object.prototype.hasOwnProperty;
var std_Object_getPrototypeOf = Object.getPrototypeOf;
var std_Object_getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;
var std_RegExp_test = RegExp.prototype.test;
var std_String_fromCharCode = String.fromCharCode;
var std_String_charCodeAt = String.prototype.charCodeAt;
var std_String_indexOf = String.prototype.indexOf;
var std_String_lastIndexOf = String.prototype.lastIndexOf;
var std_String_match = String.prototype.match;
var std_String_replace = String.prototype.replace;
var std_String_split = String.prototype.split;
var std_String_startsWith = String.prototype.startsWith;
var std_String_substring = String.prototype.substring;
var std_String_toLowerCase = String.prototype.toLowerCase;
var std_String_toUpperCase = String.prototype.toUpperCase;
var std_WeakMap = WeakMap;
var std_WeakMap_get = WeakMap.prototype.get;
var std_WeakMap_has = WeakMap.prototype.has;
var std_WeakMap_set = WeakMap.prototype.set;
var std_WeakMap_clear = WeakMap.prototype.clear;
var std_WeakMap_delete = WeakMap.prototype.delete;
var std_Map_has = Map.prototype.has;
var std_Set_has = Set.prototype.has;
var std_iterator = '@@iterator';
var std_StopIteration = StopIteration;
var std_Map_iterator = Map.prototype[std_iterator];
var std_Set_iterator = Set.prototype[std_iterator];
var std_Map_iterator_next = Object.getPrototypeOf(Map()[std_iterator]()).next;
var std_Set_iterator_next = Object.getPrototypeOf(Set()[std_iterator]()).next;
function List() {}
{
  let ListProto = std_Object_create(null);
  ListProto.indexOf = std_Array_indexOf;
  ListProto.join = std_Array_join;
  ListProto.push = std_Array_push;
  ListProto.slice = std_Array_slice;
  ListProto.sort = std_Array_sort;
  MakeConstructible(List, ListProto);
}
function Record() {
    return std_Object_create(null);
}
MakeConstructible(Record, {});
function HasProperty(o, p) {
    return p in o;
}
function ToBoolean(v) {
    return !!v;
}
function ToNumber(v) {
    return +v;
}
function CheckObjectCoercible(v) {
    if (v === undefined || v === null)
        ThrowError(12, ToString(v), "object");
}
function ToLength(v) {
    v = ToInteger(v);
    if (v <= 0)
        return 0;
    return v < 0x1fffffffffffff ? v : 0x1fffffffffffff;
}
function TypedObjectTypeDescr(typedObj) {
  return UnsafeGetReservedSlot(std_Object_getPrototypeOf(typedObj), 0);
}
function TypedObjectGet(descr, typedObj, offset) {
                                            ;
  if (!TypedObjectIsAttached(typedObj))
    ThrowError(331);
  switch (UnsafeGetReservedSlot(descr, 0)) {
  case 1:
    return TypedObjectGetScalar(descr, typedObj, offset);
  case 2:
    return TypedObjectGetReference(descr, typedObj, offset);
  case 5:
    return TypedObjectGetX4(descr, typedObj, offset);
  case 4:
  case 3:
    return TypedObjectGetDerived(descr, typedObj, offset);
  case 0:
    ;
  }
  ;
  return undefined;
}
function TypedObjectGetDerived(descr, typedObj, offset) {
                                              ;
  return NewDerivedTypedObject(descr, typedObj, offset);
}
function TypedObjectGetDerivedIf(descr, typedObj, offset, cond) {
  return (cond ? TypedObjectGetDerived(descr, typedObj, offset) : undefined);
}
function TypedObjectGetOpaque(descr, typedObj, offset) {
                                              ;
  var opaqueTypedObj = NewOpaqueTypedObject(descr);
  AttachTypedObject(opaqueTypedObj, typedObj, offset);
  return opaqueTypedObj;
}
function TypedObjectGetOpaqueIf(descr, typedObj, offset, cond) {
  return (cond ? TypedObjectGetOpaque(descr, typedObj, offset) : undefined);
}
function TypedObjectGetScalar(descr, typedObj, offset) {
  var type = UnsafeGetReservedSlot(descr, 6);
  switch (type) {
  case 0:
    return Load_int8(typedObj, offset);
  case 1:
  case 8:
    return Load_uint8(typedObj, offset);
  case 2:
    return Load_int16(typedObj, offset);
  case 3:
    return Load_uint16(typedObj, offset);
  case 4:
    return Load_int32(typedObj, offset);
  case 5:
    return Load_uint32(typedObj, offset);
  case 6:
    return Load_float32(typedObj, offset);
  case 7:
    return Load_float64(typedObj, offset);
  }
  ;
  return undefined;
}
function TypedObjectGetReference(descr, typedObj, offset) {
  var type = UnsafeGetReservedSlot(descr, 6);
  switch (type) {
  case 0:
    return Load_Any(typedObj, offset);
  case 1:
    return Load_Object(typedObj, offset);
  case 2:
    return Load_string(typedObj, offset);
  }
  ;
  return undefined;
}
function TypedObjectGetX4(descr, typedObj, offset) {
  var type = UnsafeGetReservedSlot(descr, 6);
  switch (type) {
  case 1:
    var x = Load_float32(typedObj, offset + 0);
    var y = Load_float32(typedObj, offset + 4);
    var z = Load_float32(typedObj, offset + 8);
    var w = Load_float32(typedObj, offset + 12);
    return GetFloat32x4TypeDescr()(x, y, z, w);
  case 0:
    var x = Load_int32(typedObj, offset + 0);
    var y = Load_int32(typedObj, offset + 4);
    var z = Load_int32(typedObj, offset + 8);
    var w = Load_int32(typedObj, offset + 12);
    return GetInt32x4TypeDescr()(x, y, z, w);
  }
  ;
  return undefined;
}
function TypedObjectSet(descr, typedObj, offset, fromValue) {
  if (!TypedObjectIsAttached(typedObj))
    ThrowError(331);
  if (IsObject(fromValue) && ObjectIsTypedObject(fromValue)) {
    if (!descr.variable && DescrsEquiv(descr, TypedObjectTypeDescr(fromValue))) {
      if (!TypedObjectIsAttached(fromValue))
        ThrowError(331);
      var size = UnsafeGetReservedSlot(descr, 3);
      Memcpy(typedObj, offset, fromValue, 0, size);
      return;
    }
  }
  switch (UnsafeGetReservedSlot(descr, 0)) {
  case 1:
    TypedObjectSetScalar(descr, typedObj, offset, fromValue);
    return;
  case 2:
    TypedObjectSetReference(descr, typedObj, offset, fromValue);
    return;
  case 5:
    TypedObjectSetX4(descr, typedObj, offset, fromValue);
    return;
  case 4:
    var length = ((UnsafeGetReservedSlot(descr, 7)) | 0);
    if (TypedObjectSetArray(descr, length, typedObj, offset, fromValue))
      return;
    break;
  case 0:
    var length = typedObj.length;
    if (TypedObjectSetArray(descr, length, typedObj, offset, fromValue))
      return;
    break;
  case 3:
    if (!IsObject(fromValue))
      break;
    var fieldNames = UnsafeGetReservedSlot(descr, 6);
    var fieldDescrs = UnsafeGetReservedSlot(descr, 7);
    var fieldOffsets = UnsafeGetReservedSlot(descr, 8);
    for (var i = 0; i < fieldNames.length; i++) {
      var fieldName = fieldNames[i];
      var fieldDescr = fieldDescrs[i];
      var fieldOffset = fieldOffsets[i];
      var fieldValue = fromValue[fieldName];
      TypedObjectSet(fieldDescr, typedObj, offset + fieldOffset, fieldValue);
    }
    return;
  }
  ThrowError(12,
             typeof(fromValue),
             UnsafeGetReservedSlot(descr, 1));
}
function TypedObjectSetArray(descr, length, typedObj, offset, fromValue) {
  if (!IsObject(fromValue))
    return false;
  if (fromValue.length !== length)
    return false;
  if (length > 0) {
    var elemDescr = UnsafeGetReservedSlot(descr, 6);
    var elemSize = UnsafeGetReservedSlot(elemDescr, 3);
    var elemOffset = offset;
    for (var i = 0; i < length; i++) {
      TypedObjectSet(elemDescr, typedObj, elemOffset, fromValue[i]);
      elemOffset += elemSize;
    }
  }
  return true;
}
function TypedObjectSetScalar(descr, typedObj, offset, fromValue) {
                                           ;
  var type = UnsafeGetReservedSlot(descr, 6);
  switch (type) {
  case 0:
    return Store_int8(typedObj, offset,
                     ((fromValue) | 0) & 0xFF);
  case 1:
    return Store_uint8(typedObj, offset,
                      ((fromValue) >>> 0) & 0xFF);
  case 8:
    var v = ClampToUint8(+fromValue);
    return Store_int8(typedObj, offset, v);
  case 2:
    return Store_int16(typedObj, offset,
                      ((fromValue) | 0) & 0xFFFF);
  case 3:
    return Store_uint16(typedObj, offset,
                       ((fromValue) >>> 0) & 0xFFFF);
  case 4:
    return Store_int32(typedObj, offset,
                      ((fromValue) | 0));
  case 5:
    return Store_uint32(typedObj, offset,
                       ((fromValue) >>> 0));
  case 6:
    return Store_float32(typedObj, offset, +fromValue);
  case 7:
    return Store_float64(typedObj, offset, +fromValue);
  }
  ;
  return undefined;
}
function TypedObjectSetReference(descr, typedObj, offset, fromValue) {
  var type = UnsafeGetReservedSlot(descr, 6);
  switch (type) {
  case 0:
    return Store_Any(typedObj, offset, fromValue);
  case 1:
    var value = (fromValue === null ? fromValue : ToObject(fromValue));
    return Store_Object(typedObj, offset, value);
  case 2:
    return Store_string(typedObj, offset, ToString(fromValue));
  }
  ;
  return undefined;
}
function TypedObjectSetX4(descr, typedObj, offset, fromValue) {
  ThrowError(12,
             typeof(fromValue),
             UnsafeGetReservedSlot(descr, 1));
}
function ConvertAndCopyTo(destDescr,
                          destTypedObj,
                          destOffset,
                          fromValue)
{
                                          ;
                                               ;
  if (!TypedObjectIsAttached(destTypedObj))
    ThrowError(331);
  TypedObjectSet(destDescr, destTypedObj, destOffset, fromValue);
}
function Reify(sourceDescr,
               sourceTypedObj,
               sourceOffset) {
                               ;
                                    ;
  if (!TypedObjectIsAttached(sourceTypedObj))
    ThrowError(331);
  return TypedObjectGet(sourceDescr, sourceTypedObj, sourceOffset);
}
function TypeDescrEquivalent(otherDescr) {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(329);
  if (!IsObject(otherDescr) || !ObjectIsTypeDescr(otherDescr))
    ThrowError(329);
  return DescrsEquiv(this, otherDescr);
}
function TypedArrayRedimension(newArrayType) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(329);
  if (!IsObject(newArrayType) || !ObjectIsTypeDescr(newArrayType))
    ThrowError(329);
  var oldArrayType = TypedObjectTypeDescr(this);
  var oldArrayReprKind = UnsafeGetReservedSlot(oldArrayType, 0);
  var oldElementType = oldArrayType;
  var oldElementCount = 1;
  switch (oldArrayReprKind) {
  case 0:
    oldElementCount *= this.length;
    oldElementType = oldElementType.elementType;
    break;
  case 4:
    break;
  default:
    ThrowError(329);
  }
  while (UnsafeGetReservedSlot(oldElementType, 0) === 4) {
    oldElementCount *= oldElementType.length;
    oldElementType = oldElementType.elementType;
  }
  var newElementType = newArrayType;
  var newElementCount = 1;
  while (UnsafeGetReservedSlot(newElementType, 0) == 4) {
    newElementCount *= newElementType.length;
    newElementType = newElementType.elementType;
  }
  if (oldElementCount !== newElementCount) {
    ThrowError(329);
  }
  if (!DescrsEquiv(oldElementType, newElementType)) {
    ThrowError(329);
  }
                                      ;
  return NewDerivedTypedObject(newArrayType, this, 0);
}
function X4ProtoString(type) {
  switch (type) {
  case 0:
    return "int32x4";
  case 1:
    return "float32x4";
  }
  ;
  return undefined;
}
function X4ToSource() {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(3, "X4", "toSource", typeof this);
  var descr = TypedObjectTypeDescr(this);
  if (UnsafeGetReservedSlot(descr, 0) != 5)
    ThrowError(3, "X4", "toSource", typeof this);
  var type = UnsafeGetReservedSlot(descr, 6);
  return X4ProtoString(type)+"("+this.x+", "+this.y+", "+this.z+", "+this.w+")";
}
function DescrsEquiv(descr1, descr2) {
  ;
  ;
  return UnsafeGetReservedSlot(descr1, 1) === UnsafeGetReservedSlot(descr2, 1);
}
function DescrToSource() {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(3, "Type", "toSource", "value");
  return UnsafeGetReservedSlot(this, 1);
}
function ArrayShorthand(...dims) {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(329);
  var T = GetTypedObjectModule();
  if (dims.length == 0)
    return new T.ArrayType(this);
  var accum = this;
  for (var i = dims.length - 1; i >= 0; i--)
    accum = new T.ArrayType(accum).dimension(dims[i]);
  return accum;
}
function StorageOfTypedObject(obj) {
  if (IsObject(obj)) {
    if (ObjectIsOpaqueTypedObject(obj))
      return null;
    if (ObjectIsTransparentTypedObject(obj)) {
      var descr = TypedObjectTypeDescr(obj);
      var byteLength;
      if (UnsafeGetReservedSlot(descr, 0) == 0)
        byteLength = UnsafeGetReservedSlot(descr.elementType, 3) * obj.length;
      else
        byteLength = UnsafeGetReservedSlot(descr, 3);
      return { buffer: UnsafeGetReservedSlot(obj, 2),
               byteLength: byteLength,
               byteOffset: ((UnsafeGetReservedSlot(obj, 0)) | 0) };
    }
  }
  ThrowError(329);
}
function TypeOfTypedObject(obj) {
  if (IsObject(obj) && ObjectIsTypedObject(obj))
    return TypedObjectTypeDescr(obj);
  var T = GetTypedObjectModule();
  switch (typeof obj) {
    case "object": return T.Object;
    case "function": return T.Object;
    case "string": return T.String;
    case "number": return T.float64;
    case "undefined": return T.Any;
    default: return T.Any;
  }
}
function TypedObjectArrayTypeBuild(a,b,c) {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(329);
  var kind = UnsafeGetReservedSlot(this, 0);
  switch (kind) {
  case 4:
    if (typeof a === "function")
      return BuildTypedSeqImpl(this, this.length, 1, a);
    else if (typeof a === "number" && typeof b === "function")
      return BuildTypedSeqImpl(this, this.length, a, b);
    else if (typeof a === "number")
      ThrowError(329);
    else
      ThrowError(329);
  case 0:
    var len = a;
    if (typeof b === "function")
      return BuildTypedSeqImpl(this, len, 1, b);
    else if (typeof b === "number" && typeof c === "function")
      return BuildTypedSeqImpl(this, len, b, c);
    else if (typeof b === "number")
      ThrowError(329);
    else
      ThrowError(329);
  default:
    ThrowError(329);
  }
}
function TypedObjectArrayTypeFrom(a, b, c) {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(329);
  var untypedInput = !IsObject(a) || !ObjectIsTypedObject(a);
  if (untypedInput) {
    var explicitDepth = (b === 1);
    if (explicitDepth && IsCallable(c))
      return MapUntypedSeqImpl(a, this, c);
    else if (IsCallable(b))
      return MapUntypedSeqImpl(a, this, b);
    else
      ThrowError(329);
  } else {
    var explicitDepth = (typeof b === "number");
    if (explicitDepth && IsCallable(c))
      return MapTypedSeqImpl(a, b, this, c);
    else if (IsCallable(b))
      return MapTypedSeqImpl(a, 1, this, b);
    else if (explicitDepth)
      ThrowError(329);
    else
      ThrowError(329);
  }
}
function TypedArrayMap(a, b) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(329);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(329);
  if (typeof a === "number" && typeof b === "function")
    return MapTypedSeqImpl(this, a, thisType, b);
  else if (typeof a === "function")
    return MapTypedSeqImpl(this, 1, thisType, a);
  ThrowError(329);
}
function TypedArrayMapPar(a, b) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    return callFunction(TypedArrayMap, this, a, b);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    return callFunction(TypedArrayMap, this, a, b);
  if (typeof a === "number" && IsCallable(b))
    return MapTypedParImpl(this, a, thisType, b);
  else if (IsCallable(a))
    return MapTypedParImpl(this, 1, thisType, a);
  return callFunction(TypedArrayMap, this, a, b);
}
function TypedArrayReduce(a, b) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(329);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(329);
  if (a !== undefined && typeof a !== "function")
    ThrowError(329);
  var outputType = thisType.elementType;
  return ReduceTypedSeqImpl(this, outputType, a, b);
}
function TypedArrayScatter(a, b, c, d) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(329);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(329);
  if (!IsObject(a) || !ObjectIsTypeDescr(a) || !TypeDescrIsSizedArrayType(a))
    ThrowError(329);
  if (d !== undefined && typeof d !== "function")
    ThrowError(329);
  return ScatterTypedSeqImpl(this, a, b, c, d);
}
function TypedArrayFilter(func) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(329);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(329);
  if (typeof func !== "function")
    ThrowError(329);
  return FilterTypedSeqImpl(this, func);
}
function TypedObjectArrayTypeBuildPar(a,b,c) {
  return callFunction(TypedObjectArrayTypeBuild, this, a, b, c);
}
function TypedObjectArrayTypeFromPar(a,b,c) {
  if (!IsObject(this) || !ObjectIsTypeDescr(this) || !TypeDescrIsArrayType(this))
    return callFunction(TypedObjectArrayTypeFrom, this, a, b, c);
  if (!IsObject(a) || !ObjectIsTypedObject(a))
    return callFunction(TypedObjectArrayTypeFrom, this, a, b, c);
  if (typeof b === "number" && IsCallable(c))
    return MapTypedParImpl(a, b, this, c);
  if (IsCallable(b))
    return MapTypedParImpl(a, 1, this, b);
  return callFunction(TypedObjectArrayTypeFrom, this, a, b, c);
}
function TypedArrayReducePar(a, b) {
  return callFunction(TypedArrayReduce, this, a, b);
}
function TypedArrayScatterPar(a, b, c, d) {
  return callFunction(TypedArrayScatter, this, a, b, c, d);
}
function TypedArrayFilterPar(func) {
  return callFunction(TypedArrayFilter, this, func);
}
function NUM_BYTES(bits) {
  return (bits + 7) >> 3;
}
function SET_BIT(data, index) {
  var word = index >> 3;
  var mask = 1 << (index & 0x7);
  data[word] |= mask;
}
function GET_BIT(data, index) {
  var word = index >> 3;
  var mask = 1 << (index & 0x7);
  return (data[word] & mask) != 0;
}
function BuildTypedSeqImpl(arrayType, len, depth, func) {
  ;
  if (depth <= 0 || ((depth) | 0) !== depth)
    ThrowError(329);
  var [iterationSpace, grainType, totalLength] =
    ComputeIterationSpace(arrayType, depth, len);
  var result = arrayType.variable ? new arrayType(len) : new arrayType();
  var indices = NewDenseArray(depth);
  for (var i = 0; i < depth; i++) {
    indices[i] = 0;
  }
  var grainTypeIsComplex = !TypeDescrIsSimpleType(grainType);
  var size = UnsafeGetReservedSlot(grainType, 3);
  var outOffset = 0;
  for (i = 0; i < totalLength; i++) {
    var userOutPointer = TypedObjectGetOpaqueIf(grainType, result, outOffset,
                                                grainTypeIsComplex);
    callFunction(std_Array_push, indices, userOutPointer);
    var r = callFunction(std_Function_apply, func, undefined, indices);
    callFunction(std_Array_pop, indices);
    if (r !== undefined)
      TypedObjectSet(grainType, result, outOffset, r);
    IncrementIterationSpace(indices, iterationSpace);
    outOffset += size;
  }
  return result;
}
function ComputeIterationSpace(arrayType, depth, len) {
  ;
  ;
  ;
  var iterationSpace = NewDenseArray(depth);
  iterationSpace[0] = len;
  var totalLength = len;
  var grainType = arrayType.elementType;
  for (var i = 1; i < depth; i++) {
    if (TypeDescrIsArrayType(grainType)) {
      var grainLen = grainType.length;
      iterationSpace[i] = grainLen;
      totalLength *= grainLen;
      grainType = grainType.elementType;
    } else {
      ThrowError(328);
    }
  }
  return [iterationSpace, grainType, totalLength];
}
function IncrementIterationSpace(indices, iterationSpace) {
                                                                  ;
  var n = indices.length - 1;
  while (true) {
    indices[n] += 1;
    if (indices[n] < iterationSpace[n])
      return;
                                                                     ;
    indices[n] = 0;
    if (n == 0)
      return;
    n -= 1;
  }
}
function MapUntypedSeqImpl(inArray, outputType, maybeFunc) {
  ;
  ;
  inArray = ToObject(inArray);
  ;
  if (!IsCallable(maybeFunc))
    ThrowError(9, DecompileArg(0, maybeFunc));
  var func = maybeFunc;
  var outLength = outputType.variable ? inArray.length : outputType.length;
  var outGrainType = outputType.elementType;
  var result = outputType.variable ? new outputType(inArray.length) : new outputType();
  var outUnitSize = UnsafeGetReservedSlot(outGrainType, 3);
  var outGrainTypeIsComplex = !TypeDescrIsSimpleType(outGrainType);
  var outOffset = 0;
  for (var i = 0; i < outLength; i++) {
    if (i in inArray) {
      var element = inArray[i];
      var out = TypedObjectGetOpaqueIf(outGrainType, result, outOffset,
                                       outGrainTypeIsComplex);
      var r = func(element, i, inArray, out);
      if (r !== undefined)
        TypedObjectSet(outGrainType, result, outOffset, r);
    }
    outOffset += outUnitSize;
  }
  return result;
}
function MapTypedSeqImpl(inArray, depth, outputType, func) {
  ;
  ;
  ;
  if (depth <= 0 || ((depth) | 0) !== depth)
    ThrowError(329);
  var inputType = TypeOfTypedObject(inArray);
  var [inIterationSpace, inGrainType, _] =
    ComputeIterationSpace(inputType, depth, inArray.length);
  if (!IsObject(inGrainType) || !ObjectIsTypeDescr(inGrainType))
    ThrowError(329);
  var [iterationSpace, outGrainType, totalLength] =
    ComputeIterationSpace(outputType, depth, outputType.variable ? inArray.length : outputType.length);
  for (var i = 0; i < depth; i++)
    if (inIterationSpace[i] !== iterationSpace[i])
      ThrowError(328);
  var result = outputType.variable ? new outputType(inArray.length) : new outputType();
  var inGrainTypeIsComplex = !TypeDescrIsSimpleType(inGrainType);
  var outGrainTypeIsComplex = !TypeDescrIsSimpleType(outGrainType);
  var inOffset = 0;
  var outOffset = 0;
  var isDepth1Simple = depth == 1 && !(inGrainTypeIsComplex || outGrainTypeIsComplex);
  var inUnitSize = isDepth1Simple ? 0 : UnsafeGetReservedSlot(inGrainType, 3);
  var outUnitSize = isDepth1Simple ? 0 : UnsafeGetReservedSlot(outGrainType, 3);
  function DoMapTypedSeqDepth1() {
    for (var i = 0; i < totalLength; i++) {
      var element = TypedObjectGet(inGrainType, inArray, inOffset);
      var out = TypedObjectGetOpaqueIf(outGrainType, result, outOffset,
                                       outGrainTypeIsComplex);
      var r = func(element, i, inArray, out);
      if (r !== undefined)
        TypedObjectSet(outGrainType, result, outOffset, r);
      inOffset += inUnitSize;
      outOffset += outUnitSize;
    }
    return result;
  }
  function DoMapTypedSeqDepth1Simple(inArray, totalLength, func, result) {
    for (var i = 0; i < totalLength; i++) {
      var r = func(inArray[i], i, inArray, undefined);
      if (r !== undefined)
        result[i] = r;
    }
    return result;
  }
  function DoMapTypedSeqDepthN() {
    var indices = new Uint32Array(depth);
    for (var i = 0; i < totalLength; i++) {
      var element = TypedObjectGet(inGrainType, inArray, inOffset);
      var out = TypedObjectGetOpaqueIf(outGrainType, result, outOffset,
                                       outGrainTypeIsComplex);
      var args = [element];
      callFunction(std_Function_apply, std_Array_push, args, indices);
      callFunction(std_Array_push, args, inArray, out);
      var r = callFunction(std_Function_apply, func, void 0, args);
      if (r !== undefined)
        TypedObjectSet(outGrainType, result, outOffset, r);
      inOffset += inUnitSize;
      outOffset += outUnitSize;
      IncrementIterationSpace(indices, iterationSpace);
    }
    return result;
  }
  if (isDepth1Simple)
    return DoMapTypedSeqDepth1Simple(inArray, totalLength, func, result);
  if (depth == 1)
    return DoMapTypedSeqDepth1();
  return DoMapTypedSeqDepthN();
}
function MapTypedParImpl(inArray, depth, outputType, func) {
                                                         ;
                                                                 ;
                                                        ;
                                                  ;
                                                     ;
  var inArrayType = TypeOfTypedObject(inArray);
  if (ShouldForceSequential() ||
      depth <= 0 ||
      ((depth) | 0) !== depth ||
      !TypeDescrIsArrayType(inArrayType) ||
      !TypeDescrIsUnsizedArrayType(outputType))
  {
    return MapTypedSeqImpl(inArray, depth, outputType, func);
  }
  switch (depth) {
  case 1:
    return MapTypedParImplDepth1(inArray, inArrayType, outputType, func);
  default:
    return MapTypedSeqImpl(inArray, depth, outputType, func);
  }
}
function RedirectPointer(typedObj, offset, outputIsScalar) {
  if (!outputIsScalar || !InParallelSection()) {
    typedObj = NewDerivedTypedObject(TypedObjectTypeDescr(typedObj),
                                     typedObj, 0);
  }
  SetTypedObjectOffset(typedObj, offset);
  return typedObj;
}
SetScriptHints(RedirectPointer, { inline: true });
function MapTypedParImplDepth1(inArray, inArrayType, outArrayType, func) {
                                                    ;
                                                     ;
                                                ;
  const inGrainType = inArrayType.elementType;
  const outGrainType = outArrayType.elementType;
  const inGrainTypeSize = UnsafeGetReservedSlot(inGrainType, 3);
  const outGrainTypeSize = UnsafeGetReservedSlot(outGrainType, 3);
  const inGrainTypeIsComplex = !TypeDescrIsSimpleType(inGrainType);
  const outGrainTypeIsComplex = !TypeDescrIsSimpleType(outGrainType);
  const length = inArray.length;
  const mode = undefined;
  const outArray = new outArrayType(length);
  if (length === 0)
    return outArray;
  const outGrainTypeIsTransparent = ObjectIsTransparentTypedObject(outArray);
  const slicesInfo = ComputeSlicesInfo(length);
  const numWorkers = ForkJoinNumWorkers();
  ;
  const pointers = [];
  for (var i = 0; i < numWorkers; i++) {
    const inTypedObject = TypedObjectGetDerivedIf(inGrainType, inArray, 0,
                                                  inGrainTypeIsComplex);
    const outTypedObject = TypedObjectGetOpaqueIf(outGrainType, outArray, 0,
                                                  outGrainTypeIsComplex);
    callFunction(std_Array_push, pointers, ({ inTypedObject: inTypedObject, outTypedObject: outTypedObject }));
                                                              ;
  }
  const inBaseOffset = ((UnsafeGetReservedSlot(inArray, 0)) | 0);
  ForkJoin(mapThread, 0, slicesInfo.count, ForkJoinMode(mode), outArray);
  return outArray;
  function mapThread(workerId, sliceStart, sliceEnd) {
                                           ;
                                                                        ;
    var pointerIndex = InParallelSection() ? workerId : 0;
                                                      ;
    const { inTypedObject, outTypedObject } = pointers[pointerIndex];
    const sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      const indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      const indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);
      var inOffset = inBaseOffset + std_Math_imul(inGrainTypeSize, indexStart);
      var outOffset = std_Math_imul(outGrainTypeSize, indexStart);
      const endOffset = std_Math_imul(outGrainTypeSize, indexEnd);
      SetForkJoinTargetRegion(outArray, outOffset, endOffset);
      for (var i = indexStart; i < indexEnd; i++) {
        var inVal = (inGrainTypeIsComplex
                     ? RedirectPointer(inTypedObject, inOffset,
                                       outGrainTypeIsTransparent)
                     : inArray[i]);
        var outVal = (outGrainTypeIsComplex
                      ? RedirectPointer(outTypedObject, outOffset,
                                        outGrainTypeIsTransparent)
                      : undefined);
        const r = func(inVal, i, inArray, outVal);
        if (r !== undefined) {
          if (outGrainTypeIsComplex)
            SetTypedObjectValue(outGrainType, outArray, outOffset, r);
          else
            UnsafePutElements(outArray, i, r);
        }
        inOffset += inGrainTypeSize;
        outOffset += outGrainTypeSize;
        if (outGrainTypeIsTransparent)
          ClearThreadLocalArenas();
      }
    }
    return sliceId;
  }
  return undefined;
}
SetScriptHints(MapTypedParImplDepth1, { cloneAtCallsite: true });
function ReduceTypedSeqImpl(array, outputType, func, initial) {
  ;
  ;
  var start, value;
  if (initial === undefined && array.length < 1)
    ThrowError(328);
  if (TypeDescrIsSimpleType(outputType)) {
    if (initial === undefined) {
      start = 1;
      value = array[0];
    } else {
      start = 0;
      value = outputType(initial);
    }
    for (var i = start; i < array.length; i++)
      value = outputType(func(value, array[i]));
  } else {
    if (initial === undefined) {
      start = 1;
      value = new outputType(array[0]);
    } else {
      start = 0;
      value = initial;
    }
    for (var i = start; i < array.length; i++)
      value = func(value, array[i]);
  }
  return value;
}
function ScatterTypedSeqImpl(array, outputType, indices, defaultValue, conflictFunc) {
  ;
  ;
  ;
  ;
  var result = new outputType();
  var bitvec = new Uint8Array(result.length);
  var elemType = outputType.elementType;
  var i, j;
  if (defaultValue !== elemType(undefined)) {
    for (i = 0; i < result.length; i++) {
      result[i] = defaultValue;
    }
  }
  for (i = 0; i < indices.length; i++) {
    j = indices[i];
    if (!GET_BIT(bitvec, j)) {
      result[j] = array[i];
      SET_BIT(bitvec, j);
    } else if (conflictFunc === undefined) {
      ThrowError(341);
    } else {
      result[j] = conflictFunc(result[j], elemType(array[i]));
    }
  }
  return result;
}
function FilterTypedSeqImpl(array, func) {
  ;
  ;
  var arrayType = TypeOfTypedObject(array);
  if (!TypeDescrIsArrayType(arrayType))
    ThrowError(329);
  var elementType = arrayType.elementType;
  var flags = new Uint8Array(NUM_BYTES(array.length));
  var count = 0;
  var size = UnsafeGetReservedSlot(elementType, 3);
  var inOffset = 0;
  for (var i = 0; i < array.length; i++) {
    var v = TypedObjectGet(elementType, array, inOffset);
    if (func(v, i, array)) {
      SET_BIT(flags, i);
      count++;
    }
    inOffset += size;
  }
  var resultType = (arrayType.variable ? arrayType : arrayType.unsized);
  var result = new resultType(count);
  for (var i = 0, j = 0; i < array.length; i++) {
    if (GET_BIT(flags, i))
      result[j++] = array[i];
  }
  return result;
}
function ArrayIndexOf(searchElement ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (len === 0)
        return -1;
    var n = arguments.length > 1 ? ToInteger(arguments[1]) : 0;
    if (n >= len)
        return -1;
    var k;
    if (n >= 0)
        k = n;
    else {
        k = len + n;
        if (k < 0)
            k = 0;
    }
    if (IsPackedArray(O)) {
        for (; k < len; k++) {
            if (O[k] === searchElement)
                return k;
        }
    } else {
        for (; k < len; k++) {
            if (k in O && O[k] === searchElement)
                return k;
        }
    }
    return -1;
}
function ArrayStaticIndexOf(list, searchElement ) {
    if (arguments.length < 1)
        ThrowError(37, 0, 'Array.indexOf');
    var fromIndex = arguments.length > 2 ? arguments[2] : 0;
    return callFunction(ArrayIndexOf, list, searchElement, fromIndex);
}
function ArrayLastIndexOf(searchElement ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (len === 0)
        return -1;
    var n = arguments.length > 1 ? ToInteger(arguments[1]) : len - 1;
    var k;
    if (n > len - 1)
        k = len - 1;
    else if (n < 0)
        k = len + n;
    else
        k = n;
    if (IsPackedArray(O)) {
        for (; k >= 0; k--) {
            if (O[k] === searchElement)
                return k;
        }
    } else {
        for (; k >= 0; k--) {
            if (k in O && O[k] === searchElement)
                return k;
        }
    }
    return -1;
}
function ArrayStaticLastIndexOf(list, searchElement ) {
    if (arguments.length < 1)
        ThrowError(37, 0, 'Array.lastIndexOf');
    var fromIndex;
    if (arguments.length > 2) {
        fromIndex = arguments[2];
    } else {
        var O = ToObject(list);
        var len = ((O.length) >>> 0);
        fromIndex = len - 1;
    }
    return callFunction(ArrayLastIndexOf, list, searchElement, fromIndex);
}
function ArrayEvery(callbackfn ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.every');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var T = arguments.length > 1 ? arguments[1] : void 0;
    for (var k = 0; k < len; k++) {
        if (k in O) {
            if (!callFunction(callbackfn, T, O[k], k, O))
                return false;
        }
    }
    return true;
}
function ArrayStaticEvery(list, callbackfn ) {
    if (arguments.length < 2)
        ThrowError(37, 0, 'Array.every');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayEvery, list, callbackfn, T);
}
function ArraySome(callbackfn ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.some');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var T = arguments.length > 1 ? arguments[1] : void 0;
    for (var k = 0; k < len; k++) {
        if (k in O) {
            if (callFunction(callbackfn, T, O[k], k, O))
                return true;
        }
    }
    return false;
}
function ArrayStaticSome(list, callbackfn ) {
    if (arguments.length < 2)
        ThrowError(37, 0, 'Array.some');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArraySome, list, callbackfn, T);
}
function ArrayForEach(callbackfn ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.forEach');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var T = arguments.length > 1 ? arguments[1] : void 0;
    for (var k = 0; k < len; k++) {
        if (k in O) {
            callFunction(callbackfn, T, O[k], k, O);
        }
    }
    return void 0;
}
function ArrayMap(callbackfn ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.map');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var T = arguments.length > 1 ? arguments[1] : void 0;
    var A = NewDenseArray(len);
    for (var k = 0; k < len; k++) {
        if (k in O) {
            var mappedValue = callFunction(callbackfn, T, O[k], k, O);
            UnsafePutElements(A, k, mappedValue);
        }
    }
    return A;
}
function ArrayStaticMap(list, callbackfn ) {
    if (arguments.length < 2)
        ThrowError(37, 0, 'Array.map');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayMap, list, callbackfn, T);
}
function ArrayStaticForEach(list, callbackfn ) {
    if (arguments.length < 2)
        ThrowError(37, 0, 'Array.forEach');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    callFunction(ArrayForEach, list, callbackfn, T);
}
function ArrayReduce(callbackfn ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var k = 0;
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        if (len === 0)
            ThrowError(35);
        if (IsPackedArray(O)) {
            accumulator = O[k++];
        } else {
            var kPresent = false;
            for (; k < len; k++) {
                if (k in O) {
                    accumulator = O[k];
                    kPresent = true;
                    k++;
                    break;
                }
            }
            if (!kPresent)
              ThrowError(35);
        }
    }
    for (; k < len; k++) {
        if (k in O) {
            accumulator = callbackfn(accumulator, O[k], k, O);
        }
    }
    return accumulator;
}
function ArrayStaticReduce(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(37, 0, 'Array.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduce, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduce, list, callbackfn);
}
function ArrayReduceRight(callbackfn ) {
    var O = ToObject(this);
    var len = ((O.length) >>> 0);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var k = len - 1;
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        if (len === 0)
            ThrowError(35);
        if (IsPackedArray(O)) {
            accumulator = O[k--];
        } else {
            var kPresent = false;
            for (; k >= 0; k--) {
                if (k in O) {
                    accumulator = O[k];
                    kPresent = true;
                    k--;
                    break;
                }
            }
            if (!kPresent)
                ThrowError(35);
        }
    }
    for (; k >= 0; k--) {
        if (k in O) {
            accumulator = callbackfn(accumulator, O[k], k, O);
        }
    }
    return accumulator;
}
function ArrayStaticReduceRight(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(37, 0, 'Array.reduceRight');
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduceRight, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduceRight, list, callbackfn);
}
function ArrayFind(predicate ) {
    var O = ToObject(this);
    var len = ToInteger(O.length);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.find');
    if (!IsCallable(predicate))
        ThrowError(9, DecompileArg(0, predicate));
    var T = arguments.length > 1 ? arguments[1] : undefined;
    for (var k = 0; k < len; k++) {
        var kValue = O[k];
        if (callFunction(predicate, T, kValue, k, O))
            return kValue;
    }
    return undefined;
}
function ArrayFindIndex(predicate ) {
    var O = ToObject(this);
    var len = ToInteger(O.length);
    if (arguments.length === 0)
        ThrowError(37, 0, 'Array.prototype.find');
    if (!IsCallable(predicate))
        ThrowError(9, DecompileArg(0, predicate));
    var T = arguments.length > 1 ? arguments[1] : undefined;
    for (var k = 0; k < len; k++) {
        if (callFunction(predicate, T, O[k], k, O))
            return k;
    }
    return -1;
}
function ArrayCopyWithin(target, start, end = undefined) {
    var O = ToObject(this);
    var len = ToInteger(O.length);
    var relativeTarget = ToInteger(target);
    var to = relativeTarget < 0 ? std_Math_max(len + relativeTarget, 0)
                                : std_Math_min(relativeTarget, len);
    var relativeStart = ToInteger(start);
    var from = relativeStart < 0 ? std_Math_max(len + relativeStart, 0)
                                 : std_Math_min(relativeStart, len);
    var relativeEnd = end === undefined ? len
                                        : ToInteger(end);
    var final = relativeEnd < 0 ? std_Math_max(len + relativeEnd, 0)
                                : std_Math_min(relativeEnd, len);
    var count = std_Math_min(final - from, len - to);
    if (from < to && to < (from + count)) {
        from = from + count - 1;
        to = to + count - 1;
        while (count > 0) {
            if (from in O)
                O[to] = O[from];
            else
                delete O[to];
            from--;
            to--;
            count--;
        }
    } else {
        while (count > 0) {
            if (from in O)
                O[to] = O[from];
            else
                delete O[to];
            from++;
            to++;
            count--;
        }
    }
    return O;
}
function ArrayFill(value, start = 0, end = undefined) {
    var O = ToObject(this);
    var len = ToInteger(O.length);
    var relativeStart = ToInteger(start);
    var k = relativeStart < 0
            ? std_Math_max(len + relativeStart, 0)
            : std_Math_min(relativeStart, len);
    var relativeEnd = end === undefined ? len : ToInteger(end);
    var final = relativeEnd < 0
                ? std_Math_max(len + relativeEnd, 0)
                : std_Math_min(relativeEnd, len);
    for (; k < final; k++) {
        O[k] = value;
    }
    return O;
}
function CreateArrayIteratorAt(obj, kind, n) {
    var iteratedObject = ToObject(obj);
    var iterator = NewArrayIterator();
    UnsafeSetReservedSlot(iterator, 0, iteratedObject);
    UnsafeSetReservedSlot(iterator, 1, n);
    UnsafeSetReservedSlot(iterator, 2, kind);
    return iterator;
}
function CreateArrayIterator(obj, kind) {
    return CreateArrayIteratorAt(obj, kind, 0);
}
function ArrayIteratorIdentity() {
    return this;
}
function ArrayIteratorNext() {
    if (!IsObject(this) || !IsArrayIterator(this))
        ThrowError(26, "ArrayIterator", "next", ToString(this));
    var a = UnsafeGetReservedSlot(this, 0);
    var index = UnsafeGetReservedSlot(this, 1);
    var itemKind = UnsafeGetReservedSlot(this, 2);
    var result = { value: undefined, done: false };
    if (index >= ((a.length) >>> 0)) {
        UnsafeSetReservedSlot(this, 1, 0xffffffff);
        result.done = true;
        return result;
    }
    UnsafeSetReservedSlot(this, 1, index + 1);
    if (itemKind === 0) {
        result.value = a[index];
        return result;
    }
    if (itemKind === 1) {
        var pair = NewDenseArray(2);
        pair[0] = index;
        pair[1] = a[index];
        result.value = pair;
        return result;
    }
    ;
    result.value = index;
    return result;
}
function ArrayValuesAt(n) {
    return CreateArrayIteratorAt(this, 0, n);
}
function ArrayValues() {
    return CreateArrayIterator(this, 0);
}
function ArrayEntries() {
    return CreateArrayIterator(this, 1);
}
function ArrayKeys() {
    return CreateArrayIterator(this, 2);
}
function ArrayFrom(arrayLike, mapfn=undefined, thisArg=undefined) {
    var C = this;
    var items = ToObject(arrayLike);
    var mapping = (mapfn !== undefined);
    if (mapping && !IsCallable(mapfn))
        ThrowError(9, DecompileArg(1, mapfn));
    var attrs = 0x02 | 0x01 | 0x04;
    var usingIterator = items["@@iterator"];
    if (usingIterator !== undefined) {
        var A = IsConstructor(C) ? new C() : [];
        var iterator = callFunction(usingIterator, items);
        var k = 0;
        var next;
        while (true) {
            next = iterator.next();
            if (!IsObject(next))
                ThrowError(51);
            if (next.done)
                break;
            var nextValue = next.value;
            var mappedValue = mapping ? callFunction(mapfn, thisArg, nextValue, k) : nextValue;
            _DefineDataProperty(A, k++, mappedValue, attrs);
        }
    } else {
        var len = ToInteger(items.length);
        var A = IsConstructor(C) ? new C(len) : NewDenseArray(len);
        for (var k = 0; k < len; k++) {
            var kValue = items[k];
            var mappedValue = mapping ? callFunction(mapfn, thisArg, kValue, k) : kValue;
            _DefineDataProperty(A, k, mappedValue, attrs);
        }
    }
    A.length = k;
    return A;
}
var dateTimeFormatCache = new Record();
function Date_toLocaleString() {
    var x = callFunction(std_Date_valueOf, this);
    if (Number_isNaN(x))
        return "Invalid Date";
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;
    var dateTimeFormat;
    if (locales === undefined && options === undefined) {
        if (dateTimeFormatCache.dateTimeFormat === undefined) {
            options = ToDateTimeOptions(options, "any", "all");
            dateTimeFormatCache.dateTimeFormat = intl_DateTimeFormat(locales, options);
        }
        dateTimeFormat = dateTimeFormatCache.dateTimeFormat;
    } else {
        options = ToDateTimeOptions(options, "any", "all");
        dateTimeFormat = intl_DateTimeFormat(locales, options);
    }
    return intl_FormatDateTime(dateTimeFormat, x);
}
function Date_toLocaleDateString() {
    var x = callFunction(std_Date_valueOf, this);
    if (Number_isNaN(x))
        return "Invalid Date";
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;
    var dateTimeFormat;
    if (locales === undefined && options === undefined) {
        if (dateTimeFormatCache.dateFormat === undefined) {
            options = ToDateTimeOptions(options, "date", "date");
            dateTimeFormatCache.dateFormat = intl_DateTimeFormat(locales, options);
        }
        dateTimeFormat = dateTimeFormatCache.dateFormat;
    } else {
        options = ToDateTimeOptions(options, "date", "date");
        dateTimeFormat = intl_DateTimeFormat(locales, options);
    }
    return intl_FormatDateTime(dateTimeFormat, x);
}
function Date_toLocaleTimeString() {
    var x = callFunction(std_Date_valueOf, this);
    if (Number_isNaN(x))
        return "Invalid Date";
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;
    var dateTimeFormat;
    if (locales === undefined && options === undefined) {
        if (dateTimeFormatCache.timeFormat === undefined) {
            options = ToDateTimeOptions(options, "time", "time");
            dateTimeFormatCache.timeFormat = intl_DateTimeFormat(locales, options);
        }
        dateTimeFormat = dateTimeFormatCache.timeFormat;
    } else {
        options = ToDateTimeOptions(options, "time", "time");
        dateTimeFormat = intl_DateTimeFormat(locales, options);
    }
    return intl_FormatDateTime(dateTimeFormat, x);
}
function ObjectStaticAssign(target, firstSource) {
    var to = ToObject(target);
    if (arguments.length < 2)
        return to;
    var i = 1;
    do {
        var nextSource = arguments[i];
        if (nextSource === null || nextSource === undefined)
            continue;
        var from = ToObject(nextSource);
        var keysArray = OwnPropertyKeys(from);
        var len = keysArray.length;
        var nextIndex = 0;
        const MISSING = {};
        var pendingException = MISSING;
        while (nextIndex < len) {
            var nextKey = keysArray[nextIndex];
            try {
                var desc = std_Object_getOwnPropertyDescriptor(from, nextKey);
                if (desc !== undefined && desc.enumerable)
                    to[nextKey] = from[nextKey];
            } catch (e) {
                if (pendingException === MISSING)
                    pendingException = e;
            }
            nextIndex++;
        }
        if (pendingException !== MISSING)
            throw pendingException;
    } while (++i < arguments.length);
    return to;
}
function String_codePointAt(pos) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    var position = ToInteger(pos);
    var size = S.length;
    if (position < 0 || position >= size)
        return undefined;
    var first = callFunction(std_String_charCodeAt, S, position);
    if (first < 0xD800 || first > 0xDBFF || position + 1 === size)
        return first;
    var second = callFunction(std_String_charCodeAt, S, position + 1);
    if (second < 0xDC00 || second > 0xDFFF)
        return first;
    return (first - 0xD800) * 0x400 + (second - 0xDC00) + 0x10000;
}
var collatorCache = new Record();
function String_repeat(count) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    var n = ToInteger(count);
    if (n < 0)
        ThrowError(68);
    if (!(n * S.length < (1 << 28)))
        ThrowError(70);
    n = n & ((1 << 28) - 1);
    var T = "";
    for (;;) {
        if (n & 1)
            T += S;
        n >>= 1;
        if (n)
            S += S;
        else
            break;
    }
    return T;
}
function String_iterator() {
    CheckObjectCoercible(this);
    var S = ToString(this);
    var iterator = NewStringIterator();
    UnsafeSetReservedSlot(iterator, 0, S);
    UnsafeSetReservedSlot(iterator, 1, 0);
    return iterator;
}
function StringIteratorIdentity() {
    return this;
}
function StringIteratorNext() {
    if (!IsObject(this) || !IsStringIterator(this))
        ThrowError(26, "StringIterator", "next", ToString(this));
    var S = UnsafeGetReservedSlot(this, 0);
    var index = UnsafeGetReservedSlot(this, 1);
    var size = S.length;
    var result = { value: undefined, done: false };
    if (index >= size) {
        result.done = true;
        return result;
    }
    var charCount = 1;
    var first = callFunction(std_String_charCodeAt, S, index);
    if (first >= 0xD800 && first <= 0xDBFF && index + 1 < size) {
        var second = callFunction(std_String_charCodeAt, S, index + 1);
        if (second >= 0xDC00 && second <= 0xDFFF) {
            charCount = 2;
        }
    }
    UnsafeSetReservedSlot(this, 1, index + charCount);
    result.value = callFunction(std_String_substring, S, index, index + charCount);
    return result;
}
function String_localeCompare(that) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    var That = ToString(that);
    var locales = arguments.length > 1 ? arguments[1] : undefined;
    var options = arguments.length > 2 ? arguments[2] : undefined;
    var collator;
    if (locales === undefined && options === undefined) {
        if (collatorCache.collator === undefined)
            collatorCache.collator = intl_Collator(locales, options);
        collator = collatorCache.collator;
    } else {
        collator = intl_Collator(locales, options);
    }
    return intl_CompareStrings(collator, S, That);
}
function String_static_fromCodePoint() {
    var length = arguments.length;
    var elements = new List();
    for (var nextIndex = 0; nextIndex < length; nextIndex++) {
        var next = arguments[nextIndex];
        var nextCP = ToNumber(next);
        if (nextCP !== ToInteger(nextCP) || Number_isNaN(nextCP))
            ThrowError(69, ToString(nextCP));
        if (nextCP < 0 || nextCP > 0x10FFFF)
            ThrowError(69, ToString(nextCP));
        if (nextCP <= 0xFFFF) {
            elements.push(nextCP);
            continue;
        }
        elements.push((((nextCP - 0x10000) / 0x400) | 0) + 0xD800);
        elements.push((nextCP - 0x10000) % 0x400 + 0xDC00);
    }
    return callFunction(std_Function_apply, std_String_fromCharCode, null, elements);
}
function String_static_raw(callSite, ...substitutions) {
    var numberOfSubstitutions = substitutions.length;
    var cooked = ToObject(callSite);
    var raw = ToObject(cooked.raw);
    var literalSegments = ToLength(raw.length);
    if (literalSegments <= 0)
        return "";
    var resultString = "";
    var nextIndex = 0;
    while (true) {
        var nextSeg = ToString(raw[nextIndex]);
        resultString = resultString + nextSeg;
        if (nextIndex + 1 === literalSegments)
            return resultString;
        var nextSub;
        if (nextIndex < numberOfSubstitutions)
            nextSub = ToString(substitutions[nextIndex]);
        else
            nextSub = "";
        resultString = resultString + nextSub;
        nextIndex++;
    }
}
function String_static_localeCompare(str1, str2) {
    if (arguments.length < 1)
        ThrowError(37, 0, "String.localeCompare");
    var locales = arguments.length > 2 ? arguments[2] : undefined;
    var options = arguments.length > 3 ? arguments[3] : undefined;
    return callFunction(String_localeCompare, str1, str2, locales, options);
}
function String_big() {
    CheckObjectCoercible(this);
    return "<big>" + ToString(this) + "</big>";
}
function String_blink() {
    CheckObjectCoercible(this);
    return "<blink>" + ToString(this) + "</blink>";
}
function String_bold() {
    CheckObjectCoercible(this);
    return "<b>" + ToString(this) + "</b>";
}
function String_fixed() {
    CheckObjectCoercible(this);
    return "<tt>" + ToString(this) + "</tt>";
}
function String_italics() {
    CheckObjectCoercible(this);
    return "<i>" + ToString(this) + "</i>";
}
function String_small() {
    CheckObjectCoercible(this);
    return "<small>" + ToString(this) + "</small>";
}
function String_strike() {
    CheckObjectCoercible(this);
    return "<strike>" + ToString(this) + "</strike>";
}
function String_sub() {
    CheckObjectCoercible(this);
    return "<sub>" + ToString(this) + "</sub>";
}
function String_sup() {
    CheckObjectCoercible(this);
    return "<sup>" + ToString(this) + "</sup>";
}
function EscapeAttributeValue(v) {
    var inputStr = ToString(v);
    var inputLen = inputStr.length;
    var outputStr = "";
    var chunkStart = 0;
    for (var i = 0; i < inputLen; i++) {
        if (inputStr[i] === '"') {
            outputStr += callFunction(std_String_substring, inputStr, chunkStart, i) + '&quot;';
            chunkStart = i + 1;
        }
    }
    if (chunkStart === 0)
        return inputStr;
    if (chunkStart < inputLen)
        outputStr += callFunction(std_String_substring, inputStr, chunkStart);
    return outputStr;
}
function String_anchor(name) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<a name="' + EscapeAttributeValue(name) + '">' + S + "</a>";
}
function String_fontcolor(color) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<font color="' + EscapeAttributeValue(color) + '">' + S + "</font>";
}
function String_fontsize(size) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<font size="' + EscapeAttributeValue(size) + '">' + S + "</font>";
}
function String_link(url) {
    CheckObjectCoercible(this);
    var S = ToString(this);
    return '<a href="' + EscapeAttributeValue(url) + '">' + S + "</a>";
}
function ErrorToString()
{
  var obj = this;
  if (!IsObject(obj))
    ThrowError(3, "Error", "toString", "value");
  var name = obj.name;
  name = (name === undefined) ? "Error" : ToString(name);
  var msg = obj.message;
  msg = (msg === undefined) ? "" : ToString(msg);
  if (name === "")
    return msg;
  if (msg === "")
    return name;
  return name + ": " + msg;
}
function toASCIIUpperCase(s) {
    ;
    var result = "";
    for (var i = 0; i < s.length; i++) {
        var c = s[i];
        if ("a" <= c && c <= "z")
            c = callFunction(std_String_toUpperCase, c);
        result += c;
    }
    return result;
}
var unicodeLocaleExtensionSequence = "-u(-[a-z0-9]{2,8})+";
var unicodeLocaleExtensionSequenceRE = new RegExp(unicodeLocaleExtensionSequence);
function removeUnicodeExtensions(locale) {
    var extensions;
    while ((extensions = regexp_exec_no_statics(unicodeLocaleExtensionSequenceRE, locale)) !== null) {
        locale = callFunction(std_String_replace, locale, extensions[0], "");
        unicodeLocaleExtensionSequenceRE.lastIndex = 0;
    }
    return locale;
}
var languageTagRE = (function () {
    var ALPHA = "[a-zA-Z]";
    var DIGIT = "[0-9]";
    var alphanum = "(?:" + ALPHA + "|" + DIGIT + ")";
    var regular = "(?:art-lojban|cel-gaulish|no-bok|no-nyn|zh-guoyu|zh-hakka|zh-min|zh-min-nan|zh-xiang)";
    var irregular = "(?:en-GB-oed|i-ami|i-bnn|i-default|i-enochian|i-hak|i-klingon|i-lux|i-mingo|i-navajo|i-pwn|i-tao|i-tay|i-tsu|sgn-BE-FR|sgn-BE-NL|sgn-CH-DE)";
    var grandfathered = "(?:" + irregular + "|" + regular + ")";
    var privateuse = "(?:x(?:-[a-z0-9]{1,8})+)";
    var singleton = "(?:" + DIGIT + "|[A-WY-Za-wy-z])";
    var extension = "(?:" + singleton + "(?:-" + alphanum + "{2,8})+)";
    var variant = "(?:" + alphanum + "{5,8}|(?:" + DIGIT + alphanum + "{3}))";
    var region = "(?:" + ALPHA + "{2}|" + DIGIT + "{3})";
    var script = "(?:" + ALPHA + "{4})";
    var extlang = "(?:" + ALPHA + "{3}(?:-" + ALPHA + "{3}){0,2})";
    var language = "(?:" + ALPHA + "{2,3}(?:-" + extlang + ")?|" + ALPHA + "{4}|" + ALPHA + "{5,8})";
    var langtag = language + "(?:-" + script + ")?(?:-" + region + ")?(?:-" +
                  variant + ")*(?:-" + extension + ")*(?:-" + privateuse + ")?";
    var languageTag = "^(?:" + langtag + "|" + privateuse + "|" + grandfathered + ")$";
    return new RegExp(languageTag, "i");
}());
var duplicateVariantRE = (function () {
    var ALPHA = "[a-zA-Z]";
    var DIGIT = "[0-9]";
    var alphanum = "(?:" + ALPHA + "|" + DIGIT + ")";
    var variant = "(?:" + alphanum + "{5,8}|(?:" + DIGIT + alphanum + "{3}))";
    var duplicateVariant =
        "(?:" + alphanum + "{2,8}-)+" +
        "(" + variant + ")-" +
        "(?:" + alphanum + "{2,8}-)*" +
        "\\1" +
        "(?!" + alphanum + ")";
    return new RegExp(duplicateVariant);
}());
var duplicateSingletonRE = (function () {
    var ALPHA = "[a-zA-Z]";
    var DIGIT = "[0-9]";
    var alphanum = "(?:" + ALPHA + "|" + DIGIT + ")";
    var singleton = "(?:" + DIGIT + "|[A-WY-Za-wy-z])";
    var duplicateSingleton =
        "-(" + singleton + ")-" +
        "(?:" + alphanum + "+-)*" +
        "\\1" +
        "(?!" + alphanum + ")";
    return new RegExp(duplicateSingleton);
}());
function IsStructurallyValidLanguageTag(locale) {
    ;
    if (!regexp_test_no_statics(languageTagRE, locale))
        return false;
    if (callFunction(std_String_startsWith, locale, "x-"))
        return true;
    var pos = callFunction(std_String_indexOf, locale, "-x-");
    if (pos !== -1)
        locale = callFunction(std_String_substring, locale, 0, pos);
    return !regexp_test_no_statics(duplicateVariantRE, locale) &&
           !regexp_test_no_statics(duplicateSingletonRE, locale);
}
function CanonicalizeLanguageTag(locale) {
    ;
    locale = callFunction(std_String_toLowerCase, locale);
    if (callFunction(std_Object_hasOwnProperty, langTagMappings, locale))
        return langTagMappings[locale];
    var subtags = callFunction(std_String_split, locale, "-");
    var i = 0;
    while (i < subtags.length) {
        var subtag = subtags[i];
        if (subtag.length === 1 && (i > 0 || subtag === "x"))
            break;
        if (subtag.length === 4) {
            subtag = callFunction(std_String_toUpperCase, subtag[0]) +
                     callFunction(std_String_substring, subtag, 1);
        } else if (i !== 0 && subtag.length === 2) {
            subtag = callFunction(std_String_toUpperCase, subtag);
        }
        if (callFunction(std_Object_hasOwnProperty, langSubtagMappings, subtag)) {
            subtag = langSubtagMappings[subtag];
        } else if (callFunction(std_Object_hasOwnProperty, extlangMappings, subtag)) {
            subtag = extlangMappings[subtag].preferred;
            if (i === 1 && extlangMappings[subtag].prefix === subtags[0]) {
                callFunction(std_Array_shift, subtags);
                i--;
            }
        }
        subtags[i] = subtag;
        i++;
    }
    var normal = callFunction(std_Array_join, callFunction(std_Array_slice, subtags, 0, i), "-");
    var extensions = new List();
    while (i < subtags.length && subtags[i] !== "x") {
        var extensionStart = i;
        i++;
        while (i < subtags.length && subtags[i].length > 1)
            i++;
        var extension = callFunction(std_Array_join, callFunction(std_Array_slice, subtags, extensionStart, i), "-");
        extensions.push(extension);
    }
    extensions.sort();
    var privateUse = "";
    if (i < subtags.length)
        privateUse = callFunction(std_Array_join, callFunction(std_Array_slice, subtags, i), "-");
    var canonical = normal;
    if (extensions.length > 0)
        canonical += "-" + extensions.join("-");
    if (privateUse.length > 0) {
        if (canonical.length > 0)
            canonical += "-" + privateUse;
        else
            canonical = privateUse;
    }
    return canonical;
}
var oldStyleLanguageTagMappings = {
    "pa-PK": "pa-Arab-PK",
    "zh-CN": "zh-Hans-CN",
    "zh-HK": "zh-Hant-HK",
    "zh-SG": "zh-Hans-SG",
    "zh-TW": "zh-Hant-TW"
};
function DefaultLocale() {
    var localeOfLastResort = "en-GB";
    var locale = RuntimeDefaultLocale();
    if (!IsStructurallyValidLanguageTag(locale))
        return localeOfLastResort;
    locale = CanonicalizeLanguageTag(locale);
    if (callFunction(std_Object_hasOwnProperty, oldStyleLanguageTagMappings, locale))
        locale = oldStyleLanguageTagMappings[locale];
    if (!(collatorInternalProperties.availableLocales()[locale] &&
          numberFormatInternalProperties.availableLocales()[locale] &&
          dateTimeFormatInternalProperties.availableLocales()[locale]))
    {
        locale = localeOfLastResort;
    }
    return locale;
}
function IsWellFormedCurrencyCode(currency) {
    var c = ToString(currency);
    var normalized = toASCIIUpperCase(c);
    if (normalized.length !== 3)
        return false;
    return !regexp_test_no_statics(/[^A-Z]/, normalized);
}
function addOldStyleLanguageTags(availableLocales) {
    var oldStyleLocales = std_Object_getOwnPropertyNames(oldStyleLanguageTagMappings);
    for (var i = 0; i < oldStyleLocales.length; i++) {
        var oldStyleLocale = oldStyleLocales[i];
        if (availableLocales[oldStyleLanguageTagMappings[oldStyleLocale]])
            availableLocales[oldStyleLocale] = true;
    }
    return availableLocales;
}
function CanonicalizeLocaleList(locales) {
    if (locales === undefined)
        return new List();
    var seen = new List();
    if (typeof locales === "string")
        locales = [locales];
    var O = ToObject(locales);
    var len = ((O.length) >>> 0);
    var k = 0;
    while (k < len) {
        var kPresent = HasProperty(O, k);
        if (kPresent) {
            var kValue = O[k];
            if (!(typeof kValue === "string" || IsObject(kValue)))
                ThrowError(310);
            var tag = ToString(kValue);
            if (!IsStructurallyValidLanguageTag(tag))
                ThrowError(309, tag);
            tag = CanonicalizeLanguageTag(tag);
            if (seen.indexOf(tag) === -1)
                seen.push(tag);
        }
        k++;
    }
    return seen;
}
function BestAvailableLocale(availableLocales, locale) {
    ;
    ;
    ;
    var candidate = locale;
    while (true) {
        if (availableLocales[candidate])
            return candidate;
        var pos = callFunction(std_String_lastIndexOf, candidate, "-");
        if (pos === -1)
            return undefined;
        if (pos >= 2 && candidate[pos - 2] === "-")
            pos -= 2;
        candidate = callFunction(std_String_substring, candidate, 0, pos);
    }
}
function LookupMatcher(availableLocales, requestedLocales) {
    var i = 0;
    var len = requestedLocales.length;
    var availableLocale;
    var locale, noExtensionsLocale;
    while (i < len && availableLocale === undefined) {
        locale = requestedLocales[i];
        noExtensionsLocale = removeUnicodeExtensions(locale);
        availableLocale = BestAvailableLocale(availableLocales, noExtensionsLocale);
        i++;
    }
    var result = new Record();
    if (availableLocale !== undefined) {
        result.locale = availableLocale;
        if (locale !== noExtensionsLocale) {
            var extensionMatch = regexp_exec_no_statics(unicodeLocaleExtensionSequenceRE, locale);
            var extension = extensionMatch[0];
            var extensionIndex = extensionMatch.index;
            result.extension = extension;
            result.extensionIndex = extensionIndex;
        }
    } else {
        result.locale = DefaultLocale();
    }
    return result;
}
function BestFitMatcher(availableLocales, requestedLocales) {
    return LookupMatcher(availableLocales, requestedLocales);
}
function ResolveLocale(availableLocales, requestedLocales, options, relevantExtensionKeys, localeData) {
    var matcher = options.localeMatcher;
    var r = (matcher === "lookup")
            ? LookupMatcher(availableLocales, requestedLocales)
            : BestFitMatcher(availableLocales, requestedLocales);
    var foundLocale = r.locale;
    var extension = r.extension;
    var extensionIndex, extensionSubtags, extensionSubtagsLength;
    if (extension !== undefined) {
        extensionIndex = r.extensionIndex;
        extensionSubtags = callFunction(std_String_split, extension, "-");
        extensionSubtagsLength = extensionSubtags.length;
    }
    var result = new Record();
    result.dataLocale = foundLocale;
    var supportedExtension = "-u";
    var i = 0;
    var len = relevantExtensionKeys.length;
    while (i < len) {
        var key = relevantExtensionKeys[i];
        var foundLocaleData = localeData(foundLocale);
        var keyLocaleData = foundLocaleData[key];
        var value = keyLocaleData[0];
        var supportedExtensionAddition = "";
        var valuePos;
        if (extensionSubtags !== undefined) {
            var keyPos = callFunction(std_Array_indexOf, extensionSubtags, key);
            if (keyPos !== -1) {
                if (keyPos + 1 < extensionSubtagsLength &&
                    extensionSubtags[keyPos + 1].length > 2)
                {
                    var requestedValue = extensionSubtags[keyPos + 1];
                    valuePos = callFunction(std_Array_indexOf, keyLocaleData, requestedValue);
                    if (valuePos !== -1) {
                        value = requestedValue;
                        supportedExtensionAddition = "-" + key + "-" + value;
                    }
                } else {
                    valuePos = callFunction(std_Array_indexOf, keyLocaleData, "true");
                    if (valuePos !== -1)
                        value = "true";
                }
            }
        }
        var optionsValue = options[key];
        if (optionsValue !== undefined &&
            callFunction(std_Array_indexOf, keyLocaleData, optionsValue) !== -1)
        {
            if (optionsValue !== value) {
                value = optionsValue;
                supportedExtensionAddition = "";
            }
        }
        result[key] = value;
        supportedExtension += supportedExtensionAddition;
        i++;
    }
    if (supportedExtension.length > 2) {
        var preExtension = callFunction(std_String_substring, foundLocale, 0, extensionIndex);
        var postExtension = callFunction(std_String_substring, foundLocale, extensionIndex);
        foundLocale = preExtension + supportedExtension + postExtension;
    }
    result.locale = foundLocale;
    return result;
}
function LookupSupportedLocales(availableLocales, requestedLocales) {
    var len = requestedLocales.length;
    var subset = new List();
    var k = 0;
    while (k < len) {
        var locale = requestedLocales[k];
        var noExtensionsLocale = removeUnicodeExtensions(locale);
        var availableLocale = BestAvailableLocale(availableLocales, noExtensionsLocale);
        if (availableLocale !== undefined)
            subset.push(locale);
        k++;
    }
    return subset.slice(0);
}
function BestFitSupportedLocales(availableLocales, requestedLocales) {
    return LookupSupportedLocales(availableLocales, requestedLocales);
}
function SupportedLocales(availableLocales, requestedLocales, options) {
    var matcher;
    if (options !== undefined) {
        options = ToObject(options);
        matcher = options.localeMatcher;
        if (matcher !== undefined) {
            matcher = ToString(matcher);
            if (matcher !== "lookup" && matcher !== "best fit")
                ThrowError(311, matcher);
        }
    }
    var subset = (matcher === undefined || matcher === "best fit")
                 ? BestFitSupportedLocales(availableLocales, requestedLocales)
                 : LookupSupportedLocales(availableLocales, requestedLocales);
    for (var i = 0; i < subset.length; i++) {
        _DefineDataProperty(subset, i, subset[i],
                            0x01 | 0x10 | 0x20);
    }
    _DefineDataProperty(subset, "length", subset.length,
                        0x08 | 0x10 | 0x20);
    return subset;
}
function GetOption(options, property, type, values, fallback) {
    var value = options[property];
    if (value !== undefined) {
        if (type === "boolean")
            value = ToBoolean(value);
        else if (type === "string")
            value = ToString(value);
        else
            ;
        if (values !== undefined && callFunction(std_Array_indexOf, values, value) === -1)
            ThrowError(312, property, value);
        return value;
    }
    return fallback;
}
function GetNumberOption(options, property, minimum, maximum, fallback) {
    ;
    ;
    ;
    var value = options[property];
    if (value !== undefined) {
        value = ToNumber(value);
        if (Number_isNaN(value) || value < minimum || value > maximum)
            ThrowError(308, value);
        return std_Math_floor(value);
    }
    return fallback;
}
function defineProperty(o, p, v) {
    _DefineDataProperty(o, p, v, 0x01 | 0x02 | 0x04);
}
var internalsMap = new WeakMap();
function initializeIntlObject(obj) {
    ;
    var internals = std_Object_create(null);
    internals.type = "partial";
    internals.lazyData = null;
    internals.internalProps = null;
    callFunction(std_WeakMap_set, internalsMap, obj, internals);
    return internals;
}
function setLazyData(internals, type, lazyData)
{
    ;
    ;
    ;
    internals.lazyData = lazyData;
    internals.type = type;
}
function setInternalProperties(internals, internalProps)
{
    ;
    ;
    ;
    internals.internalProps = internalProps;
    internals.lazyData = null;
}
function maybeInternalProperties(internals)
{
    ;
    ;
    var lazyData = internals.lazyData;
    if (lazyData)
        return null;
    ;
    return internals.internalProps;
}
function isInitializedIntlObject(obj) {
    return callFunction(std_WeakMap_has, internalsMap, obj);
}
function getIntlObjectInternals(obj, className, methodName) {
    ;
    var internals = callFunction(std_WeakMap_get, internalsMap, obj);
    ;
    if (internals === undefined || internals.type !== className)
        ThrowError(305, className, methodName, className);
    return internals;
}
function getInternals(obj)
{
    ;
    var internals = callFunction(std_WeakMap_get, internalsMap, obj);
    ;
    var lazyData = internals.lazyData;
    if (!lazyData)
        return internals.internalProps;
    var internalProps;
    var type = internals.type;
    if (type === "Collator")
        internalProps = resolveCollatorInternals(lazyData)
    else if (type === "DateTimeFormat")
        internalProps = resolveDateTimeFormatInternals(lazyData)
    else
        internalProps = resolveNumberFormatInternals(lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
}
var collatorKeyMappings = {
    kn: {property: "numeric", type: "boolean"},
    kf: {property: "caseFirst", type: "string", values: ["upper", "lower", "false"]}
};
function resolveCollatorInternals(lazyCollatorData)
{
    ;
    var internalProps = std_Object_create(null);
    internalProps.usage = lazyCollatorData.usage;
    var Collator = collatorInternalProperties;
    var collatorIsSorting = lazyCollatorData.usage === "sort";
    var localeData = collatorIsSorting
                     ? Collator.sortLocaleData
                     : Collator.searchLocaleData;
    var relevantExtensionKeys = Collator.relevantExtensionKeys;
    var r = ResolveLocale(Collator.availableLocales(),
                          lazyCollatorData.requestedLocales,
                          lazyCollatorData.opt,
                          relevantExtensionKeys,
                          localeData);
    internalProps.locale = r.locale;
    var key, property, value, mapping;
    var i = 0, len = relevantExtensionKeys.length;
    while (i < len) {
        key = relevantExtensionKeys[i];
        if (key === "co") {
            property = "collation";
            value = r.co === null ? "default" : r.co;
        } else {
            mapping = collatorKeyMappings[key];
            property = mapping.property;
            value = r[key];
            if (mapping.type === "boolean")
                value = value === "true";
        }
        internalProps[property] = value;
        i++;
    }
    var s = lazyCollatorData.rawSensitivity;
    if (s === undefined) {
        if (collatorIsSorting) {
            s = "variant";
        } else {
            var dataLocale = r.dataLocale;
            var dataLocaleData = localeData(dataLocale);
            s = dataLocaleData.sensitivity;
        }
    }
    internalProps.sensitivity = s;
    internalProps.ignorePunctuation = lazyCollatorData.ignorePunctuation;
    internalProps.boundFormat = undefined;
    return internalProps;
}
function getCollatorInternals(obj, methodName) {
    var internals = getIntlObjectInternals(obj, "Collator", methodName);
    ;
    var internalProps = maybeInternalProperties(internals);
    if (internalProps)
        return internalProps;
    internalProps = resolveCollatorInternals(internals.lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
}
function InitializeCollator(collator, locales, options) {
    ;
    if (isInitializedIntlObject(collator))
        ThrowError(306);
    var internals = initializeIntlObject(collator);
    var lazyCollatorData = std_Object_create(null);
    var requestedLocales = CanonicalizeLocaleList(locales);
    lazyCollatorData.requestedLocales = requestedLocales;
    if (options === undefined)
        options = {};
    else
        options = ToObject(options);
    var u = GetOption(options, "usage", "string", ["sort", "search"], "sort");
    lazyCollatorData.usage = u;
    var opt = new Record();
    lazyCollatorData.opt = opt;
    var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
    opt.localeMatcher = matcher;
    var numericValue = GetOption(options, "numeric", "boolean", undefined, undefined);
    if (numericValue !== undefined)
        numericValue = callFunction(std_Boolean_toString, numericValue);
    opt.kn = numericValue;
    var caseFirstValue = GetOption(options, "caseFirst", "string", ["upper", "lower", "false"], undefined);
    opt.kf = caseFirstValue;
    var s = GetOption(options, "sensitivity", "string",
                      ["base", "accent", "case", "variant"], undefined);
    lazyCollatorData.rawSensitivity = s;
    var ip = GetOption(options, "ignorePunctuation", "boolean", undefined, false);
    lazyCollatorData.ignorePunctuation = ip;
    setLazyData(internals, "Collator", lazyCollatorData);
}
function Intl_Collator_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;
    var availableLocales = collatorInternalProperties.availableLocales();
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}
var collatorInternalProperties = {
    sortLocaleData: collatorSortLocaleData,
    searchLocaleData: collatorSearchLocaleData,
    _availableLocales: null,
    availableLocales: function()
    {
        var locales = this._availableLocales;
        if (locales)
            return locales;
        return (this._availableLocales =
          addOldStyleLanguageTags(intl_Collator_availableLocales()));
    },
    relevantExtensionKeys: ["co", "kn"]
};
function collatorSortLocaleData(locale) {
    var collations = intl_availableCollations(locale);
    callFunction(std_Array_unshift, collations, null);
    return {
        co: collations,
        kn: ["false", "true"]
    };
}
function collatorSearchLocaleData(locale) {
    return {
        co: [null],
        kn: ["false", "true"],
        sensitivity: "variant"
    };
}
function collatorCompareToBind(x, y) {
    var X = ToString(x);
    var Y = ToString(y);
    return intl_CompareStrings(this, X, Y);
}
function Intl_Collator_compare_get() {
    var internals = getCollatorInternals(this, "compare");
    if (internals.boundCompare === undefined) {
        var F = collatorCompareToBind;
        var bc = callFunction(std_Function_bind, F, this);
        internals.boundCompare = bc;
    }
    return internals.boundCompare;
}
function Intl_Collator_resolvedOptions() {
    var internals = getCollatorInternals(this, "resolvedOptions");
    var result = {
        locale: internals.locale,
        usage: internals.usage,
        sensitivity: internals.sensitivity,
        ignorePunctuation: internals.ignorePunctuation
    };
    var relevantExtensionKeys = collatorInternalProperties.relevantExtensionKeys;
    for (var i = 0; i < relevantExtensionKeys.length; i++) {
        var key = relevantExtensionKeys[i];
        var property = (key === "co") ? "collation" : collatorKeyMappings[key].property;
        defineProperty(result, property, internals[property]);
    }
    return result;
}
var numberFormatInternalProperties = {
    localeData: numberFormatLocaleData,
    _availableLocales: null,
    availableLocales: function()
    {
        var locales = this._availableLocales;
        if (locales)
            return locales;
        return (this._availableLocales =
          addOldStyleLanguageTags(intl_NumberFormat_availableLocales()));
    },
    relevantExtensionKeys: ["nu"]
};
function resolveNumberFormatInternals(lazyNumberFormatData) {
    ;
    var internalProps = std_Object_create(null);
    var requestedLocales = lazyNumberFormatData.requestedLocales;
    var opt = lazyNumberFormatData.opt;
    var NumberFormat = numberFormatInternalProperties;
    var localeData = NumberFormat.localeData;
    var r = ResolveLocale(NumberFormat.availableLocales(),
                          lazyNumberFormatData.requestedLocales,
                          lazyNumberFormatData.opt,
                          NumberFormat.relevantExtensionKeys,
                          localeData);
    internalProps.locale = r.locale;
    internalProps.numberingSystem = r.nu;
    var s = lazyNumberFormatData.style;
    internalProps.style = s;
    if (s === "currency") {
        internalProps.currency = lazyNumberFormatData.currency;
        internalProps.currencyDisplay = lazyNumberFormatData.currencyDisplay;
    }
    internalProps.minimumIntegerDigits = lazyNumberFormatData.minimumIntegerDigits;
    internalProps.minimumFractionDigits = lazyNumberFormatData.minimumFractionDigits;
    internalProps.maximumFractionDigits = lazyNumberFormatData.maximumFractionDigits;
    if ("minimumSignificantDigits" in lazyNumberFormatData) {
        ;
        internalProps.minimumSignificantDigits = lazyNumberFormatData.minimumSignificantDigits;
        internalProps.maximumSignificantDigits = lazyNumberFormatData.maximumSignificantDigits;
    }
    internalProps.useGrouping = lazyNumberFormatData.useGrouping;
    internalProps.boundFormat = undefined;
    return internalProps;
}
function getNumberFormatInternals(obj, methodName) {
    var internals = getIntlObjectInternals(obj, "NumberFormat", methodName);
    ;
    var internalProps = maybeInternalProperties(internals);
    if (internalProps)
        return internalProps;
    internalProps = resolveNumberFormatInternals(internals.lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
}
function InitializeNumberFormat(numberFormat, locales, options) {
    ;
    if (isInitializedIntlObject(numberFormat))
        ThrowError(306);
    var internals = initializeIntlObject(numberFormat);
    var lazyNumberFormatData = std_Object_create(null);
    var requestedLocales = CanonicalizeLocaleList(locales);
    lazyNumberFormatData.requestedLocales = requestedLocales;
    if (options === undefined)
        options = {};
    else
        options = ToObject(options);
    var opt = new Record();
    lazyNumberFormatData.opt = opt;
    var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
    opt.localeMatcher = matcher;
    var s = GetOption(options, "style", "string", ["decimal", "percent", "currency"], "decimal");
    lazyNumberFormatData.style = s;
    var c = GetOption(options, "currency", "string", undefined, undefined);
    if (c !== undefined && !IsWellFormedCurrencyCode(c))
        ThrowError(307, c);
    var cDigits;
    if (s === "currency") {
        if (c === undefined)
            ThrowError(314);
        c = toASCIIUpperCase(c);
        lazyNumberFormatData.currency = c;
        cDigits = CurrencyDigits(c);
    }
    var cd = GetOption(options, "currencyDisplay", "string", ["code", "symbol", "name"], "symbol");
    if (s === "currency")
        lazyNumberFormatData.currencyDisplay = cd;
    var mnid = GetNumberOption(options, "minimumIntegerDigits", 1, 21, 1);
    lazyNumberFormatData.minimumIntegerDigits = mnid;
    var mnfdDefault = (s === "currency") ? cDigits : 0;
    var mnfd = GetNumberOption(options, "minimumFractionDigits", 0, 20, mnfdDefault);
    lazyNumberFormatData.minimumFractionDigits = mnfd;
    var mxfdDefault;
    if (s === "currency")
        mxfdDefault = std_Math_max(mnfd, cDigits);
    else if (s === "percent")
        mxfdDefault = std_Math_max(mnfd, 0);
    else
        mxfdDefault = std_Math_max(mnfd, 3);
    var mxfd = GetNumberOption(options, "maximumFractionDigits", mnfd, 20, mxfdDefault);
    lazyNumberFormatData.maximumFractionDigits = mxfd;
    var mnsd = options.minimumSignificantDigits;
    var mxsd = options.maximumSignificantDigits;
    if (mnsd !== undefined || mxsd !== undefined) {
        mnsd = GetNumberOption(options, "minimumSignificantDigits", 1, 21, 1);
        mxsd = GetNumberOption(options, "maximumSignificantDigits", mnsd, 21, 21);
        lazyNumberFormatData.minimumSignificantDigits = mnsd;
        lazyNumberFormatData.maximumSignificantDigits = mxsd;
    }
    var g = GetOption(options, "useGrouping", "boolean", undefined, true);
    lazyNumberFormatData.useGrouping = g;
    setLazyData(internals, "NumberFormat", lazyNumberFormatData);
}
var currencyDigits = {
    BHD: 3,
    BIF: 0,
    BYR: 0,
    CLF: 0,
    CLP: 0,
    DJF: 0,
    IQD: 3,
    GNF: 0,
    ISK: 0,
    JOD: 3,
    JPY: 0,
    KMF: 0,
    KRW: 0,
    KWD: 3,
    LYD: 3,
    OMR: 3,
    PYG: 0,
    RWF: 0,
    TND: 3,
    UGX: 0,
    UYI: 0,
    VND: 0,
    VUV: 0,
    XAF: 0,
    XOF: 0,
    XPF: 0
};
function CurrencyDigits(currency) {
    ;
    ;
    if (callFunction(std_Object_hasOwnProperty, currencyDigits, currency))
        return currencyDigits[currency];
    return 2;
}
function Intl_NumberFormat_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;
    var availableLocales = numberFormatInternalProperties.availableLocales();
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}
function getNumberingSystems(locale) {
    var defaultNumberingSystem = intl_numberingSystem(locale);
    return [
        defaultNumberingSystem,
        "arab", "arabext", "bali", "beng", "deva",
        "fullwide", "gujr", "guru", "hanidec", "khmr",
        "knda", "laoo", "latn", "limb", "mlym",
        "mong", "mymr", "orya", "tamldec", "telu",
        "thai", "tibt"
    ];
}
function numberFormatLocaleData(locale) {
    return {
        nu: getNumberingSystems(locale)
    };
}
function numberFormatFormatToBind(value) {
    var x = ToNumber(value);
    return intl_FormatNumber(this, x);
}
function Intl_NumberFormat_format_get() {
    var internals = getNumberFormatInternals(this, "format");
    if (internals.boundFormat === undefined) {
        var F = numberFormatFormatToBind;
        var bf = callFunction(std_Function_bind, F, this);
        internals.boundFormat = bf;
    }
    return internals.boundFormat;
}
function Intl_NumberFormat_resolvedOptions() {
    var internals = getNumberFormatInternals(this, "resolvedOptions");
    var result = {
        locale: internals.locale,
        numberingSystem: internals.numberingSystem,
        style: internals.style,
        minimumIntegerDigits: internals.minimumIntegerDigits,
        minimumFractionDigits: internals.minimumFractionDigits,
        maximumFractionDigits: internals.maximumFractionDigits,
        useGrouping: internals.useGrouping
    };
    var optionalProperties = [
        "currency",
        "currencyDisplay",
        "minimumSignificantDigits",
        "maximumSignificantDigits"
    ];
    for (var i = 0; i < optionalProperties.length; i++) {
        var p = optionalProperties[i];
        if (callFunction(std_Object_hasOwnProperty, internals, p))
            defineProperty(result, p, internals[p]);
    }
    return result;
}
function resolveDateTimeFormatInternals(lazyDateTimeFormatData) {
    ;
    var internalProps = std_Object_create(null);
    var DateTimeFormat = dateTimeFormatInternalProperties;
    var localeData = DateTimeFormat.localeData;
    var r = ResolveLocale(DateTimeFormat.availableLocales(),
                          lazyDateTimeFormatData.requestedLocales,
                          lazyDateTimeFormatData.localeOpt,
                          DateTimeFormat.relevantExtensionKeys,
                          localeData);
    internalProps.locale = r.locale;
    internalProps.calendar = r.ca;
    internalProps.numberingSystem = r.nu;
    var dataLocale = r.dataLocale;
    internalProps.timeZone = lazyDateTimeFormatData.timeZone;
    var formatOpt = lazyDateTimeFormatData.formatOpt;
    var pattern = toBestICUPattern(dataLocale, formatOpt);
    internalProps.pattern = pattern;
    internalProps.boundFormat = undefined;
    return internalProps;
}
function getDateTimeFormatInternals(obj, methodName) {
    var internals = getIntlObjectInternals(obj, "DateTimeFormat", methodName);
    ;
    var internalProps = maybeInternalProperties(internals);
    if (internalProps)
        return internalProps;
    internalProps = resolveDateTimeFormatInternals(internals.lazyData);
    setInternalProperties(internals, internalProps);
    return internalProps;
}
var dateTimeComponentValues = {
    weekday: ["narrow", "short", "long"],
    era: ["narrow", "short", "long"],
    year: ["2-digit", "numeric"],
    month: ["2-digit", "numeric", "narrow", "short", "long"],
    day: ["2-digit", "numeric"],
    hour: ["2-digit", "numeric"],
    minute: ["2-digit", "numeric"],
    second: ["2-digit", "numeric"],
    timeZoneName: ["short", "long"]
};
var dateTimeComponents = std_Object_getOwnPropertyNames(dateTimeComponentValues);
function InitializeDateTimeFormat(dateTimeFormat, locales, options) {
    ;
    if (isInitializedIntlObject(dateTimeFormat))
        ThrowError(306);
    var internals = initializeIntlObject(dateTimeFormat);
    var lazyDateTimeFormatData = std_Object_create(null);
    var requestedLocales = CanonicalizeLocaleList(locales);
    lazyDateTimeFormatData.requestedLocales = requestedLocales;
    options = ToDateTimeOptions(options, "any", "date");
    var localeOpt = new Record();
    lazyDateTimeFormatData.localeOpt = localeOpt;
    var localeMatcher =
        GetOption(options, "localeMatcher", "string", ["lookup", "best fit"],
                  "best fit");
    localeOpt.localeMatcher = localeMatcher;
    var tz = options.timeZone;
    if (tz !== undefined) {
        tz = toASCIIUpperCase(ToString(tz));
        if (tz !== "UTC")
            ThrowError(313, tz);
    }
    lazyDateTimeFormatData.timeZone = tz;
    var formatOpt = new Record();
    lazyDateTimeFormatData.formatOpt = formatOpt;
    var i, prop;
    for (i = 0; i < dateTimeComponents.length; i++) {
        prop = dateTimeComponents[i];
        var value = GetOption(options, prop, "string", dateTimeComponentValues[prop], undefined);
        formatOpt[prop] = value;
    }
    var formatMatcher =
        GetOption(options, "formatMatcher", "string", ["basic", "best fit"],
                  "best fit");
    var hr12 = GetOption(options, "hour12", "boolean", undefined, undefined);
    if (hr12 !== undefined)
        formatOpt.hour12 = hr12;
    setLazyData(internals, "DateTimeFormat", lazyDateTimeFormatData);
}
function toBestICUPattern(locale, options) {
    var skeleton = "";
    switch (options.weekday) {
    case "narrow":
        skeleton += "EEEEE";
        break;
    case "short":
        skeleton += "E";
        break;
    case "long":
        skeleton += "EEEE";
    }
    switch (options.era) {
    case "narrow":
        skeleton += "GGGGG";
        break;
    case "short":
        skeleton += "G";
        break;
    case "long":
        skeleton += "GGGG";
        break;
    }
    switch (options.year) {
    case "2-digit":
        skeleton += "yy";
        break;
    case "numeric":
        skeleton += "y";
        break;
    }
    switch (options.month) {
    case "2-digit":
        skeleton += "MM";
        break;
    case "numeric":
        skeleton += "M";
        break;
    case "narrow":
        skeleton += "MMMMM";
        break;
    case "short":
        skeleton += "MMM";
        break;
    case "long":
        skeleton += "MMMM";
        break;
    }
    switch (options.day) {
    case "2-digit":
        skeleton += "dd";
        break;
    case "numeric":
        skeleton += "d";
        break;
    }
    var hourSkeletonChar = "j";
    if (options.hour12 !== undefined) {
        if (options.hour12)
            hourSkeletonChar = "h";
        else
            hourSkeletonChar = "H";
    }
    switch (options.hour) {
    case "2-digit":
        skeleton += hourSkeletonChar + hourSkeletonChar;
        break;
    case "numeric":
        skeleton += hourSkeletonChar;
        break;
    }
    switch (options.minute) {
    case "2-digit":
        skeleton += "mm";
        break;
    case "numeric":
        skeleton += "m";
        break;
    }
    switch (options.second) {
    case "2-digit":
        skeleton += "ss";
        break;
    case "numeric":
        skeleton += "s";
        break;
    }
    switch (options.timeZoneName) {
    case "short":
        skeleton += "z";
        break;
    case "long":
        skeleton += "zzzz";
        break;
    }
    return intl_patternForSkeleton(locale, skeleton);
}
function ToDateTimeOptions(options, required, defaults) {
    ;
    ;
    if (options === undefined)
        options = null;
    else
        options = ToObject(options);
    options = std_Object_create(options);
    var needDefaults = true;
    if ((required === "date" || required === "any") &&
        (options.weekday !== undefined || options.year !== undefined ||
         options.month !== undefined || options.day !== undefined))
    {
        needDefaults = false;
    }
    if ((required === "time" || required === "any") &&
        (options.hour !== undefined || options.minute !== undefined ||
         options.second !== undefined))
    {
        needDefaults = false;
    }
    if (needDefaults && (defaults === "date" || defaults === "all")) {
        defineProperty(options, "year", "numeric");
        defineProperty(options, "month", "numeric");
        defineProperty(options, "day", "numeric");
    }
    if (needDefaults && (defaults === "time" || defaults === "all")) {
        defineProperty(options, "hour", "numeric");
        defineProperty(options, "minute", "numeric");
        defineProperty(options, "second", "numeric");
    }
    return options;
}
function BasicFormatMatcher(options, formats) {
    var removalPenalty = 120,
        additionPenalty = 20,
        longLessPenalty = 8,
        longMorePenalty = 6,
        shortLessPenalty = 6,
        shortMorePenalty = 3;
    var properties = ["weekday", "era", "year", "month", "day",
        "hour", "minute", "second", "timeZoneName"];
    var values = ["2-digit", "numeric", "narrow", "short", "long"];
    var bestScore = -Infinity;
    var bestFormat;
    var i = 0;
    var len = formats.length;
    while (i < len) {
        var format = formats[i];
        var score = 0;
        var formatProp;
        for (var j = 0; j < properties.length; j++) {
            var property = properties[j];
            var optionsProp = options[property];
            formatProp = undefined;
            if (callFunction(std_Object_hasOwnProperty, format, property))
                formatProp = format[property];
            if (optionsProp === undefined && formatProp !== undefined) {
                score -= additionPenalty;
            } else if (optionsProp !== undefined && formatProp === undefined) {
                score -= removalPenalty;
            } else {
                var optionsPropIndex = callFunction(std_Array_indexOf, values, optionsProp);
                var formatPropIndex = callFunction(std_Array_indexOf, values, formatProp);
                var delta = std_Math_max(std_Math_min(formatPropIndex - optionsPropIndex, 2), -2);
                if (delta === 2)
                    score -= longMorePenalty;
                else if (delta === 1)
                    score -= shortMorePenalty;
                else if (delta === -1)
                    score -= shortLessPenalty;
                else if (delta === -2)
                    score -= longLessPenalty;
            }
        }
        if (score > bestScore) {
            bestScore = score;
            bestFormat = format;
        }
        i++;
    }
    return bestFormat;
}
function BestFitFormatMatcher(options, formats) {
    return BasicFormatMatcher(options, formats);
}
function Intl_DateTimeFormat_supportedLocalesOf(locales ) {
    var options = arguments.length > 1 ? arguments[1] : undefined;
    var availableLocales = dateTimeFormatInternalProperties.availableLocales();
    var requestedLocales = CanonicalizeLocaleList(locales);
    return SupportedLocales(availableLocales, requestedLocales, options);
}
var dateTimeFormatInternalProperties = {
    localeData: dateTimeFormatLocaleData,
    _availableLocales: null,
    availableLocales: function()
    {
        var locales = this._availableLocales;
        if (locales)
            return locales;
        return (this._availableLocales =
          addOldStyleLanguageTags(intl_DateTimeFormat_availableLocales()));
    },
    relevantExtensionKeys: ["ca", "nu"]
};
function dateTimeFormatLocaleData(locale) {
    return {
        ca: intl_availableCalendars(locale),
        nu: getNumberingSystems(locale)
    };
}
function dateTimeFormatFormatToBind() {
    var date = arguments.length > 0 ? arguments[0] : undefined;
    var x = (date === undefined) ? std_Date_now() : ToNumber(date);
    return intl_FormatDateTime(this, x);
}
function Intl_DateTimeFormat_format_get() {
    var internals = getDateTimeFormatInternals(this, "format");
    if (internals.boundFormat === undefined) {
        var F = dateTimeFormatFormatToBind;
        var bf = callFunction(std_Function_bind, F, this);
        internals.boundFormat = bf;
    }
    return internals.boundFormat;
}
function Intl_DateTimeFormat_resolvedOptions() {
    var internals = getDateTimeFormatInternals(this, "resolvedOptions");
    var result = {
        locale: internals.locale,
        calendar: internals.calendar,
        numberingSystem: internals.numberingSystem,
        timeZone: internals.timeZone
    };
    resolveICUPattern(internals.pattern, result);
    return result;
}
var icuPatternCharToComponent = {
    E: "weekday",
    G: "era",
    y: "year",
    M: "month",
    L: "month",
    d: "day",
    h: "hour",
    H: "hour",
    k: "hour",
    K: "hour",
    m: "minute",
    s: "second",
    z: "timeZoneName",
    v: "timeZoneName",
    V: "timeZoneName"
};
function resolveICUPattern(pattern, result) {
    ;
    var i = 0;
    while (i < pattern.length) {
        var c = pattern[i++];
        if (c === "'") {
            while (i < pattern.length && pattern[i] !== "'")
                i++;
            i++;
        } else {
            var count = 1;
            while (i < pattern.length && pattern[i] === c) {
                i++;
                count++;
            }
            var value;
            switch (c) {
            case "G":
            case "E":
            case "z":
            case "v":
            case "V":
                if (count <= 3)
                    value = "short";
                else if (count === 4)
                    value = "long";
                else
                    value = "narrow";
                break;
            case "y":
            case "d":
            case "h":
            case "H":
            case "m":
            case "s":
            case "k":
            case "K":
                if (count === 2)
                    value = "2-digit";
                else
                    value = "numeric";
                break;
            case "M":
            case "L":
                if (count === 1)
                    value = "numeric";
                else if (count === 2)
                    value = "2-digit";
                else if (count === 3)
                    value = "short";
                else if (count === 4)
                    value = "long";
                else
                    value = "narrow";
                break;
            default:
            }
            if (callFunction(std_Object_hasOwnProperty, icuPatternCharToComponent, c))
                defineProperty(result, icuPatternCharToComponent[c], value);
            if (c === "h" || c === "K")
                defineProperty(result, "hour12", true);
            else if (c === "H" || c === "k")
                defineProperty(result, "hour12", false);
        }
    }
}
var langTagMappings = {
    "art-lojban": "jbo",
    "cel-gaulish": "cel-gaulish",
    "en-gb-oed": "en-GB-oed",
    "i-ami": "ami",
    "i-bnn": "bnn",
    "i-default": "i-default",
    "i-enochian": "i-enochian",
    "i-hak": "hak",
    "i-klingon": "tlh",
    "i-lux": "lb",
    "i-mingo": "i-mingo",
    "i-navajo": "nv",
    "i-pwn": "pwn",
    "i-tao": "tao",
    "i-tay": "tay",
    "i-tsu": "tsu",
    "ja-latn-hepburn-heploc": "ja-Latn-alalc97",
    "no-bok": "nb",
    "no-nyn": "nn",
    "sgn-be-fr": "sfb",
    "sgn-be-nl": "vgt",
    "sgn-br": "bzs",
    "sgn-ch-de": "sgg",
    "sgn-co": "csn",
    "sgn-de": "gsg",
    "sgn-dk": "dsl",
    "sgn-es": "ssp",
    "sgn-fr": "fsl",
    "sgn-gb": "bfi",
    "sgn-gr": "gss",
    "sgn-ie": "isg",
    "sgn-it": "ise",
    "sgn-jp": "jsl",
    "sgn-mx": "mfs",
    "sgn-ni": "ncs",
    "sgn-nl": "dse",
    "sgn-no": "nsl",
    "sgn-pt": "psr",
    "sgn-se": "swl",
    "sgn-us": "ase",
    "sgn-za": "sfs",
    "zh-cmn": "cmn",
    "zh-cmn-hans": "cmn-Hans",
    "zh-cmn-hant": "cmn-Hant",
    "zh-gan": "gan",
    "zh-guoyu": "cmn",
    "zh-hakka": "hak",
    "zh-min": "zh-min",
    "zh-min-nan": "nan",
    "zh-wuu": "wuu",
    "zh-xiang": "hsn",
    "zh-yue": "yue",
};
var langSubtagMappings = {
    "BU": "MM",
    "DD": "DE",
    "FX": "FR",
    "TP": "TL",
    "YD": "YE",
    "ZR": "CD",
    "ayx": "nun",
    "bjd": "drl",
    "ccq": "rki",
    "cjr": "mom",
    "cka": "cmr",
    "cmk": "xch",
    "drh": "khk",
    "drw": "prs",
    "gav": "dev",
    "hrr": "jal",
    "ibi": "opa",
    "in": "id",
    "iw": "he",
    "ji": "yi",
    "jw": "jv",
    "kgh": "kml",
    "lcq": "ppr",
    "mo": "ro",
    "mst": "mry",
    "myt": "mry",
    "sca": "hle",
    "tie": "ras",
    "tkk": "twm",
    "tlw": "weo",
    "tnf": "prs",
    "ybd": "rki",
    "yma": "lrr",
};
var extlangMappings = {
    "aao": {preferred: "aao", prefix: "ar"},
    "abh": {preferred: "abh", prefix: "ar"},
    "abv": {preferred: "abv", prefix: "ar"},
    "acm": {preferred: "acm", prefix: "ar"},
    "acq": {preferred: "acq", prefix: "ar"},
    "acw": {preferred: "acw", prefix: "ar"},
    "acx": {preferred: "acx", prefix: "ar"},
    "acy": {preferred: "acy", prefix: "ar"},
    "adf": {preferred: "adf", prefix: "ar"},
    "ads": {preferred: "ads", prefix: "sgn"},
    "aeb": {preferred: "aeb", prefix: "ar"},
    "aec": {preferred: "aec", prefix: "ar"},
    "aed": {preferred: "aed", prefix: "sgn"},
    "aen": {preferred: "aen", prefix: "sgn"},
    "afb": {preferred: "afb", prefix: "ar"},
    "afg": {preferred: "afg", prefix: "sgn"},
    "ajp": {preferred: "ajp", prefix: "ar"},
    "apc": {preferred: "apc", prefix: "ar"},
    "apd": {preferred: "apd", prefix: "ar"},
    "arb": {preferred: "arb", prefix: "ar"},
    "arq": {preferred: "arq", prefix: "ar"},
    "ars": {preferred: "ars", prefix: "ar"},
    "ary": {preferred: "ary", prefix: "ar"},
    "arz": {preferred: "arz", prefix: "ar"},
    "ase": {preferred: "ase", prefix: "sgn"},
    "asf": {preferred: "asf", prefix: "sgn"},
    "asp": {preferred: "asp", prefix: "sgn"},
    "asq": {preferred: "asq", prefix: "sgn"},
    "asw": {preferred: "asw", prefix: "sgn"},
    "auz": {preferred: "auz", prefix: "ar"},
    "avl": {preferred: "avl", prefix: "ar"},
    "ayh": {preferred: "ayh", prefix: "ar"},
    "ayl": {preferred: "ayl", prefix: "ar"},
    "ayn": {preferred: "ayn", prefix: "ar"},
    "ayp": {preferred: "ayp", prefix: "ar"},
    "bbz": {preferred: "bbz", prefix: "ar"},
    "bfi": {preferred: "bfi", prefix: "sgn"},
    "bfk": {preferred: "bfk", prefix: "sgn"},
    "bjn": {preferred: "bjn", prefix: "ms"},
    "bog": {preferred: "bog", prefix: "sgn"},
    "bqn": {preferred: "bqn", prefix: "sgn"},
    "bqy": {preferred: "bqy", prefix: "sgn"},
    "btj": {preferred: "btj", prefix: "ms"},
    "bve": {preferred: "bve", prefix: "ms"},
    "bvl": {preferred: "bvl", prefix: "sgn"},
    "bvu": {preferred: "bvu", prefix: "ms"},
    "bzs": {preferred: "bzs", prefix: "sgn"},
    "cdo": {preferred: "cdo", prefix: "zh"},
    "cds": {preferred: "cds", prefix: "sgn"},
    "cjy": {preferred: "cjy", prefix: "zh"},
    "cmn": {preferred: "cmn", prefix: "zh"},
    "coa": {preferred: "coa", prefix: "ms"},
    "cpx": {preferred: "cpx", prefix: "zh"},
    "csc": {preferred: "csc", prefix: "sgn"},
    "csd": {preferred: "csd", prefix: "sgn"},
    "cse": {preferred: "cse", prefix: "sgn"},
    "csf": {preferred: "csf", prefix: "sgn"},
    "csg": {preferred: "csg", prefix: "sgn"},
    "csl": {preferred: "csl", prefix: "sgn"},
    "csn": {preferred: "csn", prefix: "sgn"},
    "csq": {preferred: "csq", prefix: "sgn"},
    "csr": {preferred: "csr", prefix: "sgn"},
    "czh": {preferred: "czh", prefix: "zh"},
    "czo": {preferred: "czo", prefix: "zh"},
    "doq": {preferred: "doq", prefix: "sgn"},
    "dse": {preferred: "dse", prefix: "sgn"},
    "dsl": {preferred: "dsl", prefix: "sgn"},
    "dup": {preferred: "dup", prefix: "ms"},
    "ecs": {preferred: "ecs", prefix: "sgn"},
    "esl": {preferred: "esl", prefix: "sgn"},
    "esn": {preferred: "esn", prefix: "sgn"},
    "eso": {preferred: "eso", prefix: "sgn"},
    "eth": {preferred: "eth", prefix: "sgn"},
    "fcs": {preferred: "fcs", prefix: "sgn"},
    "fse": {preferred: "fse", prefix: "sgn"},
    "fsl": {preferred: "fsl", prefix: "sgn"},
    "fss": {preferred: "fss", prefix: "sgn"},
    "gan": {preferred: "gan", prefix: "zh"},
    "gds": {preferred: "gds", prefix: "sgn"},
    "gom": {preferred: "gom", prefix: "kok"},
    "gse": {preferred: "gse", prefix: "sgn"},
    "gsg": {preferred: "gsg", prefix: "sgn"},
    "gsm": {preferred: "gsm", prefix: "sgn"},
    "gss": {preferred: "gss", prefix: "sgn"},
    "gus": {preferred: "gus", prefix: "sgn"},
    "hab": {preferred: "hab", prefix: "sgn"},
    "haf": {preferred: "haf", prefix: "sgn"},
    "hak": {preferred: "hak", prefix: "zh"},
    "hds": {preferred: "hds", prefix: "sgn"},
    "hji": {preferred: "hji", prefix: "ms"},
    "hks": {preferred: "hks", prefix: "sgn"},
    "hos": {preferred: "hos", prefix: "sgn"},
    "hps": {preferred: "hps", prefix: "sgn"},
    "hsh": {preferred: "hsh", prefix: "sgn"},
    "hsl": {preferred: "hsl", prefix: "sgn"},
    "hsn": {preferred: "hsn", prefix: "zh"},
    "icl": {preferred: "icl", prefix: "sgn"},
    "ils": {preferred: "ils", prefix: "sgn"},
    "inl": {preferred: "inl", prefix: "sgn"},
    "ins": {preferred: "ins", prefix: "sgn"},
    "ise": {preferred: "ise", prefix: "sgn"},
    "isg": {preferred: "isg", prefix: "sgn"},
    "isr": {preferred: "isr", prefix: "sgn"},
    "jak": {preferred: "jak", prefix: "ms"},
    "jax": {preferred: "jax", prefix: "ms"},
    "jcs": {preferred: "jcs", prefix: "sgn"},
    "jhs": {preferred: "jhs", prefix: "sgn"},
    "jls": {preferred: "jls", prefix: "sgn"},
    "jos": {preferred: "jos", prefix: "sgn"},
    "jsl": {preferred: "jsl", prefix: "sgn"},
    "jus": {preferred: "jus", prefix: "sgn"},
    "kgi": {preferred: "kgi", prefix: "sgn"},
    "knn": {preferred: "knn", prefix: "kok"},
    "kvb": {preferred: "kvb", prefix: "ms"},
    "kvk": {preferred: "kvk", prefix: "sgn"},
    "kvr": {preferred: "kvr", prefix: "ms"},
    "kxd": {preferred: "kxd", prefix: "ms"},
    "lbs": {preferred: "lbs", prefix: "sgn"},
    "lce": {preferred: "lce", prefix: "ms"},
    "lcf": {preferred: "lcf", prefix: "ms"},
    "liw": {preferred: "liw", prefix: "ms"},
    "lls": {preferred: "lls", prefix: "sgn"},
    "lsg": {preferred: "lsg", prefix: "sgn"},
    "lsl": {preferred: "lsl", prefix: "sgn"},
    "lso": {preferred: "lso", prefix: "sgn"},
    "lsp": {preferred: "lsp", prefix: "sgn"},
    "lst": {preferred: "lst", prefix: "sgn"},
    "lsy": {preferred: "lsy", prefix: "sgn"},
    "ltg": {preferred: "ltg", prefix: "lv"},
    "lvs": {preferred: "lvs", prefix: "lv"},
    "lzh": {preferred: "lzh", prefix: "zh"},
    "max": {preferred: "max", prefix: "ms"},
    "mdl": {preferred: "mdl", prefix: "sgn"},
    "meo": {preferred: "meo", prefix: "ms"},
    "mfa": {preferred: "mfa", prefix: "ms"},
    "mfb": {preferred: "mfb", prefix: "ms"},
    "mfs": {preferred: "mfs", prefix: "sgn"},
    "min": {preferred: "min", prefix: "ms"},
    "mnp": {preferred: "mnp", prefix: "zh"},
    "mqg": {preferred: "mqg", prefix: "ms"},
    "mre": {preferred: "mre", prefix: "sgn"},
    "msd": {preferred: "msd", prefix: "sgn"},
    "msi": {preferred: "msi", prefix: "ms"},
    "msr": {preferred: "msr", prefix: "sgn"},
    "mui": {preferred: "mui", prefix: "ms"},
    "mzc": {preferred: "mzc", prefix: "sgn"},
    "mzg": {preferred: "mzg", prefix: "sgn"},
    "mzy": {preferred: "mzy", prefix: "sgn"},
    "nan": {preferred: "nan", prefix: "zh"},
    "nbs": {preferred: "nbs", prefix: "sgn"},
    "ncs": {preferred: "ncs", prefix: "sgn"},
    "nsi": {preferred: "nsi", prefix: "sgn"},
    "nsl": {preferred: "nsl", prefix: "sgn"},
    "nsp": {preferred: "nsp", prefix: "sgn"},
    "nsr": {preferred: "nsr", prefix: "sgn"},
    "nzs": {preferred: "nzs", prefix: "sgn"},
    "okl": {preferred: "okl", prefix: "sgn"},
    "orn": {preferred: "orn", prefix: "ms"},
    "ors": {preferred: "ors", prefix: "ms"},
    "pel": {preferred: "pel", prefix: "ms"},
    "pga": {preferred: "pga", prefix: "ar"},
    "pks": {preferred: "pks", prefix: "sgn"},
    "prl": {preferred: "prl", prefix: "sgn"},
    "prz": {preferred: "prz", prefix: "sgn"},
    "psc": {preferred: "psc", prefix: "sgn"},
    "psd": {preferred: "psd", prefix: "sgn"},
    "pse": {preferred: "pse", prefix: "ms"},
    "psg": {preferred: "psg", prefix: "sgn"},
    "psl": {preferred: "psl", prefix: "sgn"},
    "pso": {preferred: "pso", prefix: "sgn"},
    "psp": {preferred: "psp", prefix: "sgn"},
    "psr": {preferred: "psr", prefix: "sgn"},
    "pys": {preferred: "pys", prefix: "sgn"},
    "rms": {preferred: "rms", prefix: "sgn"},
    "rsi": {preferred: "rsi", prefix: "sgn"},
    "rsl": {preferred: "rsl", prefix: "sgn"},
    "sdl": {preferred: "sdl", prefix: "sgn"},
    "sfb": {preferred: "sfb", prefix: "sgn"},
    "sfs": {preferred: "sfs", prefix: "sgn"},
    "sgg": {preferred: "sgg", prefix: "sgn"},
    "sgx": {preferred: "sgx", prefix: "sgn"},
    "shu": {preferred: "shu", prefix: "ar"},
    "slf": {preferred: "slf", prefix: "sgn"},
    "sls": {preferred: "sls", prefix: "sgn"},
    "sqk": {preferred: "sqk", prefix: "sgn"},
    "sqs": {preferred: "sqs", prefix: "sgn"},
    "ssh": {preferred: "ssh", prefix: "ar"},
    "ssp": {preferred: "ssp", prefix: "sgn"},
    "ssr": {preferred: "ssr", prefix: "sgn"},
    "svk": {preferred: "svk", prefix: "sgn"},
    "swc": {preferred: "swc", prefix: "sw"},
    "swh": {preferred: "swh", prefix: "sw"},
    "swl": {preferred: "swl", prefix: "sgn"},
    "syy": {preferred: "syy", prefix: "sgn"},
    "tmw": {preferred: "tmw", prefix: "ms"},
    "tse": {preferred: "tse", prefix: "sgn"},
    "tsm": {preferred: "tsm", prefix: "sgn"},
    "tsq": {preferred: "tsq", prefix: "sgn"},
    "tss": {preferred: "tss", prefix: "sgn"},
    "tsy": {preferred: "tsy", prefix: "sgn"},
    "tza": {preferred: "tza", prefix: "sgn"},
    "ugn": {preferred: "ugn", prefix: "sgn"},
    "ugy": {preferred: "ugy", prefix: "sgn"},
    "ukl": {preferred: "ukl", prefix: "sgn"},
    "uks": {preferred: "uks", prefix: "sgn"},
    "urk": {preferred: "urk", prefix: "ms"},
    "uzn": {preferred: "uzn", prefix: "uz"},
    "uzs": {preferred: "uzs", prefix: "uz"},
    "vgt": {preferred: "vgt", prefix: "sgn"},
    "vkk": {preferred: "vkk", prefix: "ms"},
    "vkt": {preferred: "vkt", prefix: "ms"},
    "vsi": {preferred: "vsi", prefix: "sgn"},
    "vsl": {preferred: "vsl", prefix: "sgn"},
    "vsv": {preferred: "vsv", prefix: "sgn"},
    "wuu": {preferred: "wuu", prefix: "zh"},
    "xki": {preferred: "xki", prefix: "sgn"},
    "xml": {preferred: "xml", prefix: "sgn"},
    "xmm": {preferred: "xmm", prefix: "ms"},
    "xms": {preferred: "xms", prefix: "sgn"},
    "yds": {preferred: "yds", prefix: "sgn"},
    "ysl": {preferred: "ysl", prefix: "sgn"},
    "yue": {preferred: "yue", prefix: "zh"},
    "zib": {preferred: "zib", prefix: "sgn"},
    "zlm": {preferred: "zlm", prefix: "ms"},
    "zmi": {preferred: "zmi", prefix: "ms"},
    "zsl": {preferred: "zsl", prefix: "sgn"},
    "zsm": {preferred: "zsm", prefix: "ms"},
};
function IteratorIdentity() {
    return this;
}
var LegacyIteratorWrapperMap = new std_WeakMap();
function LegacyIteratorNext(arg) {
    var iter = callFunction(std_WeakMap_get, LegacyIteratorWrapperMap, this);
    try {
        return { value: iter.next(arg), done: false };
    } catch (e) {
        if (e instanceof std_StopIteration)
            return { value: undefined, done: true };
        throw e;
    }
}
function LegacyIteratorThrow(exn) {
    var iter = callFunction(std_WeakMap_get, LegacyIteratorWrapperMap, this);
    try {
        return { value: iter.throw(exn), done: false };
    } catch (e) {
        if (e instanceof std_StopIteration)
            return { value: undefined, done: true };
        throw e;
    }
}
function LegacyIterator(iter) {
    callFunction(std_WeakMap_set, LegacyIteratorWrapperMap, this, iter);
}
function LegacyGeneratorIterator(iter) {
    callFunction(std_WeakMap_set, LegacyIteratorWrapperMap, this, iter);
}
var LegacyIteratorsInitialized = std_Object_create(null);
function InitLegacyIterators() {
    var props = std_Object_create(null);
    props.next = std_Object_create(null);
    props.next.value = LegacyIteratorNext;
    props.next.enumerable = false;
    props.next.configurable = true;
    props.next.writable = true;
    props[std_iterator] = std_Object_create(null);
    props[std_iterator].value = IteratorIdentity;
    props[std_iterator].enumerable = false;
    props[std_iterator].configurable = true;
    props[std_iterator].writable = true;
    var LegacyIteratorProto = std_Object_create(GetIteratorPrototype(), props);
    MakeConstructible(LegacyIterator, LegacyIteratorProto);
    props.throw = std_Object_create(null);
    props.throw.value = LegacyIteratorThrow;
    props.throw.enumerable = false;
    props.throw.configurable = true;
    props.throw.writable = true;
    var LegacyGeneratorIteratorProto = std_Object_create(GetIteratorPrototype(), props);
    MakeConstructible(LegacyGeneratorIterator, LegacyGeneratorIteratorProto);
    LegacyIteratorsInitialized.initialized = true;
}
function NewLegacyIterator(iter, wrapper) {
    if (!LegacyIteratorsInitialized.initialized)
        InitLegacyIterators();
    return new wrapper(iter);
}
function LegacyIteratorShim() {
    return NewLegacyIterator(ToObject(this), LegacyIterator);
}
function LegacyGeneratorIteratorShim() {
    return NewLegacyIterator(ToObject(this), LegacyGeneratorIterator);
}
function MapForEach(callbackfn, thisArg = undefined) {
    var M = this;
    if (!IsObject(M))
        ThrowError(3, "Map", "forEach", typeof M);
    try {
        callFunction(std_Map_has, M);
    } catch (e) {
        ThrowError(3, "Map", "forEach", typeof M);
    }
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var entries = callFunction(std_Map_iterator, M);
    while (true) {
        var result = callFunction(std_Map_iterator_next, entries);
        if (result.done)
            break;
        var entry = result.value;
        callFunction(callbackfn, thisArg, entry[1], entry[0], M);
    }
}
var numberFormatCache = new Record();
function Number_toLocaleString() {
    var x = callFunction(std_Number_valueOf, this);
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;
    var numberFormat;
    if (locales === undefined && options === undefined) {
        if (numberFormatCache.numberFormat === undefined)
            numberFormatCache.numberFormat = intl_NumberFormat(locales, options);
        numberFormat = numberFormatCache.numberFormat;
    } else {
        numberFormat = intl_NumberFormat(locales, options);
    }
    return intl_FormatNumber(numberFormat, x);
}
function Number_isFinite(num) {
    if (typeof num !== "number")
        return false;
    return num - num === 0;
}
function Number_isNaN(num) {
    if (typeof num !== "number")
        return false;
    return num !== num;
}
function Number_isSafeInteger(number) {
    if (typeof number !== 'number')
        return false;
    if (!Number_isFinite(number))
        return false;
    var integer = ToInteger(number);
    if (integer !== number)
        return false;
    if (std_Math_abs(integer) <= 9007199254740991)
        return true;
    return false;
}
function Global_isNaN(number) {
    return Number_isNaN(ToNumber(number));
}
function Global_isFinite(number){
    return Number_isFinite(ToNumber(number));
}
function SetForEach(callbackfn, thisArg = undefined) {
    var S = this;
    if (!IsObject(S))
        ThrowError(3, "Set", "forEach", typeof S);
    try {
        callFunction(std_Set_has, S);
    } catch (e) {
        ThrowError(3, "Set", "forEach", typeof S);
    }
    if (!IsCallable(callbackfn))
        ThrowError(9, DecompileArg(0, callbackfn));
    var values = callFunction(std_Set_iterator, S);
    while (true) {
        var result = callFunction(std_Set_iterator_next, values);
        if (result.done)
            break;
        var value = result.value;
        callFunction(callbackfn, thisArg, value, value, S);
    }
}
function WeakSet_add(value) {
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(3, "WeakSet", "add", typeof S);
    if (!IsObject(value))
        ThrowError(38);
    let entries = UnsafeGetReservedSlot(this, 0);
    callFunction(std_WeakMap_set, entries, value, true);
    return S;
}
function WeakSet_clear() {
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(3, "WeakSet", "clear", typeof S);
    let entries = UnsafeGetReservedSlot(this, 0);
    callFunction(std_WeakMap_clear, entries);
    return undefined;
}
function WeakSet_delete(value) {
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(3, "WeakSet", "delete", typeof S);
    if (!IsObject(value))
        ThrowError(38);
    let entries = UnsafeGetReservedSlot(this, 0);
    return callFunction(std_WeakMap_delete, entries, value);
}
function WeakSet_has(value) {
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(3, "WeakSet", "has", typeof S);
    if (!IsObject(value))
        ThrowError(38);
    let entries = UnsafeGetReservedSlot(this, 0);
    return callFunction(std_WeakMap_has, entries, value);
}