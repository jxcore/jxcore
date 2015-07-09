// Copyright & License details are available under JXCORE_LICENSE file

var util = require('util');
var net = require('net');
var FreeList = require('freelist').FreeList;
var HTTPParser = process.binding('http_parser').HTTPParser;
var Stream = require('stream');
var EventEmitter = require('events').EventEmitter;
var IncomingMessage = require('_jx_incoming').IncomingMessage;
var assert = require('assert').ok;

// Only called in the slow case where slow means
// that the request headers were either fragmented
// across multiple TCP packets or too large to be
// processed in a single run. This method is also
// called to process trailing HTTP headers.
function parserOnHeaders(headers, url) {// pin("PS- parserOnHeaders");
  // Once we exceeded headers limit - stop collecting them
  if (this.maxHeaderPairs <= 0 || this._headers.length < this.maxHeaderPairs) {
    this._headers = this._headers.concat(headers);
  }
  this._url += url;
}

// info.headers and info.url are set only if .onHeaders()
// has not been called for this request.
//
// info.url is not set for response parsers but that's not
// applicable here since all our parsers are request parsers.
function parserOnHeadersComplete(info) {
  var parser = this;
  var headers;
  var url = info.url;

  if (!info.headers) {
    headers = parser._headers;
    parser._headers = new Array();
  } else
    headers = info.headers;

  if (!url) {
    url = parser._url;
    parser._url = '';
  }

  parser.incoming = new IncomingMessage(parser.socket, info, url);

  var n = headers.length;

  // If parser.maxHeaderPairs <= 0 - assume that there're no limit
  if (parser.maxHeaderPairs > 0) {
    n = Math.min(n, parser.maxHeaderPairs);
  }

  parser.incoming._addHeaderLines(headers, n);
  var skipBody = false; // response to HEAD or CONNECT

  if (!info.upgrade) {
    // For upgraded connections and CONNECT method request,
    // we'll emit this after parser.execute
    // so that we can capture the first part of the new protocol
    skipBody = parser.onIncoming(parser.incoming, info.shouldKeepAlive);
  }

  return skipBody;
}

// XXX This is a mess.
// TODO: http.Parser should be a Writable emits request/response events.
function parserOnBody(b, len) {
  var parser = this;
  var stream = parser.incoming;

  // if the stream has already been removed, then drop it.
  if (!stream) {
    return;
  }

  var socket = stream.socket;

  // pretend this was the result of a stream._read call.
  if (len > 0 && !stream._dumped) {
    var ret = stream.push(b);
    if (!ret)
      readStop(socket);
  }
}

function parserOnMessageComplete() {
  var parser = this;
  var stream = parser.incoming;

  if (stream) {
    stream.complete = true;
    // Emit any trailing headers.
    var headers = parser._headers;
    if (headers) {
      parser.incoming._addHeaderLines(headers, headers.length);
      parser._headers = new Array();
      parser._url = '';
    }

    if (!stream.upgrade) {
      // For upgraded connections, also emit this after parser.execute
      stream.push(null);
    }
  }

  if (stream && !parser.incoming._pendings.length) {
    // For emit end event
    stream.push(null);
  }

  // force to read the next incoming message
  readStart(parser.socket);
}

var parsers = new FreeList('parsers', 1000, function() {
  var parser = new HTTPParser(HTTPParser.REQUEST, true);

  parser._headers = new Array();
  parser._url = '';

  // Only called in the slow case where slow means
  // that the request headers were either fragmented
  // across multiple TCP packets or too large to be
  // processed in a single run. This method is also
  // called to process trailing HTTP headers.
  parser.onHeaders = parserOnHeaders;
  parser.onHeadersComplete = parserOnHeadersComplete;
  parser.onBody = parserOnBody;
  parser.onMessageComplete = parserOnMessageComplete;

  return parser;
});

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

parsers.create = function(req, socket) {
  var parser = parsers.alloc();
  parser.__ptype = req;
  parser.socket = socket;
  parser.incoming = null;

  return parser;
}

exports.parsers = parsers;
