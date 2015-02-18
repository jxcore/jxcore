// Copyright & License details are available under JXCORE_LICENSE file

var assert = require("assert");
var express = require('express');  // 4.9.5.
var app = express();

assert.strictEqual(!!app, true, "Cannot get express() instance.");
