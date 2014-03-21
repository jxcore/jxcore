# Caching

`node-sqlite3` has a built-in database object cache to avoid opening the same database multiple times. To use the caching feature, simply use `new sqlite3.cached.Database()` instead of `new sqlite3.Database()`.
