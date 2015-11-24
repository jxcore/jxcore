// Copyright & License details are available under JXCORE_LICENSE file

var $uw = process.binding('memory_wrap');
var $tw = process.binding('thread_wrap');
var $tm = process.binding('jxutils_wrap');

var _store = {};

_store.exists = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  return $uw.existMap(process.threadId, key + '');
};

_store.set = function(key, str) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  if (!str && str !== 0 && str !== false) {
    throw new TypeError('null string can not be stored in the memory store');
  }

  str = Buffer.isBuffer(str) ? str : str + '';
  $uw.setMap(process.threadId, key.toString(), str);
};

_store.get = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  return $uw.getMap(process.threadId, key + '');
};

_store.getBuffer = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  return $uw.getMap(process.threadId, key + '', true);
};

_store.read = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  return $uw.readMap(process.threadId, key + '');
};

_store.readBuffer = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  return $uw.readMap(process.threadId, key + '', true);
};

_store.remove = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  $uw.removeMap(process.threadId, key + '');
};

_store.shared = {};

_store.shared.exists = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  return $uw.existsSource('#' + key + '');
};

_store.shared.set = function(key, str) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  if (!str && str !== 0 && str !== false) {
    throw new TypeError(
        'null object can not be stored in the shared memory store');
  }

  $uw.setSource('#' + key.toString(), Buffer.isBuffer(str) ? 
      str : str + '');
};

_store.shared.setIfEqualsTo = function(key, str, value) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  if (!str && str !== 0 && str !== false) {
    throw new TypeError(
        'null object can not be stored in the shared memory store');
  }
  if (!value && value !== 0 && value !== false) {
    throw 'null object can not be compared in the shared memory store';
  }

  str = Buffer.isBuffer(str) ? str : str + '';
  value = Buffer.isBuffer(value) ? value : value + '';
  return $uw.setSourceIfEqualsTo('#' + key, str, value);
};

_store.shared.setIfNotExists = function(key, str) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  if (!str && str !== 0 && str !== false) {
    throw new TypeError(
        'null object can not be stored in the shared memory store');
  }

  str = Buffer.isBuffer(str) ? str : str + '';
  return $uw.setSourceIfNotExists('#' + key, str);
};

if (_store.shared.setIfNotExists('|store.shared', '1')) {
  $uw.setMapCount($tw.cpuCount());
}

_store.shared.setIfEqualsToOrNull = function(key, str, value) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  if (!str && str !== 0 && str !== false) {
    throw new TypeError(
        'null object can not be stored in the shared memory store');
  }
  if (!value && value !== 0 && value !== false) {
    throw 'null object can not be compared in the shared memory store';
  }

  str = Buffer.isBuffer(str) ? str : str + '';
  value = Buffer.isBuffer(value) ? value : value + '';
  return $uw.setSourceIfEqualsToOrNull('#' + key, str, value);
};

_store.shared.get = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  return $uw.getSource('#' + key);
};

_store.shared.getBuffer = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  return $uw.getSource('#' + key, true);
};

_store.shared.read = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  return $uw.readSource('#' + key);
};

_store.shared.remove = function(key) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  $uw.removeSource('#' + key);
};

var $jxt = process.binding('jxtimers_wrap');
_store.shared.expires = function(key, timer) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }

  try {
    var x = parseInt(timer);
    if (!x || isNaN(x))
      throw new Error();
  } catch (e) {
    throw new TypeError('timer has to be a positive integer number');
  }

  if (!_store.shared.exists(key)) {
    throw new Error(key + ' was not exist inside the shared store');
  }

  var was_alive_ = $jxt.startWatcher();
  $tm.expirationSource('#' + key, timer);
  if (!was_alive_) {
    var temp_timer = setTimeout(function() {
      $jxt.forceCheckKeys();
    }, timer + 1);
    temp_timer.unref();
  }
};

exports.store = _store;

exports.store.shared.setBlockTimeout = function(total) {
  var n = 0;
  try {
    n = parseInt(total);
  } catch (e) {
    throw new TypeError('total may not be a number. ' + e);
  }

  if (n <= 0) {
    throw new TypeError('blockTimeout has to be a positive number');
  }

  $uw.setSource('{blockTimeout}', new Buffer(total + '', 'binary'));
};

exports.store.shared.getBlockTimeout = function() {
  var val = $uw.readSource('{blockTimeout}');

  return (val != null) ? parseInt(val) : 10000;
};

_store.shared.safeBlockSync = function(key, block, onerr) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  if (!block)
    return;

  if (!jxcore.utils.isFunction(block)) {
    return;
  }

  var val = -5;
  var name = key.toString();

  var pid = process.threadId;
  if (!pid) {
    pid = -6;
  }
  pid = pid.toString();

  if (!$uw.setSourceIfEqualsToOrNull('+' + name, pid + '', '-5')) {
    var dtBas = Date.now();

    var tim = exports.store.shared.getBlockTimeout();
    while (true) {
      if ($uw.setSourceIfEqualsTo('+' + name, pid + '', '-5')) {
        break;
      }

      if (Date.now() - dtBas > tim) {
        $uw.setSource('+' + name, new Buffer('-5', 'binary'));
        console.error('JXcore.SafeBlock recovered itself after ' + tim +
            ' seconds of waiting');
        dtBas = Date.now();
        tim = exports.store.shared.getBlockTimeout();
      }
    }
  }

  var ex = null;
  try {
    block();
  } catch (e) {
    ex = e;
  }

  $uw.setSource('+' + name, new Buffer('-5', 'binary'));

  if (onerr && ex != null)
    onerr(ex);
};

var blockQueue = {};
_store.shared.safeBlock = function(key, block, onerr) {
  if (!key && key !== 0 && key !== false) {
    throw new TypeError('key can not be null for memory store item');
  }
  if (!block)
    return;

  if (!jxcore.utils.isFunction(block)) {
    return;
  }

  var val = -5;
  var name = key.toString();

  var pid = process.threadId;
  if (!pid) {
    pid = -6;
  }
  pid = pid.toString();

  if (blockQueue[name]) {
    blockQueue[name].push([block, onerr]);
  } else {
    blockQueue[name] = [[block, onerr]];

    var dtBas = Date.now();
    var tim = exports.store.shared.getBlockTimeout();

    var handleQueue = function() {
      var timeout = false;
      if (tim > 0 && Date.now() - dtBas > tim) {
        timeout = true;
        console.error('JXcore.SafeBlock recovered itself after ' + tim +
            'ms of waiting');
        dtBas = Date.now();
      }
      if (blockQueue[name].length <= 0) {
        delete blockQueue[name];
      } else if (timeout ||
          $uw.setSourceIfEqualsToOrNull('+' + name, pid + '', '-5')) {
        for (var o in blockQueue[name]) {
          if (!blockQueue[name].hasOwnProperty(o))
            continue;

          var ex = null;
          var funcs = blockQueue[name][o];
          try {
            if (funcs && funcs[0]) {
              funcs[0]();
            }
          } catch (e) {
            ex = e;
          }

          if (funcs && funcs[1] && ex != null) {
            funcs[1](ex);
          }
        }
        delete blockQueue[name];
        $uw.setSource('+' + name, new Buffer('-5', 'binary'));
      } else {
        setTimeout(handleQueue, 1);
      }
    };
    setTimeout(handleQueue, 1);
  }
};
