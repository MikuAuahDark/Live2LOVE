# Well shit don't blame me, blame Live2D Cubism Native Core uses CMake 3.6
cmake_minimum_required (VERSION 3.6)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "Prevented in-tree buiild!")
endif()

project(Live2LOVE)

# Check Live2D source/header files
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/live2d/Core/include/Live2DCubismCore.h")
	message(FATAL_ERROR "Live2D core header files missing")
endif()

# Add Live2D library
add_subdirectory("live2d/Core")
add_subdirectory("live2d/Framework")

set(LIVE2LOVE_SOURCE_FILES
	# Live2LOVE files
	src/Live2LOVE.cpp
	src/RefData.cpp
	src/Main.cpp
	${LIVE2LOVE_FRAMEWORK_FILES}
)

target_include_directories(Live2LOVE PRIVATE include include/live2d framework)
install(TARGETS Live2LOVE DESTINATION lib)
