// Copyright & License details are available under JXCORE_LICENSE file

/**
 * Adds --build-from-source (for node-pre-gyp) to process.argv,
 * to force compiling native addons against JXcore
 * rather than allowing for downloading the prebuilt binaries
 */
exports.checkBuildFromSource = function() {

  var cmd = process.argv[2];

  if (typeof jxcore === 'undefined' || !cmd)
    return;

  // for shortcuts refer to deps/npm/lib/npm.js # 64
  var supportedCommands = [
    'install', 'i', 'isntall',
    'rebuild', 'rb',
    'update', 'up'
  ];

  // other than one of supported commands was issued
  if (supportedCommands.indexOf(cmd) === -1)
    return;

  var arg = '--build-from-source';

  for (var a = 0, len = process.argv.length; a < len; a++) {
    if (process.argv[a].indexOf(arg) > -1)
      return;
  }

  process.argv.push(arg);
};