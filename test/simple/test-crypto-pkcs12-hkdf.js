// Copyright & License details are available under JXCORE_LICENSE file

var crypto = require('crypto');
var fs = require('fs');

var publicKey = "BK4UlMX3EvD4Q16C9rIYWFoIAgzaeVCkfTArAQrWSvOe2jCJ1sxIG7guP0mqczpfoOuae0I7w4sftixlvasvXYs=";
console.log("publicKey: ", publicKey);
console.log("publicKey.length: ", publicKey.length);

var salt = new Buffer("abcdefgr");
console.log("salt: ", salt);
console.log("salt.length: ", salt.length);

var hash = crypto.createHash('SHA256');
hash.update(publicKey);

var digestBuffer = hash.digest();
console.log("digest: ", digestBuffer);
console.log("digest.length: ", digestBuffer.length);

//var digestBuffer = new Buffer(digest);

var bytesToGenerate = 32;
console.log("salt", salt);
var q = crypto.generateHKDF(bytesToGenerate, //int
  publicKey, salt, digestBuffer); //buffer-object, int

console.log("generateHKDF() returned: ", q);
console.log("generateHKDF() returned length: ", q.length);

var pkcs12Content = crypto.pkcs12.createBundle("password", "certname", "country", "organization");

console.log("C-API createPkcs12Bundle() returned length: ", pkcs12Content.length);

var ret = crypto.pkcs12.extractPublicKey("password", pkcs12Content);

console.log("C-API extractPublicKey() returned length: ", ret.length);
console.log("C-API extractPublicKey() returned: ", ret);

// FAIL Tests

var pass = false;
try {
  crypto.generateHKDF(0, //int
    publicKey, publicKey.length, //string, int
    salt, salt.length, //array-object, int
    digestBuffer, digestBuffer.length); //buffer-object, int
} catch (e) {
  pass = true;
}

if (!pass) {
  throw new Error("generateHKDF didn't raise exception 1. param");
}

for (var i = 1; i <= 3; i++) {
  pass = false;
  var arr = [bytesToGenerate, //int
    publicKey, salt, digestBuffer];
  arr[i] = "";
  try {
    crypto.generateHKDF.apply(null, arr); //buffer-object, int
  } catch (e) {
    console.log("EXCEPTED ERROR", e.message);
    pass = true;
  }

  if (!pass) {
    throw new Error("generateHKDF didn't raise exception " + i + " param");
  }
}

for (var i = 0; i < 4; i++) {
  pass = false;
  var arr = ["password", "certname", "country", "organization"];
  arr[i] = "";
  try {
    crypto.pkcs12.createBundle.apply(null, arr); //buffer-object, int
  } catch (e) {
    console.log("EXCEPTED ERROR", e.message);
    pass = true;
  }

  if (!pass) {
    throw new Error("pkcs12.createBundle didn't raise exception " + i + " param");
  }
}

pass = false;
try {
  crypto.pkcs12.extractPublicKey("password", " ");
} catch (e) {
  console.log("EXCEPTED ERROR", e.message);
  pass = true;
}

if (!pass) {
  throw new Error("extractPublicKey didn't raise exception 2. param");
}

pass = false;
try {
  crypto.pkcs12.extractPublicKey(" ", pkcs12Content);
} catch (e) {
  pass = true;
  console.log("EXCEPTED ERROR", e.message);
}

if (!pass) {
  throw new Error("extractPublicKey didn't raise exception 1. param");
}