# To run the tests:

Unix/Macintosh:

    make test

The command above executes basic tests from *simple* and *message* subfolders of *test* directory.

However you can run tests also for specified folders, like:

    make test test-simple test-message

Other category of tests resides in *jxcore* subfolder. They can be run as plain .js files, packaged or native packaged.
Extra flags can provided as parameters - see [test/jxcore/README.md](../test/README.md) for more details.

The following command is testing *jxcore* tests as plain .js files:

    make test test-jxcore

or

    make test test-jxcore flags j

As packages:

    make test test-jxcore flags p

As native packages:

    make test test-jxcore flags n

All at once:

    make test test-jxcore flags a

Windows:

    vcbuild.bat test


