File Keeper
===========

What is File Keeper ?
---------------------

File keeper is a daemon that will create a directory at your home directory called file_keeper,
everything that you save in this directory will have its changes saved individually in the .db directory.
Making it very easy to track individual file changes.
Don't worry about your O.S, the file keeper will run on: Linux, Windows and Mac OS.

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
sudo yum install cmake libgit2 glib-devel
```

Mac OS  ( Install homebrew if neeed):
```shell
homebrew install glib libgit2 cmake
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
-----
- [ ] Make it distributed: Spread the files on servers (If the user wants to).
- [ ] Create a GUI for it, so the user can easily recover older versions.
- [ ] Mark files as deleted.

