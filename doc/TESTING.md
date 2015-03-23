# To run the tests:

## Unix/Macintosh:

    make test

The command above executes basic tests from *simple* and *message* subfolders of *test* directory.

However, you can also run tests for specified folders, like:

    make test test-simple test-message

Other category of tests resides in *jxcore* subfolder. They can be run as plain *.js* files, packaged or native packaged.
Extra flags can be provided as parameters - see [test/README.md](../test/README.md) for more details.

The following command is for testing *jxcore* tests as plain .js files:

    make test test-jxcore

or

    make test test-jxcore flags j

As packages:

    make test test-jxcore flags p

As native packages:

    make test test-jxcore flags n

All at once:

    make test test-jxcore flags a

## Windows:

    vcbuild.bat test
    
## Native Interface

Currently native interface tests are *nix only. (We would appreciate if somebody would add Windows support for the tests. This is indeed not necessary. JXcore native interface 'jx-ni' works also for Windows applications.)

In order to run JX-ni tests you need JXcore is installed on your system. Either compile from the sources or download it from [here](http://jxcore.com/Downloads) 

Assuming you are under the root folder of the project; first you should compile the project as a static library. Let's say you want to do it for SpiderMonkey build. 
```
> sudo ./configure --prefix=/testBin --engine-mozilla --static-library
> sudo make install
```

Now you can test the native interface;
```
> cd tests/native-interface
> ./run-tests.js /testBin sm 50
```

Number 50 at the end of the second command line corresponds to the number of runs per each test case. It's a bad but helpful hack! For your own sake, you may put 1 instead. If you are planning to contribute on native interface please make sure the test cases are passing 50 runs for both sm and v8.



