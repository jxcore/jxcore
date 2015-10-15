// Copyright & License details are available under JXCORE_LICENSE file

var util = require('util');
var net = require('net');
var FreeList = require('freelist').FreeList;
var HTTPParser = process.binding('http_parser').HTTPParser;
var Stream = require('stream');
var EventEmitter = require('events').EventEmitter;
var assert = require('assert').ok;

var _jx_helper = require('_jx_http_helper');

function readStart(socket) {
  if (!socket || !socket._handle || !socket._handle.readStart || socket._paused)
    return;

  socket._handle.readStart();
}

function readStop(socket) {
  if (!socket || !socket._handle || !socket._handle.readStop)
    return;

  socket._handle.readStop();
}

/* Abstract base class for ServerRequest and ClientResponse. */
var IncomingMessage = function(socket, info, url) {
  Stream.Readable.call(this);

  // XXX NOTE: This implementation is kind of all over the place.
  // When the parser emits body chunks, they go in this list.
  // _read() pulls them out, and when it finds EOF, it ends.

  this.socket = socket;
  this.connection = socket;

  if (info) {
    this.httpVersion = info.versionMajor + '.' + info.versionMinor;
    // request (server) only
    this.url = url;

    if (info.method) {
      // server only
      this.method = info.method;
      this.statusCode = null;
    } else {
      // client only
      this.method = null;
      this.statusCode = info.statusCode;
      // CHECKME dead code? we're always a request parser
    }

    this.upgrade = info.upgrade;

    this.httpVersionMajor = info.versionMajor;
    this.httpVersionMinor = info.versionMinor;
  }

  this.complete = false;
  this.headers = {};
  this.trailers = {};

  this.readable = true;

  this._pendings = new Array();
  this._pendingIndex = 0;

  // response (client) only

  this.client = this.socket;

  // flag for backwards compatibility grossness.
  this._consuming = false;

  // flag for when we decide that this message cannot possibly be
  // read by the user, so there's no point continuing to handle it.
  this._dumped = false;
};
util.inherits(IncomingMessage, Stream.Readable);

IncomingMessage.prototype.setTimeout = function(msecs, callback) {
  if (callback)
    this.on('timeout', callback);
  this.socket.setTimeout(msecs);
};

IncomingMessage.prototype.read = function(n) {
  this._consuming = true;
  this.read = Stream.Readable.prototype.read;

  return this.read(n);
};

IncomingMessage.prototype._read = function(n) {
  // We actually do almost nothing here, because the parserOnBody
  // function fills up our internal buffer directly. However, we
  // do need to unpause the underlying socket so that it flows.
  if (this.socket.readable)
    readStart(this.socket);
};

// It's possible that the socket will be destroyed, and removed from
// any messages, before ever calling this. In that case, just skip
// it, since something else is destroying this connection anyway.
IncomingMessage.prototype.destroy = function(error) {
  if (this.socket)
    this.socket.destroy(error);
};

var arrHeaders = [
  null, 'host', 'user-agent',
  'accept', 'accept-language', 'accept-encoding',
  'cookie', 'connection', 'cache-control',
  'set-cookie', // index 9 - must be maintained
  'accept-charset', 'link', 'pragma', 'www-authenticate',
  'proxy-authenticate', 'sec-websocket-extensions', 'sec-websocket-protocol'];

IncomingMessage.prototype._addHeaderLines = function(headers, n) {
  var dest = this.complete ? this.trailers : this.headers;

  for (var i = 0; i < n; i += 2) {
    var field = headers[i], value = headers[i + 1];

    if (field[0]) {
      if (field[0] == 9) { // set-cookie
        if (!dest['set-cookie']) {
          // don't use [] here ! JIT cant_compile
          var arr = new Array(1);
          arr[0] = value;
          dest['set-cookie'] = arr;
        } else
          dest['set-cookie'].push(value);
        continue;
      }

      var fname = arrHeaders[field[0]];
      if (!dest[fname])
        dest[fname] = value;
      else
        dest[fname] += ', ' + value;
      continue;
    }

    field = field[1].toLowerCase();
    if (dest[field] === undefined) {
      dest[field] = value;
      continue;
    }

    if (_jx_helper.isX(field)) {
      dest[field] += ', ' + value;
      continue;
    }

    switch (field) { // compatibility mode
      // Array headers:
      case 'host':
      case 'user-agent':
      case 'accept':
      case 'accept-charset':
      case 'accept-encoding':
      case 'accept-language':
      case 'cache-control':
      case 'connection':
      case 'cookie':
      case 'pragma':
      case 'link':
      case 'www-authenticate':
      case 'proxy-authenticate':
      case 'sec-websocket-extensions':
      case 'sec-websocket-protocol':
        dest[field] += ', ' + value;
        break;

      case 'set-cookie':
        if (!dest[field]) {
          // don't use [] here ! JIT cant_compile
          var arr = new Array(1);
          arr[0] = value;
          dest[field] = arr;
        } else {
          if (dest[field].substr) {
            // don't use [] here ! JIT cant_compile
            var arr = new Array(1);
            arr[0] = dest[field].toString();
            dest[field] = arr;
          }
          dest[field].push(value);
        }
        break;

      default:

        break;
    }
  }
};

// Add the given (field, value) pair to the message
//
// Per RFC2616, section 4.2 it is acceptable to join multiple instances of the
// same header with a ', ' if the header in question supports specification of
// multiple values this way. If not, we declare the first instance the winner
// and drop the second. Extended header fields (those beginning with 'x-') are
// always joined.
IncomingMessage.prototype._addHeaderLine = function(field, value) {
  var dest = this.complete ? this.trailers : this.headers;

  field = field.toLowerCase();
  switch (field) {
  // Array headers:
    case 'host':
    case 'user-agent':
    case 'accept':
    case 'accept-charset':
    case 'accept-encoding':
    case 'accept-language':
    case 'cache-control':
    case 'connection':
    case 'cookie':
    case 'pragma':
    case 'link':
    case 'www-authenticate':
    case 'proxy-authenticate':
    case 'sec-websocket-extensions':
    case 'sec-websocket-protocol':
      if (dest[field] !== undefined) {
        dest[field] += ', ' + value;
      } else {
        dest[field] = value;
      }
      break;

    case 'set-cookie':
      if (dest[field] !== undefined) {
        dest[field].push(value);
      } else {
        dest[field] = [value];
      }
      break;

    default:
      if (_jx_helper.isX(field)) {
        // except for x-
        if (dest[field] !== undefined) {
          dest[field] += ', ' + value;
        } else {
          dest[field] = value;
        }
      } else {
        // drop duplicates
        if (dest[field] === undefined)
          dest[field] = value;
      }
      break;
  }
};

// Call this instead of resume() if we want to just
// dump all the data to /dev/null
IncomingMessage.prototype._dump = function() {
  if (!this._dumped) {
    this._dumped = true;
    if (this.socket.parser)
      this.socket.parser.incoming = null;
    this.push(null);
    readStart(this.socket);
    this.read();
  }
};

exports.IncomingMessage = IncomingMessage;
