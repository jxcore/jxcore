# Control Flow

`node-sqlite3` provides two functions to help controlling the execution flow of statements. The default mode is to execute statements in parallel. However, the `Database#close` method will always run in exclusive mode, meaning it waits until all previous queries have completed and `node-sqlite3` will not run any other queries while a close is pending.

## Database#serialize([callback])

Puts the execution mode into serialized. This means that at most one statement object can execute a query at a time. Other statements wait in a queue until the previous statements executed.

If a callback is provided, it will be called immediately. All database queries scheduled in that callback will be serialized. After the function returns, the database is set back to its original mode again. Calling `Database#serialize()` with in nested functions is safe:

```js
    // Queries scheduled here will run in parallel.

    db.serialize(function() {
      // Queries scheduled here will be serialized.
      db.serialize(function() {
        // Queries scheduled here will still be serialized.
      });
      // Queries scheduled here will still be serialized.
    });

    // Queries scheduled here will run in parallel again.
```

Note that queries scheduled not *directly* in the callback function are not necessarily serialized:

```js
    db.serialize(function() {
      // These two queries will run sequentially.
      db.run("CREATE TABLE foo (num)");
      db.run("INSERT INTO foo VALUES (?)", 1, function() {
        // These queries will run in parallel and the second query will probably
        // fail because the table might not exist yet.
        db.run("CREATE TABLE bar (num)");
        db.run("INSERT INTO bar VALUES (?)", 1);
      });
    });
```

If you call it without a function parameter, the execution mode setting is sticky and won't change until the next call to `Database#parallelize`.

## Database#parallelize([callback])

Puts the execution mode into parallelized. This means that queries scheduled will be run in parallel.

If a callback is provided, it will be called immediately. All database queries scheduled in that callback will run parallelized. After the function returns, the database is set back to its original mode again. Calling `Database#parallelize()` with in nested functions is safe:

```js
    db.serialize(function() {
       // Queries scheduled here will be serialized.
       db.parallelize(function() {
         // Queries scheduled here will run in parallel.
       });
       // Queries scheduled here will be serialized again.
    });
```

If you call it without a function parameter, the execution mode setting is sticky and won't change until the next call to `Database#serialize`.
