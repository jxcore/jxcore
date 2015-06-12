const test       = require('tape')
    , testCommon = require('abstract-leveldown/testCommon')
    , leveldown  = require('../')

var db

test('setUp common', testCommon.setUp)

test('setUp db', function (t) {
  db = leveldown(testCommon.location())
  db.open(t.end.bind(t))
})

test('test argument-less getProperty() throws', function (t) {
  t.throws(
      db.getProperty.bind(db)
    , { name: 'Error', message: 'getProperty() requires a valid `property` argument' }
    , 'no-arg getProperty() throws'
  )
  t.end()
})

test('test non-string getProperty() throws', function (t) {
  t.throws(
      db.getProperty.bind(db, {})
    , { name: 'Error', message: 'getProperty() requires a valid `property` argument' }
    , 'no-arg getProperty() throws'
  )
  t.end()
})

test('test invalid getProperty() returns empty string', function (t) {
  t.equal(db.getProperty('foo'), '', 'invalid property')
  t.equal(db.getProperty('leveldb.foo'), '', 'invalid leveldb.* property')
  t.end()
})

test('test invalid getProperty("leveldb.num-files-at-levelN") returns numbers', function (t) {
  for (var i = 0; i < 7; i++)
    t.equal(db.getProperty('leveldb.num-files-at-level' + i), '0', '"leveldb.num-files-at-levelN" === "0"')
  t.end()
})

test('test invalid getProperty("leveldb.stats")', function (t) {
  t.ok(db.getProperty('leveldb.stats').split('\n').length > 3, 'leveldb.stats has > 3 newlines')
  t.end()
})

test('test invalid getProperty("leveldb.sstables")', function (t) {
  var expected = [0,1,2,3,4,5,6].map(function (l) { return '--- level ' + l + ' ---' }).join('\n') + '\n'
  t.equal(db.getProperty('leveldb.sstables'), expected, 'leveldb.sstables')
  t.end()
})

test('tearDown', function (t) {
  db.close(testCommon.tearDown.bind(null, t))
})
