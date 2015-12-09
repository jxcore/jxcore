// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var util = require('util');
var tls = require('tls');

var tests = [
  // Basic CN handling
  { host: 'a.com', cert: { subject: { CN: 'a.com' } }, result: true },
  { host: 'a.com', cert: { subject: { CN: 'A.COM' } }, result: true },
  { host: 'a.com', cert: { subject: { CN: 'b.com' } }, result: false },
  { host: 'a.com', cert: { subject: { CN: 'a.com.' } }, result: true },

  // Wildcards in CN
  { host: 'b.a.com', cert: { subject: { CN: '*.a.com' } }, result: true },
  { host: 'b.a.com', cert: {
    subjectaltname: 'DNS:omg.com',
    subject: { CN: '*.a.com' } },
    result: false
  },

  // Multiple CN fields
  {
    host: 'foo.com', cert: {
      subject: { CN: ['foo.com', 'bar.com'] } // CN=foo.com; CN=bar.com;
    },
    result: true
  },

  // DNS names and CN
  {
    host: 'a.com', cert: {
      subjectaltname: 'DNS:*',
      subject: { CN: 'b.com' }
    },
    result: false
  },
  {
    host: 'a.com', cert: {
      subjectaltname: 'DNS:*.com',
      subject: { CN: 'b.com' }
    },
    result: false
  },
  {
    host: 'a.co.uk', cert: {
      subjectaltname: 'DNS:*.co.uk',
      subject: { CN: 'b.com' }
    },
    result: true
  },
  {
    host: 'a.com', cert: {
      subjectaltname: 'DNS:*.a.com',
      subject: { CN: 'a.com' }
    },
    result: false
  },
  {
    host: 'a.com', cert: {
      subjectaltname: 'DNS:*.a.com',
      subject: { CN: 'b.com' }
    },
    result: false
  },
  {
    host: 'a.com', cert: {
      subjectaltname: 'DNS:a.com',
      subject: { CN: 'b.com' }
    },
    result: true
  },
  {
    host: 'a.com', cert: {
      subjectaltname: 'DNS:A.COM',
      subject: { CN: 'b.com' }
    },
    result: true
  },

  // DNS names
  {
    host: 'a.com', cert: {
      subjectaltname: 'DNS:*.a.com',
      subject: {}
    },
    result: false
  },
  {
    host: 'b.a.com', cert: {
      subjectaltname: 'DNS:*.a.com',
      subject: {}
    },
    result: true
  },
  {
    host: 'c.b.a.com', cert: {
      subjectaltname: 'DNS:*.a.com',
      subject: {}
    },
    result: false
  },
  {
    host: 'b.a.com', cert: {
      subjectaltname: 'DNS:*b.a.com',
      subject: {}
    },
    result: true
  },
  {
    host: 'a-cb.a.com', cert: {
      subjectaltname: 'DNS:*b.a.com',
      subject: {}
    },
    result: true
  },
  {
    host: 'a.b.a.com', cert: {
      subjectaltname: 'DNS:*b.a.com',
      subject: {}
    },
    result: false
  },
  // Mutliple DNS names
  {
    host: 'a.b.a.com', cert: {
      subjectaltname: 'DNS:*b.a.com, DNS:a.b.a.com',
      subject: {}
    },
    result: true
  },
  // URI names
  {
    host: 'a.b.a.com', cert: {
      subjectaltname: 'URI:http://a.b.a.com/',
      subject: {}
    },
    result: true
  },
  {
    host: 'a.b.a.com', cert: {
      subjectaltname: 'URI:http://*.b.a.com/',
      subject: {}
    },
    result: false
  },
  // IP addresses
  {
    host: 'a.b.a.com', cert: {
      subjectaltname: 'IP Address:127.0.0.1',
      subject: {}
    },
    result: false
  },
  {
    host: '127.0.0.1', cert: {
      subjectaltname: 'IP Address:127.0.0.1',
      subject: {}
    },
    result: true
  },
  {
    host: '127.0.0.2', cert: {
      subjectaltname: 'IP Address:127.0.0.1',
      subject: {}
    },
    result: false
  },
  {
    host: '127.0.0.1', cert: {
      subjectaltname: 'DNS:a.com',
      subject: {}
    },
    result: false
  },
  {
    host: 'localhost', cert: {
      subjectaltname: 'DNS:a.com',
      subject: { CN: 'localhost' }
    },
    result: false
  },
];

tests.forEach(function(test, i) {
  assert.equal(tls.checkServerIdentity(test.host, test.cert),
               test.result,
               'Test#' + i + ' failed: ' + util.inspect(test));
});