// Copyright & License details are available under JXCORE_LICENSE file

var counter_ = 0;
var COUNTER_MAX = 1e6;
var letters = "abcdefghjklmnopqrstuvwxyz";
var actual_letter = 0, actual_letter_id = 0;
var getKey = function() { // 1 / 25M
  counter_++;
  if (counter_ >= COUNTER_MAX) {
    actual_letter = letters[actual_letter_id];
    actual_letter_id++;
    actual_letter_id %= letters.length;
  }

  return counter_ + actual_letter;
};

function JXTimer(_loopTimer) {
  var loopTimer = _loopTimer;
  var items = {};
  var timers = new Array();
  var current = 0;

  var totalEnrolled = 0;
  var gInterval = null;

  for (var i = 0; i < 500; i++) {
    timers.push({});
  }

  var checkKeys = function() {
    var tm = timers[current];

    for ( var o in tm) {
      if (!o) continue;

      var t = tm[o];

      if (!t) continue;
      tm[o] = null;

      totalEnrolled--;
      items[t.id] = null;
      delete items[t.id];

      t.host.___timerId = null;

      if (t.host._onTimeout) {
        t.host._onTimeout();
      } else if (t.host.emit) {
        t.host.emit('timeout');
      }
      t.host = null;
      t = null;
    }

    timers[current] = {};

    current++;
    current %= 500;

    if (totalEnrolled <= 0) {
      counter_ = 0;
      totalEnrolled = 0;
      global.clearInterval(gInterval);
      gInterval = null;
      actual_letter = 0;
      actual_letter_id = 0;
    }
  };

  this.enroll = function(item, tsecs) {
    if (!item || !item._onTimeout) { return; }

    var itemId = item.___timerId;

    if (itemId) {
      var obj_ = items[itemId];
      if (obj_) {
        var tloc = timers[obj_.location];
        tloc[obj_.id] = null;
        delete tloc[obj_.id];
        totalEnrolled--;
        items[itemId] = null;
      }
    } else {
      itemId = getKey();
      item.___timerId = itemId;
    }

    var loc = (tsecs + current) % 500;
    var obj = {
      id: itemId,
      host: item,
      location: loc,
      kick: tsecs
    };
    timers[loc][itemId] = obj;
    items[itemId] = obj;

    totalEnrolled++;
    if (totalEnrolled == 1 && gInterval == null) {
      gInterval = setInterval(checkKeys, loopTimer);
      gInterval.unref();
    }
  };

  this.unenroll = function(item) {
    var itemId = item.___timerId;
    if (!itemId) { return; }
    var obj_ = items[itemId];
    if (obj_) {
      var tloc = timers[obj_.location];
      tloc[obj_.id] = null;
      delete tloc[obj_.id];
      items[obj_.id] = null;
      delete items[obj_.id];
      totalEnrolled--;

      if (totalEnrolled <= 0) {
        counter_ = 0;
        totalEnrolled = 0;
        global.clearInterval(gInterval);
        gInterval = null;
        actual_letter = 0;
        actual_letter_id = 0;
      }
    }
    item.___timerId = null;
  };

  this._unrefActive = function(item) {
    var itemId = item.___timerId;
    if (!itemId) { return; }
    var obj_ = items[itemId];
    if (obj_) {
      var tloc = timers[obj_.location];
      tloc[obj_.id] = null;
      delete tloc[obj_.id];
      obj_.location = (obj_.kick + current) % 500;
      timers[obj_.location][obj_.id] = obj_;
    }
  };
}

var timerRange = 5000;
var timerInter = new JXTimer(timerRange);
var timer = require('timers'); // slow case - node.js compatibility

exports.enroll = function(item, msecs) {
  if (msecs >= timerRange) {
    if (item.__ec_int === false) {
      timer.unenroll(item);
    }

    item.__ec_int = true;
    timerInter.enroll(item, global.parseInt(msecs / timerRange));
  } else {
    if (item.__ec_int) {
      timerInter.unenroll(item);
    }

    item.__ec_int = false;
    timer.enroll(item, msecs);
  }
};

exports.unenroll = function(item) {
  if (item.__ec_int == undefined) { return; }

  if (item.__ec_int) {
    timerInter.unenroll(item);
  } else {
    timer.unenroll(item);
  }
};

exports._unrefActive = function(item) {
  if (item.__ec_int == undefined) { return; }

  if (item.__ec_int) {
    timerInter._unrefActive(item);
  } else {
    timer._unrefActive(item);
  }
};