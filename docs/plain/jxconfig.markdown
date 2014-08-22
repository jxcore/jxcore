# jx.config

    Stability: 3 - Beta

You can set the below list of limitations per application & for entire system (all the JXcore apps). In order to define a global config, visit the JXcore executable binary path and create a `jx.config` file (for Windows, node.config)

    {
        "maxCPU":50
    }

The given config will be limiting all the node applications to 50 percent CPU cap. If you want to alter this configuration for a specific application (lets say `/test/myapp/index.js`)

Create a `index.js.jxcore.config` file under `/test/myapp`

    {
        "maxCPU":100
    }

The above sample can be more beneficial in combination with `jx monitor` since, JXcore monitor can automatically restart the application that reaching beyond the CPU limit.


## portTCP, portTCPS: (integer)

Limits the application’s TCP listening port range to given port addresses. When this variable is defined, the process is restricted to the defined ports instead the ones given in the JavaScript code.

example:

    index.js.jxcore.config:
    {
    "portTCP": 8080
    }

    index.js:
    var http = require('http');
    http.createServer(function (req, res) {
        res.writeHead(200, {'Content-Type': 'text/plain'});
    res.end('Hello World\n');
    }).listen(1337);

The sample JavaScript application above tries to listen port 1337. Since the predefined port for that process is 8080, the application automatically listens 8080 instead 1337. The application doesn’t know that it actually listening 8080 (this can be changed on later versions).  ‘portTCP’ affects both UDP and TCP connections. ‘portTCPS’ only affects the port number for ‘https’ module or secure socket connections.

portTCP, and portTCPS has no effect on global configuration.

## allowMonitoringAPI

You may use this parameter to disable access to [`jxcore.monitor`](jxcore-monitor.markdown#api) methods within an application.
The default value is *true*.

    jx.config:
    {
        "allowMonitoringAPI": false
    }


## allowCustomSocketPort: (boolean)

This parameter defines whether the process can listen any socket or not. Setting this parameter to false keeps application from listening any socket but the ones given by portTCP, portTCPS. If the value is false and neither portTCP, portTCPS are defined, the application won’t be listening any port and the htttp/s server will fail. This setting can be set both global and per application.


example:
    jx.config
    {
        "allowCustomSocketPort":false
    }

    /tmp/test/index.js.jxcore.config
    {
        "portTCP":9090
    }

The process above will create an http server and listen to port 9090 automatically. On the other hand, if the same app creates a child process, the child process won’t be able to create an http server since the global rule applies to the apps with no special configuration.

JXcore also provides a ‘globalApplicationConfigPath’ option to keep applications from defining local configurations per applications.


## maxMemory: (integer)

Limits the application’s maximum memory size defined in KB. If a process reaches beyond the maxMemory value, JXcore automatically ‘aborts’ it. This setting can be set both global and per application.

example:

    jx.config
    {
        "maxMemory":65536
    }

    /tmp/test/index.js.jxcore.config
    {
        "maxMemory":131072
    }

Given the above sample, all the processes are limited to 64 Mb. memory except ‘tmp/test/index.js’ which can use up to 128 Mb.


## allowSysExec: (boolean)

Using this configuration parameter, the application can be restricted to create other processes, including spawning a child process. This setting can be set both global and per application.


## maxCPU: (integer)

Limits the maximum CPU usage per process. If the process reaches beyond the defined maximum CPU percentage, JXcore terminates it. The CPU usage comparison is based on the last two seconds of activations. (Configurable maxCPUInterval) This setting can be set on both global and application’s configuration file.

example:

    jx.config
    {
        "maxCPU":10
    }

    /tmp/test/index.js.jxcore.config
    {
        "maxCPU":50
    }

Given the sample above, all processes are limited to 10% CPU usage except `tmp/test/index.js` . It can use up to 50%

Please remind that the maximum number for the maxCPU parameter is not limited to hundred (100) on multi core systems. JXcore supports multithreaded node.JS execution hence the application may consume i.e %600 on an 8 core system.


## maxCPUInterval: (integer)

Defines the sampling period for maxCPU in seconds. Default value is 2, minimum value is 1. This setting can be set both global and per application.


## globalModulePath: (string)

Defines the system wide node.js module archive location. If this value is defined, all running JXcore Node.JS processes will be checking this folder for the required modules first. This setting is global only hence cannot be defined per application.

    jx.config
    {
        "globalModulePath":"/somefolder"
    }

This parameter can be useful on systems when the administrator wants to control the node module pool.

## allowLocalNativeModules: (boolean)

Limits the process from using custom native module (“.node file”) by forcing it to load the native module from jx.config’s ’globalModulePath’ . If the native module wasn’t installed on globalModulePath, the process will receive an exception saying that the file doesn’t exist. This setting can be set on both global and application’s configuration file.


## globalApplicationConfigPath: (string)

By default, JXcore reads the application config from its location. For example; if the application file `index.js` resides under `/temp/app` folder, JXcore simply searches for `/temp/app/index.js.jxcore.config` file right before the execution of the application file. Defining ‘globalApplicationConfigPath’ keeps JXcore from reading the local application folder instead JXcore searches for the config file under the configured location. This setting parameter is global only.

Returning back to the scenario above, if the ‘globalApplicationConfigPath’ is defined as shown below;

    jx.config
    {
        "globalApplicationConfigPath": "/opt/configs"
    }

and assuming the application is available from /temp/app/index.js, the config file for the application must be located as;

`/opt/configs/_temp_app_index.js.jxcore.config`

The rule is to replace all the [ /, \, : ] given special characters to `_` (underline) and save the configuration file into global path.

The reasoning behind this option is to keep processes from defining their own configurations or overriding the global configuration.


## npmjxPath: (string)

JXcore can install node.js modules from npm without requiring the npm software installed separately. (try `jx install express`) In order to accomplish this, JXcore downloads npm and related software into user’s home folder (under .jx folder). In some cases the user home folder may not be available hence this parameter can be used to override the location for npm for JXcore. This setting is global only hence cannot be configured per application.

    jx.config
    {
        "npmjxPath": "/temp/npm"
    }
