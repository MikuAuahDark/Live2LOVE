Live2LÖVE
=========

LÖVE module (or library) for loading & rendering Live2D models.  
Require at least LÖVE 11.0 to run.

Live2D Notice
-------------

You need Live2D Cubism 3 SDK for Native to compile this project. This repository doesn't ship it, and you need to retrieve that from Live2D website.
Make sure to grab [Live2D Cubism 3 SDK for Native](https://live2d.github.io/).

Note that distributing your game which uses this library is subject to Live2D licensing.

Lua
---

The Lua include file assume it's built against LuaJIT 2.0.5 (LÖVE for Windows). If not, please modify the include and the lib files accordingly.

Compiling
---------

You need [CMake](https://cmake.org/) to generate the project. Recent version is strongly recommended, CMake 3.6 is minimum.

You need the Live2D Cubism 3 SDK for Native as described above. Your downloaded zip should have this structure

```
Cubism3SDKforNative-<version>
+ Core

* some are omitted because they're not necessary for Live2LÖVE
```

This copying instructions assume the directory exists in Live2LÖVE. If it's not exist, create it!

Copy the `Core` folder to `live2d/Core` folder in Live2LÖVE (merge).

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

### Linux

**Note: Only 64-bit target is supported!**

Build libLive2LOVE.so

```
cmake -Bbuild -H. -DBUILD_SHARED_LIBS=1
cmake --build build --config Release
```

Note that `libLive2LOVE.so` depends on `libLive2DCubismCore.so`. Linking to `libLive2DCubismCore.a` is currently
unsupported as Live2LOVE requires `-fPIC` but their static library aren't compiled with such option.

### Android

#### NDK r18 and below

Note that this is only tested to work in NDK **r16b**. NDK r17 and r18 may not work!

Example command-line to build Live2LOVE for Android targetting architecture `armeabi-v7a` with API Level 14:

```
cmake -Bbuild -H. -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a -DCMAKE_SYSTEM_VERSION=14 -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang -DCMAKE_ANDROID_STL_TYPE=c++_shared
cmake --build build --config RelWithDebInfo
```

#### NDK r19 and later

Example command-line to build Live2LOVE for Android targetting architecture `armeabi-v7a` with API Level 16:

```
cmake -Bbuild -H. -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake -DANDROID_STL=c++_shared -DANDROID_TOOLCHAIN=clang -DANDROID_PLATFORM=android-16 -DANDROID_ABI=armeabi-v7a
cmake --build build --config RelWithDebInfo
```

Regardless of NDK version, only C++ shared STL is supported. GNUSTL won't work.

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

PicoJSON is licensed under 2-clause BSD license.
