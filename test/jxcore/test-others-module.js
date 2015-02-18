// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is checking if `require.main` equals to `module`
 */

var jx = require('jxtools');

if (require.main !== module) {
  jx.throwMT("`require.main` and  `module` are NOT equal!");
}