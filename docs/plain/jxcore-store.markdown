# Memory Store

JXcore provides you with a fast and efficient in-memory storing capabilities. We call it Memory Store.

It’s implementation is based on Google’s cpp-btree library, hence benefits from it’s advantages.

The major performance benefit from Memory Store is very high access speed (even for large containers) and low memory foot-print.
Indeed, the best news is, that Memory Store is kept outside V8′s heap memory space.
As a result it doesn't fall under it's limitations (similar to Multithreaded isolates).
Keeping V8 away from high memory pressure also results in more responsive application since the unexpected garbage collection may keep the application blocked several seconds.

The Memory Store works like a standard key/value store. It is available for both main and sub threads and also as a separate or shared object.

The `key` for the element has to be unique in the scope of entire store. Basically key should be a string type, but you can also use number and it will be converted to string internally.
The `element` also should be a string type, but you can use a number and it will be converted to string internally.

The Memory store is a global object and can be used anywhere:

```js
var store = jxcore.store;
```

or for multi-threaded access:

```js
var shared = jxcore.store.shared;
```

# API

## jxcore.store

When used in multi-threading, every subthread has it’s own `jxcore.store` object.
It can be considered as a static global per store context, which means, that threads cannot share the same `jxcore.store` among themselves.
But all of the tasks running inside a particular thread have shared access to it.

### store.exists(key, element)

* `key` {String}
* `element` {String}

Sets the element's value for specific key in the store.
If the key already exists, the current element's value will overwrite the previous one.

```js
var store = jxcore.store;
if (!store.exists("111")) {
    store.set("111", "test");
}
```

### store.get(key)

* `key` {String}

Removes the key/value pair from the store and returns the element’s value as a string.
If the key is found, the method returns its value, otherwise returns `undefined`.

```js
jxcore.store.set("key1", "value1");

// below line outputs: "value for key1: value1"
console.log("value for key1:", jxcore.store.get("key1"));

// another call of get() outputs: "value for key1: undefined"
// because the key/value pair was removed at first call
console.log("value for key1:", jxcore.store.get("key1"));
```

### store.read(key)

* `key` {String}

Reads element for specific key and returns its value as a string. This method doesn't remove the element from the store.
If the key is found, the method returns its value, otherwise returns `undefined`.

```js
var store = jxcore.store;
store.set("111", "test");

// the calls below are equivalent:
var x = store.read("111");
var y = store.read(111);

// now x is equal to y
```

### store.remove(key)

* `key` {String}

Removes the key/value pair from the store.

```js
var store = jxcore.store;
store.set("111", "test");
store.remove(111);
```

### store.set(key, element)

* `key` {String}
* `element` {String}

Sets the element’s value for specific key in the store.
If the key already exists, the current element’s value will overwrite the previous one.

```js
var store = jxcore.store;
store.set("string", "test");

// below usages will make automatic conversion of provided keys and values
// into strings:
store.set(1, "one");       // equivalent to store.set("1", "one");
store.set(2, "two");       // equivalent to store.set("2", "two");
store.set(true, true);     // equivalent to store.set("true", "true");
store.set(1.45, 2.77);     // equivalent to store.set("1.45", "2.77");
```

## jxcore.store.shared

In some scenarios you may need a single store, which you could access from any thread. Use `jxcore.store.shared` object for this purpose.
You should be aware of the following principles, even if all operations on this store are thread-safe.
First of all, access time may take little longer comparing to the single threaded `jxcore.store` especially when using operations,
which modify content of the data store (`set()`, `remove()` and `get()` – the last one also removes the item).
Also, when different threads are simultaneously writing/modifying value of the same key, the final stored value is the one that is updated last.

Thread-safe `jxcore.store.shared` has exactly the same methods as single threaded `jxcore.store`, but also implements some other members, specific for multi-threading.

### store.shared.exists(key, element)

See `store.exists(key, element)`.

### store.shared.expires(key, timeout)

* `key` {String}
* `timeout` {Number}

