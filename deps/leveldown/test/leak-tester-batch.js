const BUFFERS = false
    , CHAINED = false

var leveldown  = require('..')
  , crypto     = require('crypto')
  , assert     = require('assert')
  , writeCount = 0
  , rssBase
  , db

function print () {
  if (writeCount % 100 === 0) {
    if (typeof gc != 'undefined')
      gc()

    console.log(
        'writeCount ='
      , writeCount
      , ', rss ='
      , Math.round(process.memoryUsage().rss / rssBase * 100) + '%'
      , Math.round(process.memoryUsage().rss / 1024 / 1024) + 'M'
      , JSON.stringify([0,1,2,3,4,5,6].map(function (l) {
          return db.getProperty('leveldb.num-files-at-level' + l)
        }))
    )
  }
}

var run = CHAINED
  ? function () {
      var batch = db.batch()
        , i     = 0
        , key
        , value

      for (i = 0; i < 100; i++) {
        key = 'long key to test memory usage ' + String(Math.floor(Math.random() * 10000000))
        if (BUFFERS)
          key = new Buffer(key)
        value = crypto.randomBytes(1024)
        if (!BUFFERS)
          value = value.toString('hex')
        batch.put(key, value)
      }

      batch.write(function (err) {
        assert(!err)
        process.nextTick(run)
      })

      writeCount++

      print()
    }
  : function () {
      var batch = []
        , i
        , key
        , value

      for (i = 0; i < 100; i++) {
        key = 'long key to test memory usage ' + String(Math.floor(Math.random() * 10000000))
        if (BUFFERS)
          key = new Buffer(key)
        value = crypto.randomBytes(1024)
        if (!BUFFERS)
          value = value.toString('hex')
        batch.push({ type: 'put', key: key, value: value })
      }

      db.batch(batch, function (err) {
        assert(!err)
        process.nextTick(run)
      })

      writeCount++

      print()
    }

leveldown.destroy('./leakydb', function () {
  db = leveldown('./leakydb')
  db.open({ xcacheSize: 0, xmaxOpenFiles: 10 }, function () {
    rssBase = process.memoryUsage().rss
    run()
  })
})