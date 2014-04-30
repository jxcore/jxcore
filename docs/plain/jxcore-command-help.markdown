
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

* [compile](jxcore-feature-packaging-code-protection.markdown#jxcore-features-package-manager-compile) - compile jx file from jxp project
* [install](jxcore-command-install.markdown) - installs a module from the repository
* [mt](jxcore-command-mt.markdown) - multithread the given application (no-keep)
* [mt-keep](jxcore-command-mt.markdown) - multithread the given application (keep alive)
* [package](jxcore-feature-packaging-code-protection.markdown#jxcore-features-package-manager-package) - create jxp project from the folder (recursive)

Other options are:

* `-jxv`, `--jxversion` - print jxcore's version
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
