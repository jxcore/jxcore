// Copyright & License details are available under JXCORE_LICENSE file



var jx = require('jxtools');
var assert = jx.assert;
var crypto = require('crypto');
var alice = crypto.getDiffieHellman('modp5');
var bob = crypto.getDiffieHellman('modp5');

alice.generateKeys();
bob.generateKeys();

var alice_secret = alice.computeSecret(bob.getPublicKey(), null, 'hex');
var bob_secret = bob.computeSecret(alice.getPublicKey(), null, 'hex');

/* alice_secret and bob_secret should be the same */
assert.strictEqual(alice_secret, bob_secret, "Values should be equal, but the aren't:\n" + alice_secret + "\n" + bob_secret);


if (process.threadId !== -1)
  process.release();

jx.exitNowMT();