// Copyright & License details are available under JXCORE_LICENSE file


var common = require('../common');
var assert = require('assert');
var StringDecoder = require('string_decoder').StringDecoder;
var decoder = new StringDecoder('utf8');



var buffer = new Buffer('$');
assert.deepEqual('$', decoder.write(buffer));

buffer = new Buffer('¢');
assert.deepEqual('', decoder.write(buffer.slice(0, 1)));
assert.deepEqual('¢', decoder.write(buffer.slice(1, 2)));

buffer = new Buffer('€');
assert.deepEqual('', decoder.write(buffer.slice(0, 1)));
assert.deepEqual('', decoder.write(buffer.slice(1, 2)));
assert.deepEqual('€', decoder.write(buffer.slice(2, 3)));

buffer = new Buffer([0xF0, 0xA4, 0xAD, 0xA2]);
var s = '';
s += decoder.write(buffer.slice(0, 1));
s += decoder.write(buffer.slice(1, 2));
s += decoder.write(buffer.slice(2, 3));
s += decoder.write(buffer.slice(3, 4));
assert.ok(s.length > 0);

if(process.versions.v8){ // MozJS doesn't support CESU-8
// CESU-8
	buffer = new Buffer('EDA0BDEDB18D', 'hex'); // THUMBS UP SIGN (in CESU-8)
	var s = '';
	s += decoder.write(buffer.slice(0, 1));
	s += decoder.write(buffer.slice(1, 2));
	s += decoder.write(buffer.slice(2, 3)); // complete lead surrogate

	assert.equal(s, '');
	s += decoder.write(buffer.slice(3, 4));
	s += decoder.write(buffer.slice(4, 5));
	s += decoder.write(buffer.slice(5, 6)); // complete trail surrogate
	assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

	var s = '';
	s += decoder.write(buffer.slice(0, 2));
	s += decoder.write(buffer.slice(2, 4)); // complete lead surrogate
	assert.equal(s, '');
	s += decoder.write(buffer.slice(4, 6)); // complete trail surrogate
	assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

	var s = '';
	s += decoder.write(buffer.slice(0, 3)); // complete lead surrogate
	assert.equal(s, '');
	s += decoder.write(buffer.slice(3, 6)); // complete trail surrogate
	assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

	var s = '';
	s += decoder.write(buffer.slice(0, 4)); // complete lead surrogate
	assert.equal(s, '');
	s += decoder.write(buffer.slice(4, 5));
	s += decoder.write(buffer.slice(5, 6)); // complete trail surrogate
	assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

	var s = '';
	s += decoder.write(buffer.slice(0, 5)); // complete lead surrogate
	assert.equal(s, '');
	s += decoder.write(buffer.slice(5, 6)); // complete trail surrogate
	assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

	var s = '';
	s += decoder.write(buffer.slice(0, 6));
	assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)
}

// UCS-2
decoder = new StringDecoder('ucs2');
buffer = new Buffer('ab', 'ucs2');
assert.equal(decoder.write(buffer), 'ab'); // 2 complete chars
buffer = new Buffer('abc', 'ucs2');
assert.equal(decoder.write(buffer.slice(0, 3)), 'a'); // 'a' and first of 'b'
assert.equal(decoder.write(buffer.slice(3, 6)), 'bc'); // second of 'b' and 'c'


// UTF-16LE
buffer = new Buffer('3DD84DDC', 'hex'); // THUMBS UP SIGN (in CESU-8)
var s = '';
s += decoder.write(buffer.slice(0, 1));
s += decoder.write(buffer.slice(1, 2)); // complete lead surrogate
assert.equal(s, '');
s += decoder.write(buffer.slice(2, 3));
s += decoder.write(buffer.slice(3, 4)); // complete trail surrogate
assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

var s = '';
s += decoder.write(buffer.slice(0, 2)); // complete lead surrogate
assert.equal(s, '');
s += decoder.write(buffer.slice(2, 4)); // complete trail surrogate
assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

var s = '';
s += decoder.write(buffer.slice(0, 3)); // complete lead surrogate
assert.equal(s, '');
s += decoder.write(buffer.slice(3, 4)); // complete trail surrogate
assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)

var s = '';
s += decoder.write(buffer.slice(0, 4));
assert.equal(s, '\uD83D\uDC4D'); // THUMBS UP SIGN (in UTF-16)


// A mixed ascii and non-ascii string
// Test stolen from deps/v8/test/cctest/test-strings.cc
// U+02E4 -> CB A4
// U+0064 -> 64
// U+12E4 -> E1 8B A4
// U+0030 -> 30
// U+3045 -> E3 81 85
var expected = '\u02e4\u0064\u12e4\u0030\u3045';
var buffer = new Buffer([0xCB, 0xA4, 0x64, 0xE1, 0x8B, 0xA4,
                         0x30, 0xE3, 0x81, 0x85]);
var charLengths = [0, 0, 1, 2, 2, 2, 3, 4, 4, 4, 5, 5];

// Split the buffer into 3 segments
//  |----|------|-------|
//  0    i      j       buffer.length
// Scan through every possible 3 segment combination
// and make sure that the string is always parsed.
common.print('scanning ');
for (var j = 2; j < buffer.length; j++) {
  for (var i = 1; i < j; i++) {
    var decoder = new StringDecoder('utf8');

    var sum = decoder.write(buffer.slice(0, i));

    // just check that we've received the right amount
    // after the first write
    assert.equal(charLengths[i], sum.length);

    sum += decoder.write(buffer.slice(i, j));
    sum += decoder.write(buffer.slice(j, buffer.length));
    assert.equal(expected, sum);
    common.print('.');
  }
}
console.log(' crayon!');