const BUFFERS = false

var leveldown = require('..')
  , crypto    = require('crypto')
  , putCount  = 0
  , getCount  = 0
  , rssBase
  , db

function run () {
  var key = 'long key to test memory usage ' + String(Math.floor(Math.random() * 10000000))

  if (BUFFERS) key = new Buffer(key)

  db.get(key, function (err, value) {
    getCount++

    if (err) {
      var putValue = crypto.randomBytes(1024)
      if (!BUFFERS) putValue = putValue.toString('hex')

      return db.put(key, putValue, function () {
        putCount++
        process.nextTick(run)
      })
    }

    process.nextTick(run)
  })

  if (getCount % 1000 === 0) {
    if (typeof gc != 'undefined')
      gc()
    console.log(
        'getCount ='
      , getCount
      , ', putCount = '
      , putCount
      , ', rss ='
      , Math.round(process.memoryUsage().rss / rssBase * 100) + '%'
      , Math.round(process.memoryUsage().rss / 1024 / 1024) + 'M'
      , JSON.stringify([0,1,2,3,4,5,6].map(function (l) {
          return db.getProperty('leveldb.num-files-at-level' + l)
        }))
    )
  }
}

leveldown.destroy('./leakydb', function () {
  db = leveldown('./leakydb')
  db.open({ xcacheSize: 0, xmaxOpenFiles: 10 }, function () {
    rssBase = process.memoryUsage().rss
    run()
  })
})