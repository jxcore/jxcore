// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing if possible is to execute:
 require("../common.js")
 from inside task as method
 */

var assert = require('assert');

jxcore.tasks.on('message', function (threadId, msg) {
  assert.ifError(msg, msg);
});

var method = function () {
  try {
    var common = require("../common.js");
  } catch (ex) {
    process.sendToMain("Cannot require from inside addTask(method): " + ex + "\n__dirname = " + __dirname);
  }

  try {
    var common = require("./_asset_file.js");
  } catch (ex) {
    process.sendToMain("Cannot require ./_asset_file.js from inside addTask(method): " + ex + "\n__dirname = " + __dirname);
  }

  try {
    var common = require("./_asset_file");
  } catch (ex) {
    process.sendToMain("Cannot require ./_asset_file from inside addTask(method): " + ex + "\n__dirname = " + __dirname);
  }

  try {
    var common = require(__filename);
  } catch (ex) {
    process.sendToMain("Cannot require __filename from inside addTask(method): " + ex + "\n__filename = " + __filename);
  }

  try {
    var common = require('jxtools');
  } catch (ex) {
    process.sendToMain("Cannot require jxtools from inside addTask(method): " + ex + "\n__filename = " + __filename);
  }
};

// when in main thread
if (process.threadId == -1) {
  jxcore.tasks.addTask(method);
}