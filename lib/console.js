// Copyright & License details are available under JXCORE_LICENSE file

var util = require('util');
var isSTD = (process.platform === 'android' || process.platform == 'winrt')
              && process.isEmbedded;
var $tw;
if (isSTD) {
  $tw = process.binding('jxutils_wrap');
}

function Console(stdout, stderr) {
  if (!(this instanceof Console)) {
    return new Console(stdout, stderr);
  }
  if (!isSTD) {
    if (!stdout || typeof stdout.write !== 'function') {
      throw new TypeError('Console expects a writable stream instance');
    }
  }
  if (!stderr) {
    stderr = stdout;
  }
  var prop = {
    writable: true,
    enumerable: false,
    configurable: true
  };
  prop.value = stdout;
  Object.defineProperty(this, '_stdout', prop);
  prop.value = stderr;
  Object.defineProperty(this, '_stderr', prop);
  prop.value = [];
  Object.defineProperty(this, '_times', prop);

  // bind the prototype functions to this Console instance
  Object.keys(Console.prototype).forEach(function(k) {
    this[k] = this[k].bind(this);
  }, this);
}

var customLogInterface = null, customErrorInterface = null;
Console.prototype.customInterface = function(log_interface, error_interface) {
  customLogInterface = log_interface;
  if (error_interface) {
    customErrorInterface = error_interface;
  } else {
    customErrorInterface = log_interface;
  }
};

Console.prototype._log = function(msg) {
  if (!isSTD) {
    this._stdout.write(msg);
  } else {
    $tw.print(msg);
  }
};

Console.prototype.log = function() {
  var msg = util.format.apply(this, arguments) + '\n';
  this._log(msg);

  if (customLogInterface) {
    try {
      customLogInterface(msg);
    } catch (e) {
    }
  }
};

Console.prototype.info = Console.prototype.log;

Console.prototype.warn = function() {
  var msg = util.format.apply(this, arguments) + '\n';
  if (!isSTD) {
    this._stderr.write(msg);
  } else {
    $tw.print_err_warn(msg, false);
  }

  if (customErrorInterface) {
    try {
      customErrorInterface(msg);
    } catch (e) {
    }
  }
};

Console.prototype.error = function() {
  var msg = util.format.apply(this, arguments) + '\n';
  if (!isSTD) {
    this._stderr.write(msg);
  } else {
    $tw.print_err_warn(msg, true);
  }

  if (customErrorInterface) {
    try {
      customErrorInterface(msg);
    } catch (e) {
    }
  }
};

Console.prototype.dir = function(object, options) {
  var formattedTxt = util.inspect(object, util._extend({
    customInspect: false
  }, options)) + '\n';

  if (!isSTD) {
    this._stdout.write(formattedTxt);
  } else {
    $tw.print(formattedTxt);
  }
};

function findTimeWithLabel(times, label) {
  if (!Array.isArray(times)) {
    jxcore.utils.console.log('Console.time/timeEnd: times must be an Array',
        'red');
    return;
  }

  if (typeof label !== 'string') {
    jxcore.utils.console.log('Console.time/timeEnd: label must be a string',
        'red');
    return;
  }

  var found;

  times.some(function findTime(item) {
    if (item.label === label) {
      found = item;
      return true;
    }
  });

  return found;
}

Console.prototype.time = function(label) {
  var timeEntry = findTimeWithLabel(this._times, label);
  var wallClockTime = Date.now();
  if (!timeEntry) {
    this._times.push({ label: label, time: wallClockTime });
  } else {
    timeEntry.time = wallClockTime;
  }
};

Console.prototype.timeEnd = function(label) {
  var wallClockTimeEnd = Date.now();
  var timeEntry = findTimeWithLabel(this._times, label);

  if (!timeEntry) {
    throw new Error('No such label: ' + label);
  } else {
    if (typeof timeEntry.time !== 'number') {
      jxcore.utils.console.log('Console.timeEnd: start time must be a number',
          'red');
      return;
    }
    var duration = wallClockTimeEnd - timeEntry.time;
    this.log('%s: %dms', label, duration);
  }
};

Console.prototype.trace = function() {
  var err = new Error;
  err.name = 'Trace';
  err.message = util.format.apply(this, arguments);
  Error.captureStackTrace(err, arguments.callee);
  if (process.versions.v8)
    this.error(err.stack);
  else if (process.versions.sm)
    this.error(err.stack.toString());
  else
    process
        .binding('jxutils_wrap')
        .print(
            "console.trace couldn't identify the actual JavaScript engine. " +
            "Did you overwrite 'process.versions' ?");
};

Console.prototype.assert = function(expression) {
  if (!expression) {
    var arr = Array.prototype.slice.call(arguments, 1);
    require('assert').ok(false, util.format.apply(this, arr));
  }
};

module.exports = new Console(process.stdout, process.stderr);
module.exports.Console = Console;
