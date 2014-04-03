
# Process Monitor

JXcore offers embedded process monitoring tool. It checks regularly whether monitored process/processes still exist and respawns them if needed.
When the monitor is running, then your applications can subscribe to it and start being monitored.

Apart from this feature, JXcore offers also [Internal Recovery](jxcore-feature-internal-recovery.markdown).
Please refer there for complementary information.

## Commands

This section describes list of options, which can be used from the command line for controlling the monitor's process.

## run

    > jx monitor run application_file.js

Launches given application and subscribes it for being monitored (invokes `followMe()` method).
The monitor itself should be already running (see `start` command).

Please note, that not every application is a good candidate for being monitored.
Especially those apps, which are not designed for constant running - probably they should not be monitored,
but that depends on developer and application's purpose.

For more information about this, see [Process Monitor API](jxcore-monitor.markdown).

### start

    > jx monitor start

Starts the monitor. Only one instance of monitor's process is allowed on the same machine, so if one already exists, the new will not be started.
The monitor internally creates an http server on http://127.0.0.1:port (the port number by default is 17777 - check `port` in [Config file](#config-file) section).

### stop

    > jx monitor stop

Stops the monitor - shuts down the monitor's process together with all monitored applications.

### restart

    > jx monitor restart

Restarts the monitor. Find the monitor's process and shuts it down together with all monitored applications. After that starts new and fresh monitor's process.

## Web access

While the monitor is running, you can access its http server, e.g. by a browser for the following usage:

* http://127.0.0.1:port/json - gets information (json string format) about currently monitored processes.
* http://127.0.0.1:port/logs - gets contents of a log file (See `log_path` in [Config file](#config-file) section).

## Subscribing application for being monitored

See `followMe()` and `leaveMe()` on [Process Monitor API](jxcore-monitor.markdown).

## Config file

Process Monitor can be configured with *jx.config* file. If you want to use it, you should save it into the same folder, where lies the *jx* executable file.
Below is example of *jx.config* file.

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
* **check_interval** - how often (in milliseconds) should the monitor check if monitored applications processes still exist. Default value: 1000.
* **start_delay** - how late (in milliseconds) should the application subscribe to the monitor. Default value: 2000.
* **log_path** - path and/or name of the log file. If it's only a name (without directory part), it will be written in current working directory, which means the place, from where you started the monitor's process.
You can use some predefined tags inside the log_path. Supported tags are [WEEKOFYEAR], [DAYOFMONTH], [DAYOFYEAR], [YEAR], [MONTH], [MINUTE], [HOUR].

