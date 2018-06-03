/**
 * Copyright (c) 2039 Dark Energy Processor Corporation
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#ifndef _L2L_LIVE2LOVE_
#define _L2L_LIVE2LOVE

// STL
#include <exception>
#include <map>
#include <string>
#include <vector>

// Lua
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

// Live2D Library
#include "Live2D.h"
#include "draw/DDTexture.h"
#include "motion/Live2DMotion.h"
#include "type/LDPointF.h"
#if defined(WIN32)
#	include "Live2DModelWinGL.h"
#	define L2L_WIN
	typedef live2d::Live2DModelWinGL Live2DModel;
#elif defined(__ANDROID__)
#	include "Live2DModelAndroidES2.h"
#	define L2L_ANDROID
	typedef live2d::Live2DModelAndroidES2 Live2DModel;
#elif defined(__APPLE__)
#	include "TargetConditionals.h"
#	if defined(TARGET_IPHONE_SIMULATOR) || defined(TARGET_OS_IPHONE)
#		include "Live2DModelIPhoneES2.h"
#		define L2L_IOS
		typedef live2d::Live2DModelIPhoneES2 Live2DModel;
#	endif
#else
#	error "Platform is not supported by Live2D"
#endif

// Live2D Framework
#include "L2DPhysics.h"
#include "L2DPose.h"
#include "L2DModelMatrix.h"
#include "L2DMotionManager.h"
#include "L2DEyeBlink.h"
#include "L2DMotionManager.h"
#include "L2DExpressionMotion.h"
#include "L2DTargetPoint.h"
#include "L2DTextureDesc.h"

namespace live2love
{
	typedef std::runtime_error namedException;

	// Default LOVE mesh format
	struct Live2LOVEMeshFormat
	{
		float x, y, u, v;
		unsigned char r, g, b, a;
	};

	// Live2LOVE mesh object
	struct Live2LOVEMesh
	{
		// Associated DDTexture
		live2d::DDTexture *drawData;
		// Draw data index
		int drawDataIndex;
		// Model context
		live2d::ModelContext *modelContext;
		// IDrawData context
		live2d::IDrawContext *drawContext;
		// Mesh object reference and mesh table reference
		int meshRefID, tableRefID;
		Live2LOVEMeshFormat *tablePointer;
		// Clip ID mesh
		Live2LOVEMesh *clipID;
	};

	// Live2LOVE model object
	struct Live2LOVE
	{
		// This is pretty much self-explanatory
		Live2DModel *model;
		live2d::framework::L2DMotionManager* motion;
		live2d::framework::L2DMotionManager* expression;
		live2d::framework::L2DEyeBlink* eyeBlink;
		live2d::framework::L2DPhysics* physics;
		// Mesh data list
		std::vector<Live2LOVEMesh*> meshData;
		// Mesh data map (use sparingly)
		std::map<std::string, Live2LOVEMesh*> meshDataMap;
		// List of motions (movement)
		std::map<std::string, live2d::AMotion*> motionList;
		// List of expressions
		std::map<std::string, live2d::AMotion*> expressionList;
		// Lua state
		lua_State *L;
		// Elapsed time. Modulated by 31536000.0 (1 year)
		double elapsedTime;
		// Enable movement animation
		bool movementAnimation;
		// Loop motion name
		std::string motionLoop;

		// Create new Live2LOVE object. Only load moc file
		Live2LOVE(lua_State *L, const std::string& path);
		~Live2LOVE();
		// Update model. deltaT should be in seconds.
		void update(double deltaT);
		// Draw model using LOVE renderer
		void draw(
			double x = 0, double y = 0, double r = 0,
			double sx = 0, double sy = 0,
			double ox = 0, double oy = 0,
			double kx = 0, double ky = 0
		);
		// Set texture to user-supplied LOVE Texture
		void setTexture(int live2dtexno, int loveimageidx);
		// Disable/enable animation movement (eyeblink, stationary movement thing)
		void setAnimationMovement(bool anim);
		// Set parameter value
		void setParamValue(const std::string& name, double value, double weight = 1);
		// Add parameter value
		void addParamValue(const std::string& name, double value, double weight = 1);
		// Multiply parameter value
		void mulParamValue(const std::string& name, double value, double weight = 1);
		// Get parameter value. This value is updated after the model is updated.
		double getParamValue(const std::string& name);
		// Get animation movement status
		bool getAnimationMovement() const;
		// Get list of expression names
		std::vector<const std::string*> getExpressionList();
		// Get list of motion names
		std::vector<const std::string*> getMotionList();
		// Get model canvas dimensions
		std::pair<float, float> getDimensions();
		// Set motion. mode 0 = Just play. mode 1 = Loop. mode 2 = Preserve (no loop)
		void setMotion(const std::string& name, int mode = 0);
		// Set expression
		void setExpression(const std::string& name);
		// Load motion file
		void loadMotion(const std::string& name, const std::pair<double, double>& fade, const std::string& path);
		// Load physics
		void loadPhysics(const std::string& path);
		// Load expression
		void loadExpression(const std::string& name, const std::string& path);

	private:
		// Mesh data initialization
		void setupMeshData();
		// Expression initialize
		void initializeExpression();
		// Motion initializaiton
		void initializeMotion();
		// Stencil drawing
		static int drawStencil(lua_State *L);
	};
}

#endif
