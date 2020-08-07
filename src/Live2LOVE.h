/**
 * Copyright (c) 2040 Dark Energy Processor Corporation
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
#define _L2L_LIVE2LOVE_

// STL
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Lua
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

// Live2D
#include "Live2DCubismCore.hpp"

// Live2D Framework
#include "Effect/CubismBreath.hpp"
#include "Effect/CubismEyeBlink.hpp"
#include "Effect/CubismPose.hpp"
#include "Model/CubismMoc.hpp"
#include "Model/CubismModel.hpp"
#include "Motion/CubismExpressionMotion.hpp"
#include "Motion/CubismMotion.hpp"
#include "Motion/CubismMotionManager.hpp"
#include "Physics/CubismPhysics.hpp"

namespace live2love
{
	using namespace Live2D::Cubism::Core;
	using namespace Live2D::Cubism::Framework;
	typedef std::runtime_error namedException;
	constexpr double PI = 3.14159265358979323846264338327950288;

	enum MotionModeID {
		MOTION_NORMAL,
		MOTION_LOOP,
		MOTION_PRESERVE,
		MOTION_MAX_ENUM
	};

	// Default LOVE mesh format
	struct Live2LOVEMeshFormat
	{
		float x, y, u, v;
		unsigned char r, g, b, a;
	};

	// Live2LOVE mesh object
	struct Live2LOVEMesh
	{
		// Draw data index
		int index;
		// Texture index
		int textureIndex;
		// Amount of vertices
		int numPoints;
		// Render order
		int renderOrder;
		// Blending mode
		Rendering::CubismRenderer::CubismBlendMode blending;
		// Model object
		CubismModel *model;
		// Mesh object reference and mesh table reference
		int meshRefID, tableRefID;
		Live2LOVEMeshFormat *tablePointer;
		// Clip ID mesh
		std::vector<Live2LOVEMesh*> clipID;
	};

	struct Live2LOVEParamDef
	{
		std::string name;
		double min, max, def;
	};

	struct Live2LOVEBreath
	{
		std::string paramName;
		double offset, peak, cycle, weight;
	};

	// Live2LOVE model object
	struct Live2LOVE
	{
		// Draw coordinates for love.graphics.draw
		struct DrawCoordinates 
		{
			double x, y, r, sx, sy, ox, oy, kx, ky;
		};

		// This is pretty much self-explanatory
		//uint8_t *mocFreeThis;
		CubismMoc *moc;
		CubismModel *model;
		CubismMotionManager *motion;
		CubismMotionManager* expression;
		CubismEyeBlink *eyeBlink;
		CubismPhysics *physics;
		CubismBreath *breath;
		CubismPose *pose;
		// Mesh data list
		std::vector<Live2LOVEMesh*> meshData;
		// Mesh data map (use sparingly)
		std::map<std::string, Live2LOVEMesh*> meshDataMap;
		// List of motions (movement)
		std::map<std::string, CubismMotion*> motionList;
		// List of expressions
		std::map<std::string, CubismExpressionMotion*> expressionList;
		// Lua state
		lua_State *L;
		// Enable movement animation
		bool movementAnimation;
		// Enable eye blink
		bool eyeBlinkMovement;
		// Loop motion name
		std::string motionLoop;
		// Parameter update list
		std::map<std::string, std::pair<double, double>*> postParamUpdateList;
		// Model width and height
		float modelWidth, modelHeight;
		// Model offset
		float modelOffX, modelOffY;
		// Model pixel units
		float modelPixelUnits;

		// Create new Live2LOVE object. Only load moc file
		Live2LOVE(lua_State *L, const void *buf, size_t size);
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
		// Disable/enable animation movement (physics & dynamic move over time)
		void setAnimationMovement(bool anim);
		// Disable/enable eye blinking
		void setEyeBlinkMovement(bool anim);
		// Set parameter value
		void setParamValue(const std::string& name, double value, double weight = 1);
		// Set parameter value after model update
		void setParamValuePost(const std::string& name, double value, double weight = 1);
		// Add parameter value
		void addParamValue(const std::string& name, double value, double weight = 1);
		// Multiply parameter value
		void mulParamValue(const std::string& name, double value, double weight = 1);
		// Get parameter value. This value is updated after the model is updated.
		double getParamValue(const std::string& name) const;
		// Get parameter information list
		std::vector<Live2LOVEParamDef> getParamInfoList();
		// Get animation movement status
		bool isAnimationMovementEnabled() const;
		// Get eye blink status
		bool isEyeBlinkEnabled() const;
		// Get list of expression names
		std::vector<const std::string*> getExpressionList() const;
		// Get list of motion names
		std::vector<const std::string*> getMotionList() const;
		// Get model canvas dimensions
		std::pair<float, float> getDimensions() const;
		// Set motion. mode 0 = Just play. mode 1 = Loop. mode 2 = Preserve (no loop)
		void setMotion(const std::string& name, MotionModeID mode = MOTION_NORMAL);
		// Clear motion.
		void setMotion();
		// Set expression
		void setExpression(const std::string& name);
		// Load motion file
		void loadMotion(const std::string& name, const std::pair<double, double>& fade, const void *buf, size_t size);
		// Load physics
		void loadPhysics(const void *buf, size_t size);
		// Load expression
		void loadExpression(const std::string& name, const void *buf, size_t size);
		// Load pose from JSON
		void loadPose(const void *buf, size_t size);
		// Load eye blink based on specific parameter names
		void loadEyeBlink(const std::vector<std::string> &names);
		// Load default eye blink
		void loadEyeBlink();
		// Load breath based on specific parameter names
		void loadBreath(const std::vector<Live2LOVEBreath> &params);
		// Load default breath
		void loadBreath();
		// Get model offset
		std::pair<float, float> getModelCenterPosition();

	private:
		// Mesh data initialization
		void setupMeshData();
		// Expression initialize
		void initializeExpression();
		// Motion initializaiton
		void initializeMotion();
		// Stencil drawing main loop
		void drawStencil(Live2LOVEMesh *mesh, DrawCoordinates &drawPosition, int depth);
		// Stencil drawing Lua function
		static int drawStencil(lua_State *L);
	};
}

#endif
