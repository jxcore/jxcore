const test       = require('tape')
    , testCommon = require('abstract-leveldown/testCommon')
    , leveldown  = require('../')
    , makeTest   = require('./make')

makeTest('test ended iterator', function (db, t, done) {
  // standard iterator with an end() properly called, easy

  var it = db.iterator({ keyAsBuffer: false, valueAsBuffer: false })
  it.next(function (err, key, value) {
    t.notOk(err, 'no error from next()')
    t.equal(key, 'one', 'correct key')
    t.equal(value, '1', 'correct value')
    it.end(function (err) {
      t.notOk(err, 'no error from next()')
      done()
    })
  })
})

makeTest('test non-ended iterator', function (db, t, done) {
  // no end() call on our iterator, cleanup should crash Node if not handled properly
  var it = db.iterator({ keyAsBuffer: false, valueAsBuffer: false })
  it.next(function (err, key, value) {
    t.notOk(err, 'no error from next()')
    t.equal(key, 'one', 'correct key')
    t.equal(value, '1', 'correct value')
    done()
  })
})

makeTest('test multiple non-ended iterators', function (db, t, done) {
  // no end() call on our iterator, cleanup should crash Node if not handled properly
  db.iterator()
  db.iterator().next(function () {})
  db.iterator().next(function () {})
  db.iterator().next(function () {})
  setTimeout(done, 50)
})

makeTest('test ending iterators', function (db, t, done) {
  // at least one end() should be in progress when we try to close the db
  var it1 = db.iterator().next(function () {
        it1.end(function () {})
      })
    , it2 = db.iterator().next(function () {
        it2.end(function () {})
        done()
      })
})
