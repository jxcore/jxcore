# Mesaging

Using JXcore Messaging API, your clients can easily communicate with the server (backend service) as well as with other clients.

Simply create a custom method. It can be invoked either by any of the clients or by the server itself.

If you want to create an online game, chat application, or any other project for multiple users – JXcore Messaging API is for you.

There are some tutorials for using Messaging API available here: [My first JXcore Messaging API Server](http://jxcore.com/messaging-api/#post-652).

# API Server

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

Adds custom method to the application. This method can be called from the client’s side.

server-side (my_server.js):

```js
server.addJSMethod("chatMessage", function (env, params) {
    server.sendToAll("addText", params );
});
```

client-side (index.html):

```html
<script type="text/javascript">
    jxcore.Call("chatMessage", "hello");
</script>
```

## getConfig(key)

* `key` {String}

Get value of application’s parameter.

## linkAssets(urlPath, JXP)

* `urlPath` {String}
* `JXP` {Object}

Links assets embedded inside compiled JX file and defines them as static resource used by the application.

For information, how to compile JX packages, see [compile](jxcore-feature-packaging-code-protection.html) command.

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

Calls the callback method at specific client. The `env` is the same parameter, which you received as argument for a custom method defined by you with `addJSMethod()`, while `params` is an argument for the callback. It can be anything – string, number or json literal object containing many values.

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

# API JavaScript Client

## Event: 'document.onjxready'

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

## Event: 'OnClose'

* reconnecting {Boolean}

This event is fired every time, when the client loses connection with the server. The `reconnecting` parameter has a `true` value, if client already tries to reconnect.

```js
jxcore.OnClose = function (reconnecting) {
    msg.innerHTML += "Disconnected.<BR>";
};
```

## Event: 'OnError'

* err {String}

This event is emitted every time, the error occurs.

```js
jxcore.OnError = function (err) {
    msg.innerHTML += err;
}
```

## jxcore.Call(methodName, json, cb)

* methodName {String}
* json {Object}
* cb {Function}

Invokes specific custom method named `methodName` defined on the server-side and passes to it one parameter `json`. The client's `cb` callback is optional, but when provided, it will be called after server completes invoking the method.

In the example below we call the server-side method *serverMethod* from the client-side.
In turn, as a response, the backend service will invoke the client's local *callback* function:

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

## jxcore.Close(tx)

* tx {Boolean}

Closes client and disconnects from server.

The `tx` parameter is optional. If set to `false`, the `OnClose` event will also be invoked with `false` value as an argument: `OnClose(false)`. If `true` - `OnClose` event will not get invoked.

## jxcore.GetClientId()

Gets the id of the client, which is an unique string value.

## jxcore.ReConnect()

Forces the client to reconnect to the server.

## jxcore.SendToGroup(group, methodName, json)

* groupName {String}
* methodName {String}
* json {Object}

Sends message to all clients, that have already subscribed to the specific `groupName`. The message is passed as `json` argument to the target's method named `methodName`.
The message can be any value, primitive (string, number, etc.) or json literal object.

The *addText* method should be available on every client, which is subscribed to *programmers* group.
While invoking the *addText* method at each client, the server will pass { obj : "value" } as an argument.

```js
document.getElementById("btnSend").onclick = function(){
    jxcore.SendToGroup("programmers", "addText", { obj : "value" } );
};
```

## jxcore.Start(cb)

* cb {Function}

Starts JXcore client. Connects to the server, and when it succeedes - the client’s callback `cb` is called.

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

## jxcore.Subscribe(group, callback)

* group {String}
* callback {Function}

Subscribes the client to a `group`, or channel. From now on, messages sent to that group by any other subscriber will be received by the client. Also the client can send messages to this group – see `jxcore.SendToGroup()` method.
After the server will successfully subscribe the client to the `group`, the client's `callback` will be called.

```js
jxcore.Subscribe("programmers", function() {
    alert("subscribed");
});
```

## jxcore.Unsubscribe(group, callback)

* group {String}
* callback {Function}

Unsubscribes the client from a `group`, or channel. From now on, messages sent to that group cannot be received by this client.
After the server will successfully unsubscribe the client from the `group`, the client's `callback` will be called.

```js
jxcore.Unsubscribe("programmers", function() {
    alert("unsubscribed");
});
```

# API Java Client

There is a tutorial showing, how to consume JXcore Messaging API from a [Java Client](http://jxcore.com/messaging-api/#post-665).

## Events

Messaging module for Java defines events in a separate class `jxcore.ClientEvents`. In order to listen for client's events, we need to instantiate this class and assign to `client.Events` property:

```java
import jxcore.*;

jxcore.ClientEvents events = new ClientEvents(){
    @Override public void OnErrorReceived(Client c, String Message) {
        //Error received
    }
    @Override public void OnClientConnected(Client c) {
        //Client is connected
    }
    @Override public void OnClientDisconnected(Client c) {
        //Client is disconnected
    }
    @Override public void OnEventLog(Client c, String log, LogLevel level) {
        //get the event log from here
    }
};
//now we may define this listener into our Client instance
client.Events = event;
```

### Event: 'OnErrorReceived'

* client {jxcore.Client}
* message {String}

This event is emitted whenever an error occurs at the `client`.

### Event: 'OnClientConnected'

* client {jxcore.Client}

This event is emitted after the `client` successfully connects to the server.

### Event: 'OnClientDisconnected'

* client {jxcore.Client}

This event is emitted when the `client` loses its connection with the server.

### Event: 'OnEventLog'

* client {jxcore.Client}
* log {String}
* level {jxcore.LogLevel}

This event is fired whenever the `client` logs an information `log` message. The `level` parameter is an enumeration value and can have one of the following: *Informative* or *Critical*.

## new Client(localTarget, appName, appKey, url, port, secure)

* localTarget {Object}
* appName {String}
* appKey {String}
* url {String}
* port {int}
* secure {boolean}
* resetUID {boolean}

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

* methodName {String}
* params {Object}
* callback {jxcore.Callback}

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

* groupName {String}
* methodName {String}
* params {Object}

Sends message to all clients, that have already subscribed to the specific `groupName`. The message is passed as `params` argument to the target's method named `methodName`.

The *addText* method should be available on every client, which is subscribed to *programmers* group.
While invoking the *addText* method at each client, the server will pass "Hello from client!" as an argument.

```java
client.SendToGroup("programmers", "addText", "Hello from client!");
```

## Subscribe(group, callback)

* group {String}
* callback {jxcore.Callback}

Subscribes the client to a `group`, or channel. From now on, messages sent to that group by any other subscriber will be received by the client. Also the client can send messages to this group – see `jxcore.SendToGroup()` method.
After the server will successfully subscribe the client to the `group`, the client's `callback` will be called.

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

* group {String}
* callback {jxcore.Callback}

Unsubscribes the client from a `group`, or channel. From now on, messages sent to that group cannot be received by this client.
After the server will successfully unsubscribe the client from the `group`, the client's `callback` will be called.

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