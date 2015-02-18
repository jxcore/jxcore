// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing accessibility of http.STATUS_CODES, because at some point it was unaccessible
 */

var jx = require('jxtools');
var assert = jx.assert;
var http = require("http");

var codes = http.STATUS_CODES;

assert(codes, "Cannot access http.STATUS_CODES");
assert.strictEqual(codes["200"], "OK", "No http status code 200 defined");


