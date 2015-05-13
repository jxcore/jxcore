# Debugging

Writing asynchronous functions using the threadpool unfortunately also removes all stack trace information, making debugging very hard since you only see the error message, not which statement caused it. To mitigate this problem, `node-sqlite3` has a **verbose mode** which captures stack traces when enqueuing queries. To enable this mode, call the `sqlite3.verbose()`, or call it directly when requiring: `var sqlite3 = require('sqlite3').verbose()`.

When you throw an error from a callback passed to any of the database functions, `node-sqlite3` will append the stack trace information from the original call, like this:

    Error: SQLITE_RANGE: bind or column index out of range
    --> in Database#run('CREATE TABLE foo (a, b)', 3, [Function])
        at Object.<anonymous> (demo.js:5:4)
        at Module._compile (module.js:374:26)
        at Object..js (module.js:380:10)
        at Module.load (module.js:306:31)
        at Function._load (module.js:272:10)
        at Array.<anonymous> (module.js:393:10)
        at EventEmitter._tickCallback (node.js:108:26)

Note that you shouldn't enable the verbose mode in a production setting as the performance penalty for collecting stack traces is quite high.

Verbose mode currently does not add stack trace information to error objects emitted on Statement or Database objects.

## Database#on('trace', [callback])

The `trace` event is emitted whenever a query is run. The first and only parameter to the callback is the SQL string that was sent to the database. The event is emitted as soon as the query is *executed* (e.g. with `.run()` or `.get()`). A single statement may be emitted more once. `EXPLAIN` statements will not trigger an event, so it's safe to pipe all SQL queries you receive from this event back into the database prefixed with a `EXPLAIN QUERY PLAN`.

If you execute statements from this callback, make sure that you don't enter an infinite loop!

## Database#on('profile', [callback])

The `profile` event is emitted whenever a query is finished. The first parameter is the SQL string that was sent to the database, the second parameter is the time approximate time it took to run in milliseconds. The event is emitted after the query completed.

If you execute statements from this callback, make sure that you don't enter an infinite loop!
