// Copyright & License details are available under JXCORE_LICENSE file


var print = function(obj, name) {
  console.log("\ttypeof", name, "=", typeof obj);
};

console.log("\nnamespace:", "globals");
print(Buffer, "Buffer");
print(GLOBAL, "GLOBAL");
print(clearInterval, "clearInterval");
print(clearTimeout, "clearTimeout");
print(console, "console");
print(global, "global");
print(jxcore, "jxcore");
print(process, "process");
print(root, "root");
print(setImmediate, "setImmediate");
print(setInterval, "setInterval");
print(setTimeout, "setTimeout");
