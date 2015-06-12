/**
  * This test uses infinite recursion to test iterator creation with limited
  * stack space. In order to isolate the test harness, we run in a different
  * process. This is achieved through a fork() command in
  * iterator-recursion-test.js. To prevent tap from trying to run this test
  * directly, we check for a command-line argument.
  */
const testCommon = require('abstract-leveldown/testCommon')
    , leveldown  = require('../')

if (process.argv[2] == 'run') {
  testCommon.cleanup(function () {
    var db    = leveldown(testCommon.location())
      , depth = 0

    db.open(function () {
      function recurse() {
        db.iterator({ start: '0' })
        depth++
        recurse()
      }

      try {
        recurse()
      } catch (e) {
        process.send("Catchable error at depth " + depth)
      }
    })
  })
}
