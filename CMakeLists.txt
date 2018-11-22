cmake_minimum_required (VERSION 3.1)

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

add_library(Live2LOVE SHARED
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
include_directories(include include/live2d framework)

# Check Live2D lib files
if(WIN32)
	if(MSVC)
		# excuse me wtf
		add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -DLUA_BUILD_AS_DLL -DLUA_LIB)
		# Live2D libraries aren't "DLL"
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		find_package(OpenGL REQUIRED)

		if(CMAKE_SIZEOF_VOID_P GREATER 4)
			set(LIVE2LOVE_WIN_PLATFORM "x64")
			target_link_libraries(Live2LOVE "${CMAKE_CURRENT_SOURCE_DIR}/lib/Win32/lua51_x64.lib")
		else()
			set(LIVE2LOVE_WIN_PLATFORM "x86")
			target_link_libraries(Live2LOVE "${CMAKE_CURRENT_SOURCE_DIR}/lib/Win32/lua51.lib")
		endif()

		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/windows/${LIVE2LOVE_WIN_PLATFORM}/${MSVC_TOOLSET_VERSION}/Release/live2d_opengl_mt.lib")
			add_definitions(-DL2D_TARGET_WIN_GL)
			target_link_libraries(Live2LOVE
				debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/windows/${LIVE2LOVE_WIN_PLATFORM}/${MSVC_TOOLSET_VERSION}/Debug/live2d_opengl_mt.lib"
				optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/windows/${LIVE2LOVE_WIN_PLATFORM}/${MSVC_TOOLSET_VERSION}/Release/live2d_opengl_mt.lib"
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
		find_path(GLES2_INCLUDE_DIR GLES2/gl2.h HINTS ${ANDROID_NDK})
		find_library(GLES2_LIBRARY libGLESv2.so HINTS ${GLES2_INCLUDE_DIR}/../lib)
		add_definitions(-DL2D_TARGET_ANDROID -DL2D_TARGET_ANDROID_ES2)
		target_link_libraries(Live2LOVE "${CMAKE_CURRENT_SOURCE_DIR}/lib/live2d/android/${CMAKE_ANDROID_ARCH_ABI}/liblive2d.a")
		target_link_libraries(Live2LOVE ${GLES2_LIBRARY})
	else()
		message(FATAL_ERROR "Missing Live2D lib files for Android ${CMAKE_ANDROID_ARCH_ABI} or not supported")
		return()
	endif()
else()
	message(FATAL_ERROR "Platform ${CMAKE_SYSTEM_NAME} not supported. Blame Live2D :P")
	return()
endif()

install(TARGETS Live2LOVE DESTINATION lib)
