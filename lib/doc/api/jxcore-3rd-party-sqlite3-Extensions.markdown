# Extensions


## Database#loadExtension(path, [callback])

Loads a compiled SQLite extension into the database connection object.

* `path`: Filename of the extension to load.

* `callback` *(optional)*: If provided, this function will be called when the extension was loaded successfully or when an error occurred. The first argument is an error object. When it is `null`, loading succeeded. If no callback is provided and an error occurred, an `error` event with the error object as the only parameter will be emitted on the database object.

**Note: Make sure that the extensions you load are compiled or linked against the same version as `node-sqlite3` was compiled.** Version mismatches can lead to unpredictable behavior.

## Building an Extension

**half**: The SQLite documentation gives an example of a user-defined function "half" which takes a number and returns a result (the number diveded by two): http://www.sqlite.org/cvstrac/wiki?p=LoadableExtensions

**rank**: A ready-to `make` example from the full-text search docs https://github.com/coolaj86/sqlite3-fts4-rank . See also: http://www.mail-archive.com/sqlite-users@sqlite.org/msg71740.html