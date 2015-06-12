const test         = require('tape')
    , fs           = require('fs')
    , path         = require('path')
    , mkfiletree   = require('mkfiletree')
    , readfiletree = require('readfiletree')
    , testCommon   = require('abstract-leveldown/testCommon')
    , leveldown    = require('../')
    , makeTest     = require('./make')

test('test argument-less destroy() throws', function (t) {
  t.throws(
      leveldown.destroy
    , { name: 'Error', message: 'destroy() requires `location` and `callback` arguments' }
    , 'no-arg destroy() throws'
  )
  t.end()
})

test('test callback-less, 1-arg, destroy() throws', function (t) {
  t.throws(
      leveldown.destroy.bind(null, 'foo')
    , { name: 'Error', message: 'destroy() requires `location` and `callback` arguments' }
    , 'callback-less, 1-arg destroy() throws'
  )
  t.end()
})

test('test destroy non-existant directory', function (t) {
  leveldown.destroy('/1/2/3/4', function () {
    t.equal(arguments.length, 0, 'no arguments returned on callback')
    t.end()
  })
})

test('test destroy non leveldb directory', function (t) {
  var tree = {
      'foo': 'FOO'
    , 'bar': { 'one': 'ONE', 'two': 'TWO', 'three': 'THREE' }
  }
  mkfiletree.makeTemp('destroy-test', tree, function (err, dir) {
    t.notOk(err, 'no error')
    leveldown.destroy(dir, function () {
      t.ok(arguments, 'no arguments')
      readfiletree(dir, function (err, actual) {
        t.notOk(err, 'no error')
        t.deepEqual(actual, tree, 'directory remains untouched')
        mkfiletree.cleanUp(function (err) {
          t.notOk(err, 'no error')
          t.end()
        })
      })
    })
  })
})

makeTest('test destroy() cleans and removes leveldb-only dir', function (db, t, done, location) {
  db.close(function (err) {
    t.notOk(err, 'no error')
    leveldown.destroy(location, function () {
      t.notOk(fs.existsSync(), 'directory completely removed')
      done(false)
    })
  })
})

makeTest('test destroy() cleans and removes only leveldb parts of a dir', function (db, t, done, location) {
  fs.writeFileSync(path.join(location, 'foo'), 'FOO')
  db.close(function (err) {
    t.notOk(err, 'no error')
    leveldown.destroy(location, function () {
      readfiletree(location, function (err, tree) {
        t.deepEqual(tree, { 'foo': 'FOO' }, 'non-leveldb files left intact')
        done(false)
      })
    })
  })
})
