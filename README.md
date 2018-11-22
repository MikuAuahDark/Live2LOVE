Live2LÖVE
=========

LÖVE module (or library) for loading & rendering Live2D models.  
Require at least LÖVE 11.0 to run.

Live2D Notice
-------------

You need Live2D Cubism SDK v2.1 to compile this project. This repository doesn't ship it, and you need to retrieve that from Live2D website. Make sure to grab 
[Live2D Cubism SDK v2.1 for OpenGL (Cocos2d-x) development](http://sites.cybernoids.jp/cubism-sdk2/opengl2-1).

Note that distributing your game which uses this library is subject to Live2D licensing.

Lua
---

The Lua include file assume it's built against LuaJIT 2.0.5 (LÖVE for Windows). If not, please modify the include and the lib files accordingly.

Compiling
---------

This is build instruction for Windows and Android. iOS build instruction needs contributions.

You need [CMake](https://cmake.org/) to generate the project. Recent version is strongly recommended.

You need the Live2D Cubism SDK v2.1 as described above. Your downloaded zip should have this structure

```
Live2D Cubism SDK for OpenGL v2.1.06
+ framework
+ include
+ lib
++ windows
+++ {x86,x64}
++++ 120
+++++ {Debug,Release}
++++++ live2d_opengl.lib

* some are omitted because they're not necessary for Live2LÖVE
* {a,b} means there's both folder "a" and "b" with same folder structure
```

This copying instructions assume the directory exists in Live2LÖVE. If it's not exist, create it!

Copy the `framework` folder from Live2D to `framework` folder in Live2LÖVE (merge).  
Copy the `include` **contents** from Live2D to `include/live2d` folder in Live2LÖVE.  
Copy the `lib` folder from Live2D to `lib/live2d` folder in Live2LÖVE.  

Once all set, you should be able to build the project with CMake like this (example for Windows)

```
cmake -G "Visual Studio 15 Win64" -T v120_xp -H. -Bbuild -DCMAKE_INSTALL_PREFIX:PATH=./output
cmake --build build --config Release --target install
```

It will generate both static and shared library of the project in `output` folder.

Documentation
-------------

The documentation is generated with [LDoc](https://github.com/stevedonovan/LDoc). You can generate the docs with this command

```
ldoc -c ldocConfig.ld -d docs
```

License
-------

This LÖVE library is licensed under zLib license, excluding the Live2D include files and libraries.  
**Games/things released with this library is subject to Live2D licensing!**
