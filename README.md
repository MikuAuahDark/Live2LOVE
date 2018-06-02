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

Windows Compiling
-----------------

Build instruction for Android is coming soon. iOS build instruction needs contributions.

Look at `sln/`. The project itself uses MSVC 1800/120 (VS2013) platform toolset, but the solution file is created with Visual Studio 2017.
MSVC 1800/120 is what normally used to compile LÖVE, and it's recommended to (because LÖVE ships their VS2013 runtime DLLs).
The rest of this compiling steps makes those assumption.

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

For the lib file, it's bit tricky. It's recommended to copy the x64 libs first then x86 ones.  
Copy `lib/windows/x64/120/Debug/live2d_opengl.lib` from Live2D to `lib/Win32/Debug/live2d_opengl_x64.lib` (yes, notice the `_x64` suffix)  
**For x86, do not add `_x64` suffix!** Do same for Release libraries.

The rest is just opening the solution file, then build.  
The output DLL will be in `sln/{Debug,Release}` for x86 build and `sln/x64/{Debug,Release}` for x64 build.

License
-------

This LÖVE library is licensed under zLib license, excluding the Live2D include files and libraries.  
**Games/things released with this library is subject to Live2D licensing!**
