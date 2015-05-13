
# Process Monitor

JXcore offers embedded process monitoring tool. It checks regularly whether the monitored process/processes still exist and restarts them, if required.
When the monitor is running, your applications can subscribe to it and start being monitored.

Apart from this feature, JXcore offers also [Internal Recovery](jxcore-feature-internal-recovery.html).
Please refer there for complementary information.

## Commands

This section describes list of options, which can be used from the command line for controlling the monitor's process.

### kill

    > jx monitor kill application_file.js

Unsubscribes an application from being monitored and then shuts it down.
On linux systems, this command can be executed only by the user which have started the monitor.

If the application was running in few instances, all of them would be killed. See the example below:

    > jx monitor start
    > jx monitor run index.js
    > jx monitor run index.js

We launch `index.js` application as two separate processes and call the following command to kill both of them:

    > jx monitor kill index.js

Other monitored applications as well as the monitor itself will still be running.

### run

    > jx monitor run application_file.js

Launches a given application and subscribes it for being monitored (invokes `followMe()` method).
The monitor itself should be already running (see `start` command).

Please note: not every application is a good candidate for being monitored.
Especially those apps, which are not designed for constant running - they should probably not be monitored,
but that depends on the developer and application's purpose.

For more information about this, see [Process Monitor API](jxcore-monitor.html).

### start

    > jx monitor start

Starts the monitor. Only one instance of monitor's process is allowed on the same machine, so if one already exists, the new will not be started.
The monitor internally creates an http server on http://127.0.0.1:port (the port number by default is 17777 - check `port` in [Config file](#jxcore_command_monitor_config_file) section).

### stop

    > jx monitor stop

Stops the monitor - shuts down the monitor's process together with all monitored applications.
On linux systems, this command can be executed only by the user which have started the monitor.

### restart

    > jx monitor restart

Restarts the monitor. Finds the monitor's process and shuts it down together with all monitored applications.
After that it starts new and fresh monitor's process.

## Web access

While the monitor is running, you can access its http server, e.g. by a browser for the following usage:

* http://127.0.0.1:port/json - gets information (json string format) about currently monitored processes.
* http://127.0.0.1:port/logs - gets contents of a log file (See `log_path` in [Config file](#jxcore_command_monitor_config_file) section).

## Subscribing application for being monitored

See `followMe()` and `leaveMe()` on [Process Monitor API](jxcore-monitor.html).

## Config file

Process Monitor can be configured with a *jx.config* file. If you want to use it, you should save it in the same folder,
where the *jx* executable file is located. Below is an example of a *jx.config* file.

*jx.config*

```js
{
    "monitor": {
        "port": 17777,
        "check_interval": 1000,
        "start_delay": 2000,
        "log_path": "monitor_[WEEKOFYEAR]_[YEAR].log",
    }
}
```

* **port** - it is an http port, which is used by monitored applications to communicate with the monitor. Default value: 17777.
* **check_interval** - how often (in milliseconds) should the monitor check monitored applications' processes
to determine whether they still exist or not. Default value: 1000.
* **start_delay** - how late (in milliseconds) should the application subscribe to the monitor. Default value: 2000.
* **log_path** - path and/or name of the log file. If it is only a name (without directory part),
it will be written in current working directory, which means the place from where you started the monitor's process.
You can use some predefined tags inside the log_path. Supported tags are [WEEKOFYEAR], [DAYOFMONTH], [DAYOFYEAR], [YEAR], [MONTH], [MINUTE], [HOUR].

