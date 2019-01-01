cmake_minimum_required (VERSION 3.1)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
project(Live2LOVE)

# Check Live2D source/header files
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/live2d/ALive2DModel.h")
	message(FATAL_ERROR "Live2D core header files missing")
	return()
elseif(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/framework/Live2DFramework.cpp")
	message(FATAL_ERROR "Live2D framework source files missing")
	return()
endif()

set(LIVE2LOVE_SOURCE_FILES
	# Live2LOVE files
	src/Live2LOVE.cpp
	src/RefData.cpp
	src/Main.cpp
	# Live2D framework files
	framework/L2DBaseModel.cpp
	framework/L2DExpressionMotion.cpp
	framework/L2DEyeBlink.cpp
	framework/L2DMatrix44.cpp
	framework/L2DModelMatrix.cpp
	framework/L2DMotionManager.cpp
	framework/L2DPhysics.cpp
	framework/L2DPose.cpp
	framework/L2DTargetPoint.cpp
	framework/L2DTextureDesc.cpp
	framework/L2DViewMatrix.cpp
	framework/Live2DFramework.cpp
)

# Check Live2D lib files
if(WIN32)
	if(MSVC)
		if(BUILD_SHARED_LIBS)
			add_library(Live2LOVE SHARED ${LIVE2LOVE_SOURCE_FILES})
		else()
			add_library(Live2LOVE STATIC ${LIVE2LOVE_SOURCE_FILES})
		endif()

		# excuse me wtf (MSVC quirks)
		target_compile_definitions(Live2LOVE PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE LUA_BUILD_AS_DLL LUA_LIB)

		# Live2D libraries aren't "DLL"
		option(LIVE2LOVE_MT "Build multi-thread (/MT) version of library" OFF)
		if(LIVE2LOVE_MT)
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
			set(LIVE2LOVE_LIBFILE_NAME "live2d_opengl_mt.lib")
		else()
			set(LIVE2LOVE_LIBFILE_NAME "live2d_opengl.lib")
		endif()

		# Live2D requires OpenGL for some reason, but
		# we don't use their drawing function at all
		find_package(OpenGL REQUIRED)

		if(CMAKE_SIZEOF_VOID_P GREATER 4)
			set(LIVE2LOVE_WIN_PLATFORM "x64")
			target_link_libraries(Live2LOVE "${CMAKE_CURRENT_SOURCE_DIR}/lib/Win32/lua51_x64.lib")
		else()
			set(LIVE2LOVE_WIN_PLATFORM "x86")
			target_link_libraries(Live2LOVE "${CMAKE_CURRENT_SOURCE_DIR}/lib/Win32/lua51.lib")
		endif()

		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/windows/${LIVE2LOVE_WIN_PLATFORM}/${MSVC_TOOLSET_VERSION}/Release/${LIVE2LOVE_LIBFILE_NAME}")
			add_definitions(-DL2D_TARGET_WIN_GL)
			target_link_libraries(Live2LOVE
				debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/windows/${LIVE2LOVE_WIN_PLATFORM}/${MSVC_TOOLSET_VERSION}/Debug/${LIVE2LOVE_LIBFILE_NAME}"
				optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/windows/${LIVE2LOVE_WIN_PLATFORM}/${MSVC_TOOLSET_VERSION}/Release/${LIVE2LOVE_LIBFILE_NAME}"
			)
			target_link_libraries(Live2LOVE ${OPENGL_LIBRARIES})
		else()
			message(FATAL_ERROR "Missing Live2D lib files for MSVC ${MSVC_TOOLSET_VERSION} ${LIVE2LOVE_WIN_PLATFORM} or not supported")
			return()
		endif()
	else()
		message(FATAL_ERROR "Live2D only support Windows using MSVC compiler")
		return()
	endif()
elseif(APPLE)
	message(FATAL_ERROR "TODO")
	return()
elseif(ANDROID)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/android/${CMAKE_ANDROID_ARCH_ABI}/liblive2d.a")
		add_library(Live2LOVE STATIC ${LIVE2LOVE_SOURCE_FILES})
		find_path(GLES2_INCLUDE_DIR GLES2/gl2.h HINTS ${CMAKE_ANDROID_NDK})
		find_library(GLES2_LIBRARY libGLESv2.so HINTS ${GLES2_INCLUDE_DIR}/../lib)
		target_compile_definitions(Live2LOVE PRIVATE L2D_TARGET_ANDROID L2D_TARGET_ANDROID_ES2)
		target_link_libraries(Live2LOVE "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/android/${CMAKE_ANDROID_ARCH_ABI}/liblive2d.a")
		target_link_libraries(Live2LOVE ${GLES2_LIBRARY})
		message(STATUS "Found liblive2d.a at lib/live2d/android/${CMAKE_ANDROID_ARCH_ABI}")
	else()
		message(FATAL_ERROR "Missing Live2D lib files for Android ${CMAKE_ANDROID_ARCH_ABI} or not supported")
		return()
	endif()
else()
	message(FATAL_ERROR "Platform ${CMAKE_SYSTEM_NAME} not supported. Blame Live2D :P")
	return()
endif()

target_include_directories(Live2LOVE PRIVATE include include/live2d framework)
install(TARGETS Live2LOVE DESTINATION lib)
