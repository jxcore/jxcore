# Process Monitor

JXcore offers embedded process monitoring tool. It checks regularly whether the monitored process/processes still exist and restarts them, if required.
When the monitor is running, your applications can subscribe to it and start being monitored.

The Process Monitor is global and can be referenced from anywhere:

```js
var mon = jxcore.monitor;
```

You can control monitor's process from the [command line](jxcore-command-monitor.html) also.

Apart from this feature, JXcore also offers [Internal Recovery](jxcore-feature-internal-recovery.html).

All `jxcore.monitor` methods can be disabled from *jx.config* file.
See [`allowMonitoringAPI`](jxconfig.html#jxconfig_allowmonitoringapi).

## API

### monitor.followMe(callback, waitCallback)

* `callback` {Function}
    * `error` {Boolean}
    * `message` {String}
* `waitCallback` {Function}
    * `delay` {Number}

Subscribes the application for being monitored. The monitor itself should be already running (started from command line).
The `callback` will be invoked when the operation completes, whether with success or failure.

There is also another argument: `waitCallback`. It is invoked in the case when subscription to the monitor is configured to be delayed
(check `start_delay` in [Config file](jxcore-command-monitor.html#jxcore_command_monitor_config_file) section).
In that case `waitCallback` will be called before the `callback` and it will receive one argument with value equal to `start_delay` parameter.
Please note: even if you didn't explicitly define this parameter in a *jx.config* file, the default value will be used.

The following code tries to subscribe to the monitor:

```js
jxcore.monitor.followMe(function (err, txt) {
    if (err) {
        console.log("Did not subscribed to the monitor: ", txt);
    } else {
        console.log("Subscribed successfully: ", txt);
    }
}, function (delay) {
    console.log("Subscribing is delayed by %d ms.", delay);
});
```

Please note, in this sample, the application does nothing else except for subscribing to the monitor, and after that - it just exists.
There are few things to be explained here:

1. The `start_delay` (see [Config file](jxcore-command-monitor.html#jxcore_command_monitor_config_file)) parameter is engaged.
It waits for defined amount of time before really subscribing to the monitor.
In this particular example, because the application ends before `start_delay` elapses - it does not perform subscription for being monitored.

2. Having said that, we can conclude that there is no point to monitor such an application which is not designed for constant running.
If purpose of that application is to just run a task and exit - the monitor unnecessarily would restart it again and again.

### monitor.leaveMe(callback)

* `callback` {Function}
    * `error` {Boolean}
    * `message` {String}

Unsubscribes the application from the monitor. The `callback` will be invoked when the operation completes, whether with success or failure.

```js
jxcore.monitor.leaveMe(function (err, txt) {
    if (err) {
        console.log("Could not unsubscribe from the monitor: ", txt);
    } else {
        console.log("Unsubscribed successfully: ", txt);
    }
});
```

