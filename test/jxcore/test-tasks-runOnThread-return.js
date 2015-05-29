// Copyright & License details are available under JXCORE_LICENSE file

/*
 This unit is testing tasks.runOnThread() and verifies if method was executed.
 */

var assert = require('assert');
var util = require("util");

var cnt = 5;
var finished = 0;

jxcore.tasks.setThreadCount(cnt);

function method(arg) {
  return arg;
}

var args = [1, "2", "three", {four: 1}, [5], ["six"]];

jxcore.tasks.runOnThread(0, method, 1, function (err, ret) {
  var expected = 1;
  assert.strictEqual(expected, ret, "method(" + expected + ") received wrong value: " + JSON.stringify(ret) + " instead of " + JSON.stringify(expected));
  finished++;
});


jxcore.tasks.runOnThread(1, method, "2", function (err, ret) {
  var expected = "2";
  assert.strictEqual(expected, ret, "method(" + expected + ") received wrong value: " + JSON.stringify(ret) + " instead of " + JSON.stringify(expected));
  finished++;
});


jxcore.tasks.runOnThread(2, method, "three", function (err, ret) {
  var expected = "three";
  assert.strictEqual(expected, ret, "method(" + expected + ") received wrong value: " + JSON.stringify(ret) + " instead of " + JSON.stringify(expected));
  finished++;
});


jxcore.tasks.runOnThread(3, method, {four: 1}, function (err, ret) {
  var expected = JSON.stringify({four: 1});
  var got = JSON.stringify(ret);

  assert.strictEqual(expected, got, "method(" + expected + ") received wrong value: " + got + " instead of " + expected);
  finished++;
});


jxcore.tasks.runOnThread(4, method, [5], function (err, ret) {
  var expected = JSON.stringify([5]);
  var got = JSON.stringify(ret);

  assert.strictEqual(expected, got, "method(" + expected + ") received wrong value: " + got + " instead of " + expected);
  finished++;
});


process.on('exit', function () {
  assert.strictEqual(finished, cnt, "Test did not finish! Some of the tasks were not executed.");
});
