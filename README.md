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

Once all set, you should be able to build the project with CMake:

### Windows

Build Live2LOVE.dll (shared library)

```
cmake -T v120 -H. -Bbuild -DBUILD_SHARED_LIBS=1
cmake --build build --config Release
```

Build Live2LOVE.lib (static library)

```
cmake -T v120 -H. -Bbuild -DBUILD_SHARED_LIBS=0
cmake --build build --config Release
```

### Android

Assume you're using recent NDK (r16b known to work)

```
cmake -Bbuild -H. -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a -DCMAKE_SYSTEM_VERSION=14 -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang -DCMAKE_ANDROID_STL_TYPE=c++_shared
cmake --build build --config RelWithDebInfo
```

Note that only C++ shared STL is supported. GNUSTL won't work.

It will generate libLive2LOVE.a which you can add to LOVE source as prebuit and then you have to patch `src/modules/love/love.cpp`

```cpp
// put it in place where there are tons of luaopen_*
extern "C" int luaopen_Live2LOVE(lua_State *L)

// put it above `love.nogame` string.
	{ "Live2LOVE", luaopen_Live2LOVE },
```

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
