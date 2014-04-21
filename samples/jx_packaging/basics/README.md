This sample project shows asset access / folder browsing / method hiding inside a JX protected file.

**Create the package**

Browse the readFile folder from a terminal/console window and;

```$ jx compile sample1.jxp```

Above command creates a JX binary file under the current path. Create another folder as shown below
and move the JX file there.

For OSX / Linux
```
$ mkdir ../sub
$ mv sample1.jx ../sub
$ cd ..
$ jx test.js
```

For Windows
```
$ mkdir ..\sub
$ move sample1.jx ..\sub
$ cd ..
$ jx test.js
```

**The expected output is**

readFileSync ...


asset.txt This is a string from asset.txt file

other.txt This is a string from other.txt file under a subFolder



readDirSync ...


subFolder [ 'other.txt' ]


Source hiddenFunction : function () { [hidden code] }

Source normalFunction : function (){

   console.log("Hello!");

}



