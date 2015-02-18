// Copyright & License details are available under JXCORE_LICENSE file

/*
 At some point we was unable to see method body when calling toString().
 This unit is testing this behaviour.
 */


var jx = require('jxtools');
var assert = jx.assert;

var method = function () {
  var s = "method_body_should_be_visible";
};

var contains = method.toString().indexOf('var s = "method_body_should_be_visible"') > -1;

assert.ok(contains, "Body of the method is hidden, but should not be.")