// Copyright & License details are available under JXCORE_LICENSE file





var common = require('../common');
var assert = require('assert');

var dns = require('dns'),
    child_process = require('child_process');


// Try resolution without callback

assert.throws(function() {
  dns.resolve('google.com', 'A');
});
assert.throws(function() {
  dns.resolve('127.0.0.1', 'PTR');
});


var hosts = ['example.com',
             'example.org',
             'ietf.org', // AAAA
             'google.com', // MX, multiple A records
             '_xmpp-client._tcp.google.com', // SRV
             'oakalynhall.co.uk' // Multiple PTR replies
];

var records = ['A', 'AAAA', 'MX', 'TXT', 'SRV'];

var i = hosts.length;
while (i--) {

  var j = records.length;
  while (j--) {
    var hostCmd = 'dig -t ' + records[j] + ' ' + hosts[i] +
                  '| grep "^' + hosts[i] + '\\.\\W.*IN.*' + records[j] + '"' +
                  '| sed -E "s/[[:space:]]+/ /g" | cut -d " " -f 5- ' +
                  '| sed -e "s/\\.$//"';
    child_process.exec(hostCmd, checkDnsRecord(hosts[i], records[j]));
  }
}

// CNAME should resolve
var resolveCNAME = 'before';
dns.resolve('labs.nrcmedia.nl', 'CNAME', function(err, result) {
  assert.deepEqual(result, ['nrcmedia.nl']);
  assert.equal(resolveCNAME, 'beforeafter');
});
resolveCNAME += 'after';

// CNAME should not resolve
dns.resolve('nrcmedia.nl', 'CNAME', function(err, result) {
  assert.ok(err.errno, dns.NODATA);
});

function checkDnsRecord(host, record) {
  var myHost = host,
      myRecord = record;
  return function(err, stdout) {
    var expected = [];
    var footprints = 'before';
    if (stdout.length)
      expected = stdout.substr(0, stdout.length - 1).split('\n');

    switch (myRecord) {
      case 'A':
      case 'AAAA':
        dns.resolve(myHost, myRecord, function(error, result, ttl, cname) {
          if (error) result = [];
          cmpResults(expected, result, ttl, cname);

          // do reverse lookup check
          var ll = result.length;
          while (ll--) {
            var ip = result[ll];
            var reverseCmd = 'host ' + ip +
                             '| cut -d " " -f 5-' +
                             '| sed -e "s/\\.$//"';

            child_process.exec(reverseCmd, checkReverse(ip));
          }
          assert.equal(footprints, 'beforeafter');
        });
        break;
      case 'MX':
        dns.resolve(myHost, myRecord, function(error, result, ttl, cname) {
          if (error) result = [];

          var strResult = [];
          var ll = result.length;
          while (ll--) {
            strResult.push(result[ll].priority + ' ' + result[ll].exchange);
          }
          cmpResults(expected, strResult, ttl, cname);
          assert.equal(footprints, 'beforeafter');
        });
        break;
      case 'TXT':
        dns.resolve(myHost, myRecord, function(error, result, ttl, cname) {
          if (error) result = [];

          var strResult = [];
          var ll = result.length;
          while (ll--) {
            strResult.push('"' + result[ll] + '"');
          }
          cmpResults(expected, strResult, ttl, cname);
          assert.equal(footprints, 'beforeafter');
        });
        break;
      case 'SRV':
        dns.resolve(myHost, myRecord, function(error, result, ttl, cname) {
          if (error) result = [];

          var strResult = [];
          var ll = result.length;
          while (ll--) {
            strResult.push(result[ll].priority + ' ' +
                           result[ll].weight + ' ' +
                           result[ll].port + ' ' +
                           result[ll].name);
          }
          cmpResults(expected, strResult, ttl, cname);
          assert.equal(footprints, 'beforeafter');
        });
        break;
    }
    footprints += 'after';
  }
}

function checkReverse(ip) {
  var myIp = ip;

  return function(errr, stdout) {
    var expected = stdout.substr(0, stdout.length - 1).split('\n');

    var reversing = dns.reverse(myIp, function(error, domains, ttl, cname) {
      if (error) domains = [];
      cmpResults(expected, domains, ttl, cname);
    });
  }
}

function cmpResults(expected, result, ttl, cname) {
  if (expected.length != result.length) {
    if (expected.length == 1 &&
        expected[0] == '3(NXDOMAIN)' &&
        result.length == 0) {
      // it's ok, dig returns NXDOMAIN, while dns module returns nothing
    } else {
      console.log('---WARNING---\nexpected ' + expected +
                  '\nresult ' + result + '\n-------------');
    }
    return;
  }
  expected.sort();
  result.sort();

  var ll = expected.length;
  while (ll--) {
    assert.equal(result[ll], expected[ll]);
    console.log('Result ' + result[ll] +
                ' was equal to expected ' + expected[ll]);
  }
}

// #1164
var getHostByName = 'before';
dns.getHostByName('localhost', function() {
  assert.equal(getHostByName, 'beforeafter');
});
getHostByName += 'after';

var getHostByAddr = 'before';
dns.getHostByAddr('127.0.0.1', function() {
  assert.equal(getHostByAddr, 'beforeafter');
});
getHostByAddr += 'after';

var lookupEmpty = 'before';
dns.lookup('', function() {
  assert.equal(lookupEmpty, 'beforeafter');
});
lookupEmpty += 'after';

var lookupIp = 'before';
dns.lookup('127.0.0.1', function() {
  assert.equal(lookupIp, 'beforeafter');
});
lookupIp += 'after';

var lookupIp4 = 'before';
dns.lookup('127.0.0.1', 4, function() {
  assert.equal(lookupIp4, 'beforeafter');
});
lookupIp4 += 'after';

var lookupIp6 = 'before';
dns.lookup('ietf.org', 6, function() {
  assert.equal(lookupIp6, 'beforeafter');
});
lookupIp6 += 'after';

var lookupLocal = 'before';
dns.lookup('localhost', function() {
  assert.equal(lookupLocal, 'beforeafter');
});
lookupLocal += 'after';