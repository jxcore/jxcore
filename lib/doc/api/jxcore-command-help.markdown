
# JXcore Command Help

Executing `jx` binary with `--help` option displays list of possible commands.

    > jx --help

## Usage

```bash
jx [options] [ -e script | script.js ] [arguments]
jx debug script.js [arguments]
```

## Options

Many of the options are thoroughly documented in separate sections of the documentation:

* [compile](jxcore-feature-packaging-code-protection.html#jxcore_features_package_manager_compile) - compile jx file from jxp project
* [package](jxcore-feature-packaging-code-protection.html#jxcore_features_package_manager_package) - create jxp project from the folder (recursive)
* [install](jxcore-command-install.html) - install a module from the repository
* [mt](jxcore-command-mt.html) - run the given application with multiple instances (no-keep)
* [mt-keep](jxcore-command-mt.html) - run the given application with multiple instances (keep alive)

Other options are:

* `-jxv`, `--jxversion` - print jxcore's version
* `-jsv`, `--jsversion` - print underlying JS engine's name and version
* `-a`, `--arch` - print processor architecture, for which jx is compiled (process.arch)
* `-v`, `--version` - print corresponding node's version
* `-e`, `--eval` - script evaluate script
* `-p`, `--print` - evaluate script and print result
* `-i`, `--interactive` - always enter the REPL even if stdin does not appear to be a terminal
* `--no-deprecation` - silence deprecation warnings
* `--trace-deprecation` - show stack traces on deprecations
* `--v8-options` - print v8 command line options
* `--max-stack-size=val` - set max v8 stack size (bytes)

## Environment variables

* `NODE_PATH`              ':'-separated list of directories prefixed to the module search path.
* `NODE_MODULE_CONTEXTS`   - Set to 1 to load modules in their own global contexts.
* `NODE_DISABLE_COLORS`    - Set to 1 to disable colors in the REPL
