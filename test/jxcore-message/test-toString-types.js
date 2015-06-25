// Copyright & License details are available under JXCORE_LICENSE file


var print = function(obj, name) {
  var str = "";
  if (obj === null) str = "[null]"; else
  if (obj === undefined) str = "[undefined]"; else
  str = obj.toString() || "[empty string]";
  console.log("\t`" + name + "` toString() =", str);
};

console.log("\nnamespace:", "types");
print("str", "some string");
print("", "empty string");

print(0, "number 0");
print(1, "number 1");
print(2.7, "number 2.7");

print(true, "boolean true");
print(false, "boolean false");

print([], "empty array");
print([ 1, "two"], "non empty array");

print({}, "empty object");
print({ x : 1 }, "non empty object");

print(null, "null");
print(undefined, "undefined");

print(function() {}, "function");
