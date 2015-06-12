const leveldown   = require('../')
    , timestamp   = require('monotonic-timestamp')
    , crypto      = require('crypto')
    , fs          = require('fs')
    , du          = require('du')

    , entryCount  = 10000000
    , concurrency = 10
    , timesFile   = './write_sorted_times.csv'
    , dbDir       = './write_sorted.db'
    , data        = crypto.randomBytes(256) // buffer

var db          = leveldown(dbDir)
  , timesStream = fs.createWriteStream(timesFile, 'utf8')

function report (ms) {
  console.log('Wrote', entryCount, 'in', Math.floor(ms / 1000) + 's')
  timesStream.end()
  du(dbDir, function (err, size) {
    if (err)
      throw err
    console.log('Database size:', Math.floor(size / 1024 / 1024) + 'M')
  })
  console.log('Wrote times to ', timesFile)
}

db.open({ errorIfExists: true, createIfMissing: true }, function (err) {
  if (err)
    throw err

  var inProgress  = 0
    , totalWrites = 0
    , startTime   = Date.now()
    , writeBuf    = ''

  function write() {
    if (totalWrites % 100000 == 0) console.log(inProgress, totalWrites)

    if (totalWrites % 1000 == 0) {
      timesStream.write(writeBuf)
      writeBuf = ''
    }

    if (totalWrites++ == entryCount)
      return report(Date.now() - startTime)

    if (inProgress >= concurrency || totalWrites > entryCount)
      return

    var time = process.hrtime()
    inProgress++

    db.put(timestamp(), data, function (err) {
      if (err)
        throw err
      writeBuf += (Date.now() - startTime) + ',' + process.hrtime(time)[1] + '\n'
      inProgress--
      process.nextTick(write)
    })

    process.nextTick(write)
  }

  write()
})
