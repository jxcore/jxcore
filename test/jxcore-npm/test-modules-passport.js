// Copyright & License details are available under JXCORE_LICENSE file

var assert = require("assert");
var passport = require('passport');  // 1.2.1

assert.ok(passport, "The passport module was not loaded.");
assert.ok(passport.serializeUser, "There is no method passport.serializeUser.");
assert.ok(passport.use, "There is no method passport.use.");