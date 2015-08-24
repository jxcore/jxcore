// Copyright & License details are available under JXCORE_LICENSE file

var domain;

exports.usingDomains = false;

// By default EventEmitters will print a warning if more than
// 10 listeners are added to it. This is a useful default which
// helps finding memory leaks.
//
// Obviously not all Emitters should be limited to 10. This function allows
// that to be increased. Set to zero for unlimited.
var defaultMaxListeners = 10;

function EventEmitter() {
  this.domain = null;
  if (exports.usingDomains) {
    // if there is an active domain, then attach to it.
    domain = domain || require('domain');
    if (domain.active && !(this instanceof domain.Domain)) {
      this.domain = domain.active;
    }
  }
  this._events = this._events || {};
  this._maxListeners = this._maxListeners || defaultMaxListeners;
}

exports.EventEmitter = EventEmitter;

exports.EventEmitter.prototype.setMaxListeners = function setMaxListeners(n) {
  if (typeof n !== 'number' || n < 0 || isNaN(n))
    throw new TypeError('n must be a positive number');
  this._maxListeners = n;
};

exports.EventEmitter.prototype.emit = function emit(type, arg1, arg2) {
  var skip = false;
  if (!this._events) {
    this._events = {};
    skip = true;
  }

  // If there is no 'error' event listener then throw.
  if (type === 'error') {
    var eo = this._events.error;
    if (!eo || (typeof eo === 'object' && !eo.length)) {
      if (this.domain) {
        if (!arg1)
          arg1 = new TypeError('Uncaught, unspecified "error" event.');
        arg1.domainEmitter = this;
        arg1.domain = this.domain;
        arg1.domainThrown = false;
        this.domain.emit('error', arg1);
      } else if (arg1 instanceof Error) {
        throw arg1; // Unhandled 'error' event
      } else {
        throw new TypeError('Uncaught, unspecified "error" event.');
      }
      return false;
    }
  }

  if (skip || typeof type == 'undefined' || type === null) return false;
  if (!this._events.hasOwnProperty(type)) return false;
  
  var args, listeners;
  
  var handler = this._events[type];
  var tph = (typeof handler);
  if (tph === 'undefined') return false;

  var domain_handler = (this.domain && this !== global.process);
  if (domain_handler) this.domain.enter();

  if (tph === 'function') {
    switch (arguments.length) {
    // fast cases
    case 1:
      handler.call(this);
      break;
    case 2:
      handler.call(this, arg1);
      break;
    case 3:
      handler.call(this, arg1, arg2);
      break;
    // slower
    default:
      args = Array.prototype.slice.call(arguments, 1);
      handler.apply(this, args);
    }
  } else if (Array.isArray(handler)) {
    args = Array.prototype.slice.call(arguments, 1);

    listeners = handler.slice();
    var len = listeners.length;
    for (var i = 0; i < len; i++)
      listeners[i].apply(this, args);
  }

  if (domain_handler) this.domain.exit();

  return true;
};

EventEmitter.prototype.addListener = function addListener(type, listener) {
  if (typeof listener !== 'function')
    throw new TypeError('listener must be a function');

  if (!this._events)
    this._events = {};
  else if (this._events.newListener)
    this.emit('newListener', type, typeof listener.listener === 'function'
            ? listener.listener : listener);

  if (!this._events.hasOwnProperty(type))
    // Optimize the case of one listener. Don't need the extra array object.
    this._events[type] = listener;
  else if (Array.isArray(this._events[type])) {
    // If we've already got an array, just append.
    this._events[type].push(listener);
  } else {
    // Adding the second element, need to change to array.
    var arr = new Array(2);
    arr[0] = this._events[type];
    arr[1] = listener;
    this._events[type] = arr;
  }

  // Check for listener leak
  var et = this._events[type];
  if (Array.isArray(et) && !et.warned) {
    var m = this._maxListeners;
    if (m && m > 0 && et.length > m) {
      et.warned = true;
      console.error('(node) warning: possible EventEmitter memory '
              + 'leak detected. %d listeners added. '
              + 'Use emitter.setMaxListeners() to increase limit.', et.length);
      console.trace();
    }
  }

  return this;
};

EventEmitter.prototype.on = EventEmitter.prototype.addListener;

EventEmitter.prototype.once = function once(type, listener) {
  if (typeof listener !== 'function')
    throw new TypeError('listener must be a function');

  var fired = false;

  function g() {
    this.removeListener(type, g);

    if (!fired) {
      fired = true;
      listener.apply(this, arguments);
    }
  }

  g.listener = listener;
  this.on(type, g);

  return this;
};

// emits a 'removeListener' event iff the listener was removed
EventEmitter.prototype.removeListener = function removeListener(type, listener) {
  var list, position, length, i;

  if (typeof listener !== 'function')
    throw new TypeError('listener must be a function');

  if (!this._events || !this._events.hasOwnProperty(type)) return this;

  list = this._events[type];
  length = list.length;
  position = -1;

  if (list === listener
          || (typeof list.listener === 'function' && list.listener === listener)) {
    delete this._events[type];
    if (this._events.removeListener)
      this.emit('removeListener', type, listener);

  } else if (typeof list === 'object') {
    for (i = length; i-- > 0;) {
      if (list[i] === listener
              || (list[i].listener && list[i].listener === listener)) {
        position = i;
        break;
      }
    }

    if (position < 0) return this;

    if (list.length === 1) {
      list.length = 0;
      delete this._events[type];
    } else {
      list.splice(position, 1);
    }

    if (this._events.removeListener)
      this.emit('removeListener', type, listener);
  }

  return this;
};

EventEmitter.prototype.removeAllListeners = function removeAllListeners(type) {
  var key, listeners;

  if (!this._events) return this;

  // not listening for removeListener, no need to emit
  if (!this._events.removeListener) {
    if (arguments.length === 0)
      this._events = {};
    else if (this._events[type]) delete this._events[type];
    return this;
  }

  // emit removeListener for all listeners on all events
  if (arguments.length === 0) {
    for (key in this._events) {
      if (key === 'removeListener' || !this._events.hasOwnProperty(key)) continue;
      this.removeAllListeners(key);
    }
    this.removeAllListeners('removeListener');
    this._events = {};
    return this;
  }

  listeners = this._events[type];

  if (typeof listeners === 'function') {
    this.removeListener(type, listeners);
  } else if (Array.isArray(listeners)) {
    // LIFO order
    while (listeners.length)
      this.removeListener(type, listeners[listeners.length - 1]);
  }
  delete this._events[type];

  return this;
};

EventEmitter.prototype.listeners = function listeners(type) {
  var ret;
  if (!this._events || !this._events[type])
    ret = new Array();
  else if (typeof this._events[type] === 'function')
    ret = [this._events[type]];
  else
    ret = this._events[type].slice();
  return ret;
};

EventEmitter.listenerCount = function(emitter, type) {
  var ret;
  if (!emitter._events || !emitter._events[type])
    ret = 0;
  else if (typeof emitter._events[type] === 'function')
    ret = 1;
  else
    ret = emitter._events[type].length;
  return ret;
};