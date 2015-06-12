var path = require('path');
var binding = require('_jx_loadEmbedded').require('sqlite3');

var sqlite3 = module.exports = exports = binding;
var util = require('util');
var EventEmitter = require('events').EventEmitter;

function errorCallback(args) {
    if (typeof args[args.length - 1] === 'function') {
        var callback = args[args.length - 1];
        return function(err) { if (err) callback(err); };
    }
}

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

sqlite3.cached = {
    Database: function(file, a, b) {
        if (file === '' || file === ':memory:') {
            // Don't cache special databases.
            return new Database(file, a, b);
        }

        if (file[0] !== '/') {
            file = path.join(process.cwd(), file);
        }

        if (!sqlite3.cached.objects[file]) {
            var db =sqlite3.cached.objects[file] = new Database(file, a, b);
        }
        else {
            // Make sure the callback is called.
            var db = sqlite3.cached.objects[file];
            var callback = (typeof a === 'number') ? b : a;
            if (typeof callback === 'function') {
                function cb() { callback.call(db, null); }
                if (db.open) process.nextTick(cb);
                else db.once('open', cb);
            }
        }

        return db;
    },
    objects: {}
};


var Database = sqlite3.Database;
var Statement = sqlite3.Statement;

inherits(Database, EventEmitter);
inherits(Statement, EventEmitter);

// Database#prepare(sql, [bind1, bind2, ...], [callback])
Database.prototype.prepare = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);

    if (!params.length || (params.length === 1 && typeof params[0] === 'function')) {
        return new Statement(this, sql, params[0]);
    }
    else {
        var statement = new Statement(this, sql, errorCallback(params));
        return statement.bind.apply(statement, params);
    }
};

// Database#run(sql, [bind1, bind2, ...], [callback])
Database.prototype.run = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.run.apply(statement, params).finalize();
    return this;
};

// Database#get(sql, [bind1, bind2, ...], [callback])
Database.prototype.get = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.get.apply(statement, params).finalize();
    return this;
};

// Database#all(sql, [bind1, bind2, ...], [callback])
Database.prototype.all = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.all.apply(statement, params).finalize();
    return this;
};

// Database#each(sql, [bind1, bind2, ...], [callback], [complete])
Database.prototype.each = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.each.apply(statement, params).finalize();
    return this;
};

Database.prototype.map = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.map.apply(statement, params).finalize();
    return this;
};

Statement.prototype.map = function() {
    var params = Array.prototype.slice.call(arguments);
    var callback = params.pop();
    params.push(function(err, rows) {
        if (err) return callback(err);
        var result = {};
        if (rows.length) {
            var keys = Object.keys(rows[0]), key = keys[0];
            if (keys.length > 2) {
                // Value is an object
                for (var i = 0; i < rows.length; i++) {
                    result[rows[i][key]] = rows[i];
                }
            } else {
                var value = keys[1];
                // Value is a plain value
                for (var i = 0; i < rows.length; i++) {
                    result[rows[i][key]] = rows[i][value];
                }
            }
        }
        callback(err, result);
    });
    return this.all.apply(this, params);
};

var isVerbose = false;

var supportedEvents = [ 'trace', 'profile', 'insert', 'update', 'delete' ];

Database.prototype.addListener = Database.prototype.on = function(type) {
    var val = EventEmitter.prototype.addListener.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0) {
        this.configure(type, true);
    }
    return val;
};

Database.prototype.removeListener = function(type) {
    var val = EventEmitter.prototype.removeListener.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0 && !this._events[type]) {
        this.configure(type, false);
    }
    return val;
};

Database.prototype.removeAllListeners = function(type) {
    var val = EventEmitter.prototype.removeAllListeners.apply(this, arguments);
    if (supportedEvents.indexOf(type) >= 0) {
        this.configure(type, false);
    }
    return val;
};

var EventEmitter = require('events').EventEmitter;
var util = require('util');

function extendTrace(object, property, pos) {
    var old = object[property];
    object[property] = function() {
        var error = new Error();
        var name = object.constructor.name + '#' + property + '(' + 
            Array.prototype.slice.call(arguments).map(function(el) {
                return util.inspect(el, false, 0);
            }).join(', ') + ')';

        if (typeof pos === 'undefined') pos = -1;
        if (pos < 0) pos += arguments.length;
        var cb = arguments[pos];
        if (typeof arguments[pos] === 'function') {
            arguments[pos] = function replacement() {
                try {
                    return cb.apply(this, arguments);
                } catch (err) {
                    if (err && err.stack && !err.__augmented) {
                        err.stack = filter(err).join('\n');
                        err.stack += '\n--> in ' + name;
                        err.stack += '\n' + filter(error).slice(1).join('\n');
                        err.__augmented = true;
                    }
                    throw err;
                }
            };
        }
        return old.apply(this, arguments);
    };
}


function filter(error) {
    return error.stack.split('\n').filter(function(line) {
        return line.indexOf(__filename) < 0;
    });
}

// Save the stack trace over EIO callbacks.
sqlite3.verbose = function() {
    if (!isVerbose) {
        extendTrace(Database.prototype, 'prepare');
        extendTrace(Database.prototype, 'get');
        extendTrace(Database.prototype, 'run');
        extendTrace(Database.prototype, 'all');
        extendTrace(Database.prototype, 'each');
        extendTrace(Database.prototype, 'map');
        extendTrace(Database.prototype, 'exec');
        extendTrace(Database.prototype, 'close');
        extendTrace(Statement.prototype, 'bind');
        extendTrace(Statement.prototype, 'get');
        extendTrace(Statement.prototype, 'run');
        extendTrace(Statement.prototype, 'all');
        extendTrace(Statement.prototype, 'each');
        extendTrace(Statement.prototype, 'map');
        extendTrace(Statement.prototype, 'reset');
        extendTrace(Statement.prototype, 'finalize');
        isVerbose = true;
    }

    return this;
};
