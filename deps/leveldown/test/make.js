const test       = require('tape')
    , testCommon = require('abstract-leveldown/testCommon')
    , cleanup    = testCommon.cleanup
    , location   = testCommon.location
    , leveldown  = require('../')

function makeTest (name, testFn) {
  test(name, function (t) {
    cleanup(function () {
      var loc  = location()
        , db   = leveldown(loc)
        , done = function (close) {
            if (close === false)
              return cleanup(t.end.bind(t))
            db.close(function (err) {
              t.notOk(err, 'no error from close()')
              cleanup(t.end.bind(t))
            })
          }
      db.open(function (err) {
       t.notOk(err, 'no error from open()')
        db.batch(
            [
                { type: 'put', key: 'one', value: '1' }
              , { type: 'put', key: 'two', value: '2' }
              , { type: 'put', key: 'three', value: '3' }
            ]
          , function (err) {
              t.notOk(err, 'no error from batch()')
              testFn(db, t, done, loc)
            }
        )
      })
    })
  })
}

module.exports = makeTest
