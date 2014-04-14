# Mesaging

Using JXcore Messaging, your clients can easily communicate with the server (backend service) as well as with other clients.

Simply create a custom method. It can be invoked either by any of the clients or by the server itself.

If you want to create an online game, chat application, or any other project for multiple users – JXcore Messaging is for you.

There are some tutorials for using JXcore Messaging available here: [Hello World](http://jxcore.com/messaging-api/#cat-32).

# API Server

## Configuration

All of the server's settings listed below may be changed using [`setConfig()`](#setconfig-key-value) method.

### chunked

* {Boolean} default `true`

Enables messaging server to send multiple messages at once to the client. This increases performance of JXcore Messaging.
There are some browser versions, however, which don't support this feature (for example IE below v8).
In this case, chunked mode is internally disabled, even if server's `chunked` option is set to `true`.

### collectorLatency

* {Number} default 50

Defines interval in milliseconds for pushing messages to the clients.
Each message, that client has sent to the server or other clients is not processed immediately.
Instead it is queued and processed together with other messages collected within `collectorLatency` period.
This way server stays more responsive, because doesn't have to deal with each message separately.

### console

* {Boolean} default `true`

When enabled, displays log and error messages to the console output.

### consoleInfo

* {Boolean} default `false`

When enabled, displays additional (informative) log messages to the console output.

### consoleThreadNumber

* {Boolean} default `true`

When JXcore Messaging server runs in multithreaded mode and this option is enabled, each of log and error messages displayed to the console
contains information about thread ID, from which the message comes.

Example output:

```
Thread#1 JXcore Messaging v0.22
Thread#0 JXcore Messaging v0.22
Thread#1 HTTP  -> http://192.168.1.11:8000/test
Thread#0 HTTP  -> http://192.168.1.11:8000/test
```

### enableClientSideSubscription

* {Boolean} default `false`

When this option is set to `false` (and it is by default), the client's methods `Subscribe()` and `Unsubscribe()` are disabled.
They still exist in client's API, but invoking them will have no effect, since the calls will be ignored on the server-side.
Client's are still able to send messages to the groups, but since they cannot subscribe o them, it should be done by the server.
See server-side methods [`subscribeClient()`](#subscribeclient-env-groupname) and [`unSubscribeClient()`](#unsubscribeclient-env-groupname).

### encoding

* {String} default "UTF-8"

Defines encoding type of messages being sent both ways between server and client's.

### httpsCertLocation
### httpsKeyLocation

* {String} default null

Those both options define locations for SSL certificate files. See also: `httpsServerPort` option.

### httpServerPor

* {Number} default 8000

Defines port for HTTP server of JXcore Messaging Server.

### httpsServerPort

* {Number} default 0

Defines port for HTTPS (SSL) server of JXcore Messaging.
The default value 0 also disables SSL support and it means, that JXcore Messaging backend will run based on regular HTTP protocol.
When `httpsServerPort` is set to a number, both `httpsCertLocation` and `httpsKeyLocation` should be provided and be a valid file paths,
otherwise SSL support will not be enabled.

### IPAddress

* {String} default "localhost";

Defines the IP address, on which JXcore Messaging backend's HTTP or HTTPS server will be running.
By default it's "localhost", but you may also use any valid IP address, like "192.168.1.11" or any other.

Client's will use this value to connect to the server, so the IP should be always accessible for them.
For example, you should avoid situations in which JXcore Messaging server is configured for IP set to "localhost" or "127.0.0.1",
but client's are connecting from remote machines using server's public address.
Although this might work, and client's might connect it might also generate errors.
For example some browsers may fail to use WebSockets and will try to switch to older HTTP protocols.

### listenerTimeout

* {Number} default 60000

Defines long polling request time in milliseconds. The maximum value is should not be greater that 120000 (120 seconds).

### mapiVersion

* {String}

Contains version number of JXcore Messaging Server. For example "0.22".
It is used mostly for informational purpose and is displayed for example when server starts from tthe console window.

## Events

### subscribe

* `env` {Object} - see [Object: `env`](#object-env)
* `params` {Object}
    * `req` {Object} - object containing information about client's request
    * `group` {String} - name of the group, to which user subscribes
    * `groups` {Array} - names of the groups, to which user already belongs
* `allow` {Function}

Condition for this event to be fired is that server-side [`enableClientSideSubscription`](#enableclientsidesubscription) option should enabled.
By default it is set to false, and it means, that client's cannot subscribe to a channels nor unsubscribe from them.
In that case the event `subscribe` is never raised.

When `enableClientSideSubscription` is set to true, the `subscribe` event fires whenever client subscribes to a `group` by calling its `Subscribe()` method.
If no listener is attached to this event, user subscribes always.

Attaching to this event gives you control, whether to allow the user to be subscribed or not.
The `allow` argument is a function, which should be called if you want to accept user's subscription request.
Otherwise user will not be subscribed to a `group`, and client's callback will not be invoked.

client-side (index.html):

```js
var callback = function(group) {
    alert("subscribed to " + group);
};

jxcore.Subscribe("group1", callback);
```

server-side (my_server.js):
```js
server.on("subscribe", function(env, params, allow) {
    // don't ever subscribe to "admin_group"
    if (params.group !== "admin_group") {
        allow();
    }
});
```

### unsubscribe

* `env` {Object} - see [Object: `env`](#object-env)
* `params` {Object}
    * `req` {Object}
    * `group` {String}
    * `groups` {Array}
* `allow` {Function}

Condition for this event to be fired is that server-side [`enableClientSideSubscription`](#enableclientsidesubscription) option should enabled.
By default it is set to false, and it means, that client's cannot subscribe to a channels nor unsubscribe from them.
In that case the event `subscribe` is never raised.

When `enableClientSideSubscription` is set to true, the `unsubscribe` event fires whenever client unsubscribes from a `group` by calling its `Unsubscribe()` method.
If no listener is attached to this event, user unsubscribes always.

Attaching to this event gives you control, whether to allow the user to be unsubscribed or not.

See also: [Event: 'subscribe'](#event-subscribe)

### sendToGroup

* `env` {Object} - see [Object: `env`](#object-env)
* `params` {Object}
    * `req` {Object} - object containing information about client's request
    * `group` {String} - group, to which client is sending a message
    * `method` {Array} - name of the method called by a client with `SendToGroup()`
    * `message` {Object} - contents of a message
* `allow` {Function}

This event fires, whenever user calls `SendToGroup()` method.
If no listener is attached to this event, messages are always sent.

Attaching to this event gives you control, whether to allow for sending particular message or not.

The `allow` argument is a function, which should be called if you want to let the message to be sent.

client-side (index.html):

```js
    btnSend.onclick = function() {
        jxcore.SendToGroup('group1', "clientsMethod", { txt: "my_message"} );
    };
```

server-side (my_server.js):

```js
server.on('sendToGroup', function(env, params, allow) {
    if (params.group === "group" && params.method === "clientsMethod") {
        allow();
    }
});
```

## Object: env

* `ClientId` {String}
* 'ApplicationName` {String}
* 'SessionID` {String}
* `Index` {number}

This object is passed to some of the methods described in this document. It contains information about a call made from a client's side.
For example, when a client invokes server's method by using `Call()`, the server's method will receive `env` object apart from argument passed to `Call()`.

`Index` represents id of client's callback and is used by `sendCallback()` method.

## allowedResourceTypes

List of supported types for resource files.

* `png` - image/png
* `jpg` - image/jpeg
* `jpeg` - image/jpeg
* `gif` - image/gif
* `html` - text/html
* `css` - text/css
* `js` - text/javascript
* `woff` - application/octet-stream
* `ttf` - application/octet-stream
* `svg` - application/octet-stream
* `otf` - application/octet-stream
* `eot` - application/octet-stream

If you want to add new `avi` type, you can do it like this:

```js
server.allowedResourceTypes.avi = "video/avi";
```

Or you can delete the existing one:

```js
delete server.allowedResourceTypes.woff;
```

## addJSMethod(name, method)

* `name` {String}
* `method` {Function}
    * `env` {Object}
    * `params` {Object}

Adds custom method to the application and it can be called from the client’s side.
Method can receive two parameters: [`env`](#object-env) as well as `params`, which is the value sent by client.

client-side (index.html):

```html
<script type="text/javascript">
    jxcore.Call("chatMessage", "hello");
</script>
```

server-side (my_server.js):

```js
server.addJSMethod("chatMessage", function (env, params) {
    // params contains "Hello" string
    server.sendToAll("addText", params );
});
```

## getConfig(key)

* `key` {String}

Get value of application’s parameter.

## linkAssets(urlPath, JXP)

* `urlPath` {String}
* `JXP` {Object}

Links assets embedded inside compiled JX file and defines them as static resource used by the application.

For information, how to compile JX packages, see [compile](jxcore-feature-packaging-code-protection.markdown) command.

The `urlPath` parameter is an url path, from which your application will access the asset files.
Please note, that it will be combined with `urlPath` provided in `setApplication()` method.

The `JXP` refers to the object, which is embedded inside compiled JX file, and holds contents of JXP project file. You can access the JXP object by calling `exports.$JXP`.

Let's assume, that your JXP file contains asset definition:

```js
{
    ...
    ...
    "assets": [
       "README.txt",
       "Licence.txt"
    ],
    ...
}
```

Then you can link them to your application in a runtime:

```js
server.linkAssets("/files", exports.$JXP);
```

Now, we could access it for example with a browser:

    http://host:port/chat/files/README.txt

Please note, that "/chat" part is a root path for entire application (provided in `setApplication()`), while "/files" part is an argument from the `linkAssets()`.
Now, the both combine into "/chat/files".

## linkResource(urlPath, filePath)

* `urlPath` {String}
* `filePath` {String}

Defines static resource file used by the application.

The `urlPath` is a path, from which your application will access the resource file. Please note, that it will be combined with `urlPath` provided in `setApplication()` method.

The `filePath` is server's filesystem path (relative or absolute) to the resource file.

```js
server.linkResource("/app", ["./index.html", "text/html" ]);
```

Now, we could access it for example with a browser:

    http://host:port/chat/app

Please note, that "/chat" part is a root path for entire application (provided in `setApplication()`), while "/app" part is an argument from the `linkResource()`.
Now, the both combine into "/chat/app".

## linkResourcesFromPath(url, dir)

* `url` {String}
* `dir` {String}

Allows linking multiple resources recursively from a given directory.

Adding the whole ./assets directory (relative path from JXcore server's working directory).

```js
server.linkResourcesFromPath("/assets/", "./assets/");
```
Now, we could access it for example with a browser:

    http://host:port/chat/assets

Please note, that "/chat" part is a root path for entire application (provided in `setApplication()`), while "/assets" part is an argument from the `linkResourcesFromPath()`.
Now, the both combine into "/chat/assets".

## sendCallBack(env, params)

* `env` {Object}
* `params` {Object}

Calls the callback method at specific client. The `env` is the same parameter, which you received as argument for a custom method defined by you with `addJSMethod()`,
while `params` is an argument for the callback. It can be anything – string, number or json literal object containing many values.

```js
server.addJSMethod("serverMethod", function (env, params) {
    // server responses to a client by calling it's callback
    server.sendCallBack(env, params + " World!");
});
```

## sendToGroup(groupName, methodName, params)

* `groupName` {String}
* `methodName` {String}
* `params` {Object}

Sends message to a group of subscribers, currently connected to the application. The `methodName` is the name of the method invoked on the client's side (every subscriber of this group should has this method defined), while `params` is an argument for that method.

Server can send message to group of subscribers, but they need to subscribe first. See `Subscribe()`.

In the code below, whenever client will call server's `sendFromServer()` method with "Hello" as params argument, the server for each client subscribed to *programmers* channel, will invoke his `clientCustomMethod()` passing there "Hello World!" string.

```js
server.addJSMethod("sendFromServer", function (env, params) {
    server.sendToGroup("programmers", "clientCustomMethod", params + "World!");
});
```

## setApplication(applicationName, urlPath, secretKey)

* `applicationName` {String}
* `urlPath` {String}
* `secretKey` {String}

Defines new application with specified `applicationName` and default root `urlPath`. Every assets or resources added to this application will start from this path.

The `secretKey` parameter is for encrypting the client locator and can be obtained from jxcore control panel.

Server-side (*my_server.js*):

```js
server.setApplication("ChatSample", "/chat", "NUBISA-STANDARD-KEY-CHANGE-THIS");
```

Client-side (*index.html*):

```html
<script src="/chat/jx?ms=connect" type="text/javascript"></script>
```

Please note, that the "/chat" part in the url is the `urlPath` parameter described above.

## setConfig(key, value)

* `key` {String}
* `value` {String}

Defines value for application’s parameter. Allows changing server configuration.

## sendToAll(methodName, params)

* `methodName` {String}
* `params` {Object}

Send message to all of the clients connected currently to the application.

## setEngine(app)

* `app` {Object}

Defines the server engine (like express)...

## start(options)

* `options` {Object}

Starts JXcore application with optional `options` for the server. Once started, it will be accessible to all clients.

## subscribeClient(env, groupName)

* `env` {Object} - see [Object: `env`](#object-env)
* `groupName` {String}

Subscribes the client to a `groupName`, or channel. This is server-side equivalent of `Subscribe()` method from client's API.

From now on, messages sent to that group by any other subscriber or server will be received by the client.
Also the client can send messages to this group – see `SendToGroup()` method.

This method should be used in one of server's custom method defined with `addJSMethod()`,
because it requires the [`env`](#object-env) object containing information about client's call.

For example, when client calls:

```js
    jxcore.Call("someMethod", true);
```

on the server-side you can use it to subscribe him to a group:

```js
server.addJSMethod("someMethod", function(env, param) {
    if (param === true) {
        server.subscribeClient(env, "testGroup");
    }
});
```

Of course you may apply any logic or algorithm for making decision, whether subscribe the client and how.

## unSubscribeClient(env, groupName)

* `env` {Object} - see [Object: `env`](#object-env)
* `groupName` {String}

Unsubscribes the client from a `groupName`, or channel. This is server-side equivalent of `Unsubscribe()` method from client's API.

From now on, messages sent to that group cannot be received by this client.
Also, the client cannot messages to that group.

The usage is analogous to the `subscribeClient()` method.

client-side:

```js
    jxcore.Call("someMethod", false);
```

server-side:

```js
server.addJSMethod("someMethod", function(env, param) {
    if (param === true) {
        server.subscribeClient(env, "testGroup");
    } else {
        server.unSubscribeClient(env, "testGroup");
    }
});
```

# API JavaScript Client

## Events

### document.onjxready

There is a special event `document.onjxready`, which is called right after the JXcore script is loaded:

```html
<script src="/helloworld/jx?ms=connect" type="text/javascript">
```

Inside that event we can start to use jxcore object and for example we attach to the following events: `OnClose`, `OnError` and `Start()` method. The last one is the most important one for us. Please see the comments in the code above to catch the idea.

```js
<script type="text/javascript">

    document.onjxready = function () {

        jxcore.Start(function (status) {

            var send_button = document.getElementById('send_button');
            // let's enable button, right now the script is loaded
            send_button.disabled = "";

            var msg = document.getElementById('msg');
            msg.innerHTML += "Connected.<BR>";

            var callback = function (s) {
                msg.innerHTML += s + "<BR>";
            };

            send_button.onclick = function () {
                // let's call the server-side method "serverMethod" from the client!
                // in turn, as a response, the backend service will invoke
                // client's local "callback" defined above!
                jxcore.Call("serverMethod", "Hello", callback);
            };
        });

        jxcore.OnClose = function (reconnecting) {
            msg.innerHTML += "Disconnected.<BR>";
        };

        jxcore.OnError = function (err) {
            msg.innerHTML += err;
        }
    };
</script>
```

### OnClose

* `reconnecting` {Boolean}

This event is fired every time, when the client loses connection with the server.
The `reconnecting` parameter has a `true` value, if client already tries to reconnect.

```js
jxcore.OnClose = function (reconnecting) {
    msg.innerHTML += "Disconnected.<BR>";
};
```

### OnError

* `err` {String}

This event is emitted every time, the error occurs.

```js
jxcore.OnError = function (err) {
    msg.innerHTML += err;
}
```

### OnSubscription

* `subscribed` {Boolean}
* `groupName` {String}

This event is raised when client gets subscribed to a group or unsubscribed from it by a call made from a server-side (`subscribeClient()` or `unSubscribeClient()` methods).

The `subscribed` value indicates, whether this event was raised as a result of subscription (`true`) or unsubscription (`false`) request.
The `groupName` is the name of the group, for which the event occured.

When subscription/unsubscription request was made by a client's method `Subscribe()` or `Unsubscribe()`, the event `OnSubscription` is not raised, but instead you may provide the callback for those methods.
Please refer to their description for more details.

client-side:

```js
jxcore.Call("someMethod", true);

jxcore.OnSubscription = function (subscribed,  group) {
    if (subscribed) {
        alert("event: subscribed to a group " + group);
    } else {
        alert("event: unsubscribed from a group " + group);
    }
};
```

server-side:

```js
server.addJSMethod("someMethod", function(env, param) {
    if (param === true) {
        server.subscribeClient(env, "testGroup");
    }
});
```

## Call(methodName, json, cb)

* `methodName` {String}
* `json` {Object}
* `callback` {Function}
    * `param` {Object}

Invokes specific custom method named `methodName` defined on the server-side and passes to it one parameter `json`.
The client's `callback` is optional, but when provided, it will be called after server completes invoking the method
and will receive `param` argument sent from the server-side.

In the example below we call the server-side method "serverMethod" from the client-side.
In turn, as a response, the backend service will invoke the client's local `callback` function:

```js
var callback = function(param) {
    alert(param);
}

jxcore.Call("serverMethod", "hello", callback);
```

or simply:

```js
jxcore.Call("serverMethod", "hello", function(param) {
    alert(param);
});
```

## Close(tx)

* `tx` {Boolean}

Closes client and disconnects from the server.

The `tx` parameter is optional. If set to `false`, the `OnClose` event will also be invoked with `false` value as an argument: `OnClose(false)`.
If `true` - then `OnClose` event will not get invoked.

## GetClientId()

Gets the id of the client, which is an unique string value.

## ReConnect()

Forces the client to reconnect to the server.

## SendToGroup(groupName, methodName, json)

* `groupName` {String}
* `methodName` {String}
* `json` {Object}

Sends message to all of the clients, that have already subscribed to the specific `groupName`.
The message is passed as `json` argument to the target's method named `methodName`.
The message can be any value, primitive (string, number, etc.) or json literal object.

```js
document.getElementById("btnSend").onclick = function(){
    jxcore.SendToGroup("programmers", "addText", { obj : "value" } );
};
```

The "addText" method should be available on every client which is subscribed to "programmers" group.
While invoking the "addText" method at each client, the server will pass { obj : "value" } as an argument.

## Start(cb)

* `callback` {Function}
    * `status` {Boolean}

Starts JXcore client. Connects to the server, and when it succeeds - the client’s callback `callback` is called.

```js
document.onjxready = function () {
    jxcore.Start(function (status) {

        // here we are, after the client has connected to server
        // we can enable the button now
        var btnSend = document.getElementById('btnSend');
        btnSend.disabled = "";

        // do anything else
        // see tutorials for more usage
    });
};
```

## Subscribe(groupName, callback)

* `groupName` {String}
* `callback` {Function}
    * `groupName` {String}

Subscribes the client to a `groupName`, or channel. From now on, messages sent to that group by any other subscriber will be received by the client.
Also the client can send messages to this group – see `SendToGroup()` method.
After the server will successfully subscribe the client to the `groupName`, the client's `callback` will be called.

```js
jxcore.Subscribe("programmers", function(groupName) {
    alert("subscribed to group: " + groupName);
});
```

## Unsubscribe(groupName, callback)

* `groupName` {String}
* `callback` {Function}
    * `groupName` {String}

Unsubscribes the client from a `groupName`, or channel. From now on, messages sent to that group cannot be received by this client.
After the server will successfully unsubscribe the client from the `groupName`, the client's `callback` will be called.

```js
jxcore.Unsubscribe("programmers", function(groupName) {
    alert("unsubscribed from a group:" + groupName);
});
```

# API Java Client

There is a tutorial showing, how to consume JXcore Messaging API from a [Java Client](http://jxcore.com/messaging-api/#post-665).

## Events

Messaging module for Java defines events in a separate class `ClientEvents`. In order to listen for client's events, we need to instantiate this class and assign to `client.Events` property:

```java
import jxcore.*;

jxcore.ClientEvents events = new ClientEvents(){
    @Override
    public void OnErrorReceived(Client c, String Message) {
        // Error received
    }
    @Override
    public void OnClientConnected(Client c) {
        // Client is connected
    }
    @Override
    public void OnClientDisconnected(Client c) {
        // Client is disconnected
    }
    @Override
    public void OnEventLog(Client c, String log, LogLevel level) {
        // get the event log from here
    }
    @Override
    public void OnSubscription(Client c, Boolean subscribed, String group) {
        // Client was subscribed to a group or unsubscribed from a group
        // by a server-side call
    }
};
//now we may define this listener into our Client instance
client.Events = event;
```

### OnErrorReceived

* `client` {jxcore.Client}
* `message` {String}

This event is emitted whenever an error occurs at the `client`.

### OnClientConnected

* `client` {jxcore.Client}

This event is emitted after the `client` successfully connects to the server.

### OnClientDisconnected

* `client` {jxcore.Client}

This event is emitted when the `client` loses its connection with the server.

### OnEventLog

* `client` {jxcore.Client}
* `log` {String}
* `level` {jxcore.LogLevel}

This event is fired whenever the `client` logs an information `log` message. The `level` parameter is an enumeration value and can have one of the following: *Informative* or *Critical*.

### OnSubscription

* `client` {jxcore.Client}
* `subscribed` {Boolean}
* `groupName` {String}

This event is raised when client gets subscribed to a group or unsubscribed from it by a call made from a server-side (`subscribeClient()` or `unSubscribeClient()` methods).

The `subscribed` value indicates, whether this event was raised as a result of subscription (`true`) or unsubscription (`false`) request.
The `groupName` is the name of the group, for which the event occured.

When subscription/unsubscription request was made by a client's method `Subscribe()` or `Unsubscribe()`, the event `OnSubscription` is not raised, but instead you may provide the callback for those methods.
Please refer to their description for more details.

## new Client(localTarget, appName, appKey, url, port, secure)

* `localTarget` {Object}
* `appName` {String}
* `appKey` {String}
* `url` {String}
* `port` {int}
* `secure` {boolean}
* `resetUID` {boolean}

Creates an instance of JXcore Java Client with specified application name `appName` and application key `appKey` (which is obtainable from web control panel).
The `url` parameter specifies JXcore server URL, e.g. *sampledomain.com* or *120.1.2.3*. You can also enable SSL support with `secure` parameter.

Setting `resetUID` as `true` will reset the unique instance id (session id).

The first argument `localTarget` is an instance of a local class, which will be answering the calls from server.
In that class you will specify client methods, which will be callable by other clients or the server itself.

```java
import jxcore.*;

Client client = new Client(new CustomMethods(), "channels",
    "NUBISA-STANDARD-KEY-CHANGE-THIS", "localhost", 8000, false, true);
```

## Call(methodName, params, callback)

* `methodName` {String}
* `params` {Object}
* `callback` {jxcore.Callback}

Invokes specific custom method `methodName` defined on the server-side and passes to it `params` value. The `methodName` should also contain the class name and the namespace, e.g. *com.example.MyClass.MyMethod*.

The optional parameter `callback` is the client’s function, which will be called after server completes invoking the method.

In the example below we call the server-side method *serverMethod* from the client-side.
In turn, as a response, the backend service will invoke the client's local `callback` function:

```java
client.Call("serverMethod", "Hello", callback);
```

## Connect()

Starts the client, connects to the server. Returns a boolean value based on the result.

```java
if (client.Connect()) {
    System.out.println("ready!");
}
```

## getClientId()

Gets the string containing unique id of the client.

## SendToGroup(String groupName, String methodName, Object params)

* `groupName` {String}
* `methodName` {String}
* `params` {Object}

Sends message to all clients, that have already subscribed to the specific `groupName`. The message is passed as `params` argument to the target's method named `methodName`.

The "addText" method should be available on every client, which is subscribed to *programmers* group.
While invoking the *addText* method at each client, the server will pass "Hello from client!" as an argument.

```java
client.SendToGroup("programmers", "addText", "Hello from client!");
```

## Subscribe(groupName, callback)

* `groupName` {String}
* `callback` {jxcore.Callback}
    * `groupName` {String}

Subscribes the client to a `groupName`, or channel. From now on, messages sent to that group by any other subscriber will be received by the client.
Also the client can send messages to this group – see `SendToGroup()` method.
After the server will successfully subscribe the client to the `groupName`, the client's `callback` will be called.

```java
try {
     client.Subscribe("programmers", new Callback() {
         @Override
         public void call(Object o) throws Exception {
             System.out.println("Subscribed to " + o.toString());
             client.SendToGroup("programmers", "clientMethod",
             "Hello from client!");
         }
     });
} catch (Exception e) {
     System.out.println("Cannot subscribe.");
}
```

## Unsubscribe(group, callback)

* `groupName` {String}
* `callback` {jxcore.Callback}
    * `groupName` {String}

Unsubscribes the client from a `groupName`, or channel. From now on, messages sent to that group cannot be received by this client.
After the server will successfully unsubscribe the client from the `groupName`, the client's `callback` will be called.

```java
try {
     client.Unubscribe("programmers", new Callback() {
         @Override
         public void call(Object o) throws Exception {
             System.out.println("Unubscribed from " + o.toString());
         }
     });
} catch (Exception e) {
     System.out.println("Cannot unubscribe.");
}
```