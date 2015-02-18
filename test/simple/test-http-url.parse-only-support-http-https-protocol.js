// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var http = require('http');
var url = require('url');


assert.throws(function() {
  http.request(url.parse('file:///whatever'));
}, function(err) {
  if (err instanceof Error) {
    assert.strictEqual(err.message, 'Protocol:file: not supported.');
    return true;
  }
});

assert.throws(function() {
  http.request(url.parse('mailto:asdf@asdf.com'));
}, function(err) {
  if (err instanceof Error) {
    assert.strictEqual(err.message, 'Protocol:mailto: not supported.');
    return true;
  }
});

assert.throws(function() {
  http.request(url.parse('ftp://www.example.com'));
}, function(err) {
  if (err instanceof Error) {
    assert.strictEqual(err.message, 'Protocol:ftp: not supported.');
    return true;
  }
});

assert.throws(function() {
  http.request(url.parse('javascript:alert(\'hello\');'));
}, function(err) {
  if (err instanceof Error) {
    assert.strictEqual(err.message, 'Protocol:javascript: not supported.');
    return true;
  }
});

assert.throws(function() {
  http.request(url.parse('xmpp:isaacschlueter@jabber.org'));
}, function(err) {
  if (err instanceof Error) {
    assert.strictEqual(err.message, 'Protocol:xmpp: not supported.');
    return true;
  }
});

assert.throws(function() {
  http.request(url.parse('f://some.host/path'));
}, function(err) {
  if (err instanceof Error) {
    assert.strictEqual(err.message, 'Protocol:f: not supported.');
    return true;
  }
});

//TODO do I need to test url.parse(notPrococol.example.com)?