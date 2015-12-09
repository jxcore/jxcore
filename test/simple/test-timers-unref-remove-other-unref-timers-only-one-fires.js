// Copyright & License details are available under JXCORE_LICENSE file


/*
 * The goal of this test is to make sure that, after the regression introduced
 * by 934bfe23a16556d05bfb1844ef4d53e8c9887c3d, the fix preserves the following
 * behavior of unref timers: if two timers are scheduled to fire at the same
 * time, if one unenrolls the other one in its _onTimeout callback, the other
 * one will *not* fire.
 *
 * This behavior is a private implementation detail and should not be
 * considered public interface.
 */
var timers = require('timers');
var assert = require('assert');

var nbTimersFired = 0;

var foo = new function() {
  this._onTimeout = function() {
    ++nbTimersFired;
    timers.unenroll(bar);
  };
}();

var bar = new function() {
  this._onTimeout = function() {
    ++nbTimersFired;
    timers.unenroll(foo);
  };
}();

timers.enroll(bar, 1);
timers._unrefActive(bar);

timers.enroll(foo, 1);
timers._unrefActive(foo);

setTimeout(function() {
  assert.notEqual(nbTimersFired, 2);
}, 20);