Sets an expiration `timeout` (milliseconds) for specified `key`.
If there is no `read()`, `get()` or `set()` method called on this `key `within the `timeout` period (after `expires()` has been called),
the `key` and it's value are automatically removed from the shared.store.
Otherwise, whenever one of those methods are invoked for this `key` before the `timeout` elapses,
JXcore invokes again the `expires()` method with the same parameters, so at that moment the `timeout` counter starts from zero.

Precision of `key` expiration is +/- 10 milliseconds, which means, that if you set the `timeout` to e.g. 510 ms,
the expiration may occur in a range between 500 and 520 ms.

```js
var mem = jxcore.store.shared;

mem.set("key", "Hello");
mem.expires("key", 350);

setTimeout(function(){
    // the key still exists in the shared store
    // mem.read("key") will will rewind the timeout counter to the start
    // so it will expire after another 350 ms
    console.log("data", mem.read("key"));
},300);

setTimeout(function(){
    // right now the timeout is expired
    // and the key is already removed from the store.
    // the return value of mem.read("key") will be undefined
    console.log("data", mem.read("key"));
},900);
```

### store.shared.get(key)

See `store.get(key)`.

### store.shared.getBlockTimeout()

Gets the maximum time (milliseconds) during which the key in a `safeBlock()` is locked.
By default its value is 10000 milliseconds and can be changed with `setBlockTimeout()`.

### store.shared.read(key)

See `store.read(key)`.

### store.shared.remove(key)

See `store.remove(key)`.

### store.shared.set(key, element)

See `store.set(key, element)`.

### store.shared.safeBlock(key, safeBlockFunction, errorCallback)

* `key` {String}
* `safeBlockFunction` {Function}
* `errorCallback` {Function}

Allows to execute number of store related operations on a single `key` in one continuous safe block.
During it's execution, access to this element (the key and it's value) is locked for other threads.
The default maximum blocking time is defined by `setBlockTimeout()` parameter.

The `safeBlockFunction` is the execution block. This should be a function without any arguments.

```js
jxcore.store.shared.safeBlock("myNumber", function () {

        if (!shared.exists("myNumber")) {
            shared.set("myNumber", 0);
        }

        var n = shared.read("myNumber");
        n = parseInt(n) + 1;
        shared.set("myNumber", n);

        // working with a different key should not be performed in this block:
        shared.set("myNumber_2", 333);
});
```

In the example above we are using a `safeBlock` to perform multithread-safe increment of *myNumber* value kept in the shared store.

Functionality of the `safeBlockFunction` should be limited only to operations on a single key kept in the store, because locking mechanism applies only for that `key` and there is simply no safety guarantee for any other keys.

Every `safeBlock()` invocation locks the key and it's value for a specific amount of time defined by `setBlockTimeout()`.

The `errorCallback` will be invoked whenever an exception occurs inside the `safeBlockFunction`. Exception is caught in a safe manner, so the key and its value will be unlocked for other threads access.

```js
jxcore.store.shared.safeBlock("myNumber",
    function () {
        throw "some error";
    },
    function(err) {
        console.log(err);
    }
);
```

### store.shared.setBlockTimeout()

Sets the maximum time (milliseconds) during which the key in a `safeBlock` is locked.
By default its value is 10000 milliseconds.
When it elapses, JXcore automatically unlocks the key for other threads access.

### store.shared.setIfEqualsTo(key, newValue, checkValue)

* `key` {String}
* `newValue` {String}
* `checkValue` {String}

Sets the element's value for specific key in the store, but only if its current value equals to `check_value`.

Returns `true` if `newValue` was set (current value was not equal to `checkValue`). Otherwise returns `false`.

### store.shared.setIfEqualsToOrNull(key, newValue, checkValue)

* `key` {String}
* `newValue` {String}
* `checkValue` {String}

Sets the element's value for specific key in the store, but only if its current value equals to `check_value` or if the key does not exist in the store.

Returns `true` if `newValue` was set (current value was not equal to `checkValue` or the `key` did not exists). Otherwise returns `false`.

### store.shared.setIfNotExists(key, element)

* `key` {String}
* `newValue` {String}
* `checkValue` {String}

Sets the element's value for specific key in the store, but only if the key does not exist yet.

Returns `true` if `newValue` was set (the `key` did not exists). Otherwise returns `false`.
