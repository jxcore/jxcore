# CONTRIBUTING

JXcore is an open source / open governance project and publicly available [on Github](https://github.com/jxcore/jxcore). We are open to any type of contributions; 

 - Feature ideas 
 - Bug fixing / feature implementation
 - Tutorials, talks, ... 
 - JXcore.io / Docs ... in other languages 
 - JXcore.io look and feel updates
 
Feel free to share your concerns / questions / additions from the [issue tracker](https://github.com/jxcore/jxcore/issues) 

### FORK it!

Fork the project from [GitHub](https://github.com/jxcore/jxcore) and check out
your copy.

```
$ git clone git@github.com:username/jxcore.git
$ cd jxcore
$ git remote add upstream git://github.com/jxcore/jxcore.git
```

Now decide if you want your feature or bug fix to go into the master branch
or the stable branch.  As a rule of thumb, bug fixes go into the stable branch
while new features go into the master branch.

### BRANCH

If you have decided on the proper branch.  Create a feature branch
and start hacking:

```
$ git checkout -b my-feature-branch -t origin/b0.3
```

(Where b0.3 is the latest active branch as of this writing.)


### COMMIT

Make sure git knows your name and email address:

```
$ git config --global user.name "J. Random User"
$ git config --global user.email "j.random.user@example.com"
```

Writing good commit logs is important.  A commit log should describe what
is changed and why.  Follow these guidelines when writing one:

1. The first line should be 50 characters or less and contain a short
   description of the change prefixed with the name of the changed
   subsystem (e.g. "net: add localAddress and localPort to Socket").
2. Keep the second line blank.
3. Wrap all other lines at 72 columns.

### REBASE

Use `git rebase` (not `git merge`) to sync your work from time to time.

```
$ git fetch upstream
$ git rebase upstream/b0.3  # or upstream/master
```

### TEST

Bug fixes and features should come with tests.  Add your tests in the
test/simple/ directory.  Look at other tests to see how they should be
structured (license boilerplate, common includes, etc.).

For C,C++ files we have a pre-built `.clang-format` file. Google `clang-formatting plugin` for your preferred development IDE and use our configuration file from the root folder of the project.

For JS files, Make sure the linter is happy and that all tests pass.  Please, do not submit
patches that fail either check.
```
$ make jslint test
```


### PUSH

```
$ git push origin my-feature-branch
```

Go to https://github.com/username/jxcore and select your feature branch.  Click
the 'Pull Request' button and fill out the form.

Pull requests are usually reviewed within a few days.  If there are comments
to address, apply your changes in a separate commit and push that to your
feature branch.  Post a comment in the pull request afterwards; GitHub does
not send out notifications when you add commits.


### CONTRIBUTOR LICENSE 

Please note that, we accept the contributions under MIT terms as the JXcore project itself is relased under this license.
