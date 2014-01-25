File Keeper
===========

What is File Keeper ?
---------------------

File keeper is a file tracking software that will record all file changes allowing the user
to restore old versions of a given file.

To keep track of your files is easy, just move them to the file_keeper directory localized
at you home directory.


What I need to compile it?
--------------------------
1. CMake (version 2.8 or above)
2. glib 2.0
3. libgit2

But I hate dependencies, shortcuts please!
--------------------------
Ok ok.

Linux - Fedora: 
```shell
sudo yum install cmake libgit2-devel glib-devel
```

Mac OS  ( Install homebrew if neeed):
```shell
brew install glib libgit2 cmake
```

Windows:

You must download it yourself.

1. glib - http://www.gtk.org/download/win32.php

2. libgit2 -  http://libgit2.github.com/

3. cmake - http://www.cmake.org/

How do I compile it?
--------------------
```shell
cd /file/keeper/dir
cmake .
make
```

TODO
-------

* Create a GUI application.
* Normalize namespace.
* Transform file_watcher.c in a class.
* Test on Windows


