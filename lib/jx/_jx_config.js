// Copyright & License details are available under JXCORE_LICENSE file

var fs = require('fs');
var tw = process.binding("jxutils_wrap");
var path = require('path');
var isWindows = process.platform === 'win32';

var app_location = "";
if (process._Monitor) {
  app_location = process.argv[3];
} else if (process._MTED) {
  app_location = process.argv[2];
} else {
  app_location = process.argv[1];
}

var jxconfig = {
  maxMemory: null,
  allowSysExec: null,
  allowLocalNativeModules: true,
  allowCustomSocketPort: null,
  globalModulePath: null,
  maxCPU: null,
  maxCPUInterval: null,
  globalApplicationConfigPath: null,
  npmjxPath: null,
  allowMonitoringAPI: true
};

var global_config_path = process.execPath + ".config";
global_config_path = global_config_path.replace("node.exe.config",
        "node.config");

var applyConfig = function(formPath, content) {
  if (fs.existsSync(formPath) || content) {
    try {
      var conf;
      if (!content) {
        conf = fs.readFileSync(formPath) + "";
      } else
        conf = content;
      conf = JSON.parse(conf);
      for ( var o in jxconfig) {
        if (conf[o] != undefined) {
          jxconfig[o] = conf[o];
        }
      }
    } catch (e) {
      console.error("Application couldn't start because of the configuration file is either corrupted or not accessible.", e);
      process.exit(1);
    }
  }
};

if (!process._EmbeddedSource) applyConfig(global_config_path);

if (jxconfig.npmjxPath && jxconfig.npmjxPath.substr) {
  var _path = jxconfig.npmjxPath;
  _path = _path.trim();
  if (_path[_path.length - 1] == path.sep) {
    _path = _path.substr(0, _path.length - 1);
  }

  Object.defineProperty(process, "__npmjxpath", {
    value: _path,
    configurable: false,
    writable: false
  });
}

delete (jxconfig.npmjxPath);

jxconfig.portTCP = null;
jxconfig.portTCPS = null;

var getMemContent = function(content) {
  var $uw = process.binding('memory_wrap');
  if (!$uw.existsSource(content)) return null;
  var data = new Buffer($uw.readSource(content), 'base64');
  var str = data.toString();
  data = null;

  return str;
};

if (!jxconfig.globalApplicationConfigPath) {
  if (process._EmbeddedSource) {
    // this cannot work, it looks like the content is not available yet
    // applyConfig(null, getMemContent('@' + process.cwd() + path.sep + 'jxcore.config.jx'));
    applyConfig(process.execPath + ".jxcore.config");
  } else
    applyConfig(app_location + ".jxcore.config");
} else {
  app_location = app_location.replace(/[\/]/g, "_").replace(/[\\]/g, "_")
          .replace(/[:]/g, "_");
  var loc = jxconfig.globalApplicationConfigPath + path.sep + app_location
          + ".jxcore.config";

  applyConfig(loc);
}

jxcore.store.shared.set("$$$$$$$$$$$$$", 1); // dummy set

if (jxconfig.globalModulePath && jxconfig.globalModulePath.substr) {
  jxconfig.globalModulePath = jxconfig.globalModulePath.trim();
  if (jxconfig.globalModulePath[jxconfig.globalModulePath.length - 1] != path.sep) {
    jxconfig.globalModulePath += path.sep;
  }

  if (isWindows)
    jxconfig.globalModulePath = jxconfig.globalModulePath.toLowerCase();

  var mod = require('module');
  mod.addGlobalPath(jxconfig.globalModulePath + "node_modules");
  exports.__jx_global_path = jxconfig.globalModulePath;
}

tw.beforeApplicationStart(jxconfig);

if (!jxconfig.allowMonitoringAPI) {
  var mem = process.binding('memory_wrap');
  mem.setSource("_jx_monitor_helper",
          "console.error('monitoring API is disabled for this process');");
  mem.setSource("_jx_monitor",
          "console.error('monitoring API is disabled for this process');");
}
