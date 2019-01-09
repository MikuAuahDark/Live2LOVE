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

// std
#include <cmath>

// STL
#include <algorithm>
#include <functional>
#include <exception>
#include <map>
#include <new>
#include <string>
#include <vector>

// Lua
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// Live2LOVE
#include "Live2LOVE.h"

// Live2D Library (additional)
#include "base/IBaseContext.h"

// RefData
#include "RefData.h"

// Sort operator, for std::sort
static bool compareDrawOrder(const live2love::Live2LOVEMesh *a, const live2love::Live2LOVEMesh *b)
{
	int da = a->drawData->getDrawOrder(*a->modelContext, a->drawContext);
	int db = b->drawData->getDrawOrder(*b->modelContext, b->drawContext);
	//return da == db ? a->drawDataIndex < b->drawDataIndex : da < db;
	if (da == db)
		if (a->partsIndex == b->partsIndex)
			return a->drawDataIndex < b->drawDataIndex;
		else
			return a->partsIndex < b->partsIndex;
	else
		return da < db;
}

// Push the FileData into stack. fileSize must not be NULL
const void *loadFileData(lua_State *L, const std::string& path, size_t *fileSize)
{
	// New file data
	RefData::getRef(L, "love.filesystem.newFileData");
	lua_pushlstring(L, path.c_str(), path.length());
	lua_call(L, 1, 2);
	if (lua_isnil(L, -2))
	{
		// Cannot open file. Error message is in -1
		// Create exception object
		live2love::namedException temp = live2love::namedException(lua_tostring(L, -1));
		// Then pop both values (nil and error message)
		lua_pop(L, 2);
		// Then throw the exception
		throw temp;
	}

	// We now have FileData at -2 index, so pop 1 value
	lua_pop(L, 1);
	// Now FileData in -1
	const void *ptr;
	// Push all things necessary
	lua_getfield(L, -1, "getSize");
	lua_pushvalue(L, -2);
	lua_getfield(L, -3, "getPointer");
	lua_pushvalue(L, -2);
	// Call getPointer
	lua_call(L, 1, 1);
	ptr = lua_topointer(L, -1);
	lua_pop(L, 1);
	// Call getSize
	lua_call(L, 1, 1);
	*fileSize = lua_tointeger(L, -1);
	lua_pop(L, 1); // Pop the getSize result

	return ptr;
}

// Push the ByteData into stack.
template<class T> T* createData(lua_State *L, size_t amount = 1)
{
	lua_checkstack(L, lua_gettop(L) + 8);
	size_t memalloc = sizeof(T) * amount;
	// New file data
	RefData::getRef(L, "love.data.newByteData");
	lua_pushinteger(L, memalloc);
	lua_call(L, 1, 1);
	lua_getfield(L, -1, "getPointer");
	lua_pushvalue(L, -2);
	lua_call(L, 1, 1);
	T* val = (T*)lua_topointer(L, -1);
	lua_pop(L, 1); // pop the pointer

	// Leave the ByteData in stack
	return val;
}

live2love::Live2LOVE::Live2LOVE(lua_State *L, const std::string& path)
: L(L)
, motionLoop("")
, elapsedTime(0.0)
, movementAnimation(true)
, eyeBlinkMovement(true)
, motion(nullptr)
, expression(nullptr)
, physics(nullptr)
, eyeBlink(nullptr)
{
	size_t modelSize;
	const void *modelData = loadFileData(L, path, &modelSize);
	// Init model
	model = Live2DModel::loadModel(modelData, modelSize);
	if (model == nullptr) throw namedException("Failed to load model");

	eyeBlink = new live2d::framework::L2DEyeBlink();
	model->setPremultipliedAlpha(false);
	model->update();
	// Pop the FileData
	lua_pop(L, 1);
	// Setup mesh data
	setupMeshData();
}
live2love::Live2LOVE::~Live2LOVE()
{
	// Delete all mesh
	for (auto mesh: meshData)
	{
		RefData::delRef(L, mesh->tableRefID);
		RefData::delRef(L, mesh->meshRefID);
		delete mesh;
	}

	// Delete all motions
	for (auto& x: motionList)
		// nullptr delete is okay
		delete x.second;
	// Delete all expressions
	for (auto& x: expressionList)
		delete x.second;

	delete eyeBlink;
	// Delete physics
	delete physics;
	// Delete model
	delete model;
}

void live2love::Live2LOVE::setupMeshData()
{
	// Check stack
	lua_checkstack(L, 64);
	// Get model context
	live2d::ModelContext *modelContext = model->getModelContext();
	// Get drawable count
	int drawableCount = modelContext->getDrawDataCount();
	meshData.reserve(drawableCount);
	// Push newMesh
	RefData::getRef(L, "love.graphics.newMesh");
	
	// Load mesh
	for (int i = 0; i < drawableCount; i++)
	{
		// Get DrawDataTexture object
		live2d::DDTexture *ddtex = dynamic_cast<live2d::DDTexture*>(modelContext->getDrawData(i));
		// Not HonokaMiku's DecrypterContext!
		live2d::IDrawContext * dctx = modelContext->getDrawContext(i);
		if (ddtex == nullptr) continue; // not DDTexture. Skip.
		
		// Create new mesh object
		Live2LOVEMesh *mesh = new Live2LOVEMesh;
		mesh->drawData = ddtex;
		mesh->drawContext = dctx;
		mesh->modelContext = modelContext;
		mesh->drawDataIndex = i;
		mesh->partsIndex = dctx->getPartsIndex();

		// Create mesh table list
		int numPoints;
		int polygonCount;
		float opacity = ddtex->getOpacity(*modelContext, dctx);
		l2d_index *vertexMap = ddtex->getIndexArray(&polygonCount);
		l2d_pointf *uvmap = ddtex->getUvMap();
		l2d_pointf *points = model->getTransformedPoints(i, &numPoints);
		int verticesCount = polygonCount * 3;
		
		// Build mesh
		lua_pushvalue(L, -1); // love.graphics.newMesh
		lua_pushinteger(L, verticesCount);
		lua_pushstring(L, "triangles"); // Mesh draw mode
		lua_pushstring(L, "stream"); // Mesh usage
		lua_call(L, 3, 1); // love.graphics.newMesh
		mesh->meshRefID = RefData::setRef(L, -1); // Add mesh reference
		// Pop the Mesh object
		lua_pop(L, 1);
		
		Live2LOVEMeshFormat *meshDataRaw = createData<Live2LOVEMeshFormat>(L, verticesCount);
		for (int j = 0; j < verticesCount; j++)
		{
			Live2LOVEMeshFormat& m = meshDataRaw[j];
			l2d_index k = vertexMap[j];
			// Mesh table format: {x, y, u, v, r, g, b, a}
			// r, g, b will be 1
			m.x = points[k * 2 + 0];
			m.y = points[k * 2 + 1];
			m.u = uvmap[k * 2 + 0];
			m.v = uvmap[k * 2 + 1];
			m.r = m.g = m.b = 255;
			m.a = floor(opacity * 255.0);
		}
		mesh->tableRefID = RefData::setRef(L, -1); // Add FileData reference
		mesh->tablePointer = meshDataRaw;
		lua_pop(L, 1); // pop the FileData reference

		// Push to vector
		meshData.push_back(mesh);
		meshDataMap[ddtex->getDrawDataID()->toChar()] = mesh;
	}

	// Find clip ID list
	for (int i = 0; i < drawableCount; i++)
	{
		Live2LOVEMesh *mesh = meshData[i];
		auto cliplist = mesh->drawData->getClipIDList();

		if (cliplist)
		{
			for (int k = 0; k < cliplist->size(); k++)
			{
				const char *id = cliplist->operator[](k)->toChar();
				if (id) mesh->clipID.push_back(meshDataMap[id]);
			}
		}
	}
}

void live2love::Live2LOVE::update(double dT)
{
	elapsedTime = fmod((elapsedTime + dT), 31536000.0);
	double t = elapsedTime * 2 * M_PI;
	// Motion update
	if (motion)
	{
		model->loadParam();
		if (motion->isFinished() && motionLoop.length() > 0)
			// Revert
			motion->startMotion(motionList[motionLoop], false);

		if (!motion->updateParam(model) && movementAnimation && eyeBlink && eyeBlinkMovement)
			// Update eye blink
			eyeBlink->setParam(model);

		model->saveParam();
	}
	// Expression update
	if (expression) expression->updateParam(model);
	// Movement update
	if (movementAnimation)
	{
		model->addToParamFloat("PARAM_ANGLE_X", 15.0 * sin(t / 6.5345), 0.5);
		model->addToParamFloat("PARAM_ANGLE_Y", 8.0 * sin(t / 3.5345), 0.5);
		model->addToParamFloat("PARAM_ANGLE_Z", 10.0 * sin(t / 5.5345), 0.5);
		model->addToParamFloat("PARAM_BODY_ANGLE_X", 4.0 * sin(t / 15.5345), 0.5);
		model->setParamFloat("PARAM_BREATH", 0.5 + 0.5 * sin(t / 3.2345), 1.0); // Override user-set value
		// Physics update
		if (physics) physics->updateParam(model);
	}
	// If there's parameter change, set it
	for (auto& list: paramUpdateList)
		((*model).*list.second.first)(list.first.c_str(), list.second.second.first, list.second.second.second);
	paramUpdateList.clear();
	// Update model
	model->update();

	// Update mesh data
	for (auto mesh: meshData)
	{
		// Get mesh ref
		RefData::getRef(L, mesh->meshRefID);
		// Get "setVertices"
		lua_getfield(L, -1, "setVertices");
		lua_pushvalue(L, -2);
		// Get data mesh ref
		RefData::getRef(L, mesh->tableRefID);
		int polygonCount, meshLen;
		double opacity =
			double(mesh->drawContext->interpolatedOpacity) *
			double(mesh->drawContext->baseOpacity);
		l2d_index *vertexMap = mesh->drawData->getIndexArray(&polygonCount);
		l2d_pointf *points = model->getTransformedPoints(mesh->drawDataIndex, &meshLen);
		meshLen = polygonCount * 3;

		// Update
		for (int j = 0; j < meshLen; j++)
		{
			Live2LOVEMeshFormat& m = mesh->tablePointer[j];
			l2d_index i = vertexMap[j];
			m.x = points[i * 2 + 0];
			m.y = points[i * 2 + 1];
			m.a = floor(opacity * 255.0);
		}

		// Call setVertices
		lua_call(L, 2, 0);
	}
	// Update draw order
	std::sort(meshData.begin(), meshData.end(), compareDrawOrder);
}

void live2love::Live2LOVE::draw(double x, double y, double r, double sx, double sy, double ox, double oy, double kx, double ky)
{
	if (!lua_checkstack(L, lua_gettop(L) + 24))
		throw namedException("Internal error: cannot grow Lua stack size");

	// Save blending
	RefData::getRef(L, "love.graphics.setBlendMode");
	RefData::getRef(L, "love.graphics.getBlendMode");
	lua_call(L, 0, 2);
	// Set blending mode to alpha,alphamultiply
	lua_pushvalue(L, -3);
	lua_pushstring(L, "alpha");
	lua_pushstring(L, "alphamultiply");
	lua_call(L, 2, 0);
	int blendMode = live2d::DDTexture::COLOR_COMPOSITION_NORMAL; // alpha,alphamultiply
	// Get love.graphics.draw
	RefData::getRef(L, "love.graphics.draw");
	// List mesh data
	for (auto mesh: meshData)
	{
		bool stencilSet = false;
		// If there's clip ID, draw stencil first.
		if (mesh->clipID.size() > 0)
		{
			// Get stencil function
			RefData::getRef(L, "love.graphics.setStencilTest");
			RefData::getRef(L, "love.graphics.stencil");
			// Push upvalues
			lua_pushvalue(L, -3); // love.graphics.draw
			lua_pushlightuserdata(L, mesh);
			lua_pushnumber(L, x);
			lua_pushnumber(L, y);
			lua_pushnumber(L, r);
			lua_pushnumber(L, sx);
			lua_pushnumber(L, sy);
			lua_pushnumber(L, ox);
			lua_pushnumber(L, oy);
			lua_pushnumber(L, kx);
			lua_pushnumber(L, ky);
			lua_pushcclosure(L, Live2LOVE::drawStencil, 11);
			lua_call(L, 1, 0); // love.graphics.stencil
			lua_pushvalue(L, -1);
			lua_pushstring(L, "greater");
			lua_pushnumber(L, 0);
			lua_call(L, 2, 0); // love.graphics.setStencilTest
			stencilSet = true;
		}
		int meshBlendMode = (mesh->drawData->getOptionFlag() >> 1) & 3;
		if (meshBlendMode != blendMode)
		{
			// Push love.graphics.setBlendMode
			lua_pushvalue(L, stencilSet ? -5 : -4);
			switch (blendMode = meshBlendMode)
			{
				default:
				case live2d::DDTexture::COLOR_COMPOSITION_NORMAL:
				{
					// Normal blending (alpha,alphamultiply)
					lua_pushstring(L, "alpha");
					lua_pushstring(L, "alphamultiply");
					break;
				}
				case live2d::DDTexture::COLOR_COMPOSITION_SCREEN:
				{
					// Screen blending (add,alphamultiply)
					// but why it should be "add" tho...
					lua_pushstring(L, "add");
					lua_pushstring(L, "alphamultiply");
					break;
				}
				case live2d::DDTexture::COLOR_COMPOSITION_MULTIPLY:
				{
					// Multiply bnending (multiply,premultiplied)
					lua_pushstring(L, "multiply");
					lua_pushstring(L, "premultiplied");
					break;
				}
			}
			// Call it
			lua_call(L, 2, 0);
		}
		lua_pushvalue(L, stencilSet ? -2 : -1);
		RefData::getRef(L, mesh->meshRefID);
		lua_pushnumber(L, x);
		lua_pushnumber(L, y);
		lua_pushnumber(L, r);
		lua_pushnumber(L, sx);
		lua_pushnumber(L, sy);
		lua_pushnumber(L, ox);
		lua_pushnumber(L, oy);
		lua_pushnumber(L, kx);
		lua_pushnumber(L, ky);
		// Draw
		lua_call(L, 10, 0);

		// If there's stencil, disable it.
		if (stencilSet)
			lua_call(L, 0, 0);
	}

	// Remove love.graphics.draw
	lua_pop(L, 1);
	// Reset blend mode
	lua_call(L, 2, 0);
}

void live2love::Live2LOVE::setTexture(int live2dtexno, int loveimageidx)
{
	// List mesh
	for (auto mesh: meshData)
	{
		if (mesh->drawData->getTextureNo() == live2dtexno - 1)
		{
			// Get mesh ref
			RefData::getRef(L, mesh->meshRefID);
			lua_getfield(L, -1, "setTexture");
			lua_pushvalue(L, -2);
			lua_pushvalue(L, loveimageidx);
			// Call it
			lua_call(L, 2, 0);
		}
	}
}

void live2love::Live2LOVE::setAnimationMovement(bool a)
{
	movementAnimation = a;
}

void live2love::Live2LOVE::setEyeBlinkMovement(bool a)
{
	eyeBlinkMovement = a;
}

bool live2love::Live2LOVE::isAnimationMovementEnabled() const
{
	return movementAnimation;
}

bool live2love::Live2LOVE::isEyeBlinkEnabled() const
{
	return eyeBlinkMovement;
}

void live2love::Live2LOVE::setParamValue(const std::string& name, double value, double weight)
{
	model->setParamFloat(name.c_str(), value, weight);
}

void live2love::Live2LOVE::setParamValuePost(const std::string& name, double value, double weight)
{
	paramUpdateList[name] = std::pair<setParamF, std::pair<double, double>>(
		&Live2DModel::setParamFloat,
		std::pair<double, double>(value, weight)
	);
}

void live2love::Live2LOVE::addParamValue(const std::string& name, double value, double weight)
{
	model->addToParamFloat(name.c_str(), value, weight);
}

void live2love::Live2LOVE::mulParamValue(const std::string& name, double value, double weight)
{
	model->multParamFloat(name.c_str(), value, weight);
}

double live2love::Live2LOVE::getParamValue(const std::string& name)
{
	return model->getParamFloat(name.c_str());
}

live2d::LDVector<live2d::ParamDefFloat*> *live2love::Live2LOVE::getParamInfoList()
{
	return model->getModelImpl()->getParamDefSet()->getParamDefFloatList();
}

std::vector<const std::string*> live2love::Live2LOVE::getExpressionList()
{
	std::vector<const std::string*> value = {};

	for (auto& x: expressionList)
		value.push_back(&x.first);

	return value;
}

std::vector<const std::string*> live2love::Live2LOVE::getMotionList()
{
	std::vector<const std::string*> value = {};

	for (auto& x: motionList)
		value.push_back(&x.first);

	return value;
}

std::pair<float, float> live2love::Live2LOVE::getDimensions()
{
	return std::pair<float, float>(model->getCanvasWidth(), model->getCanvasHeight());
}

void live2love::Live2LOVE::setMotion(const std::string& name, int mode)
{
	// No motion? well load one first before using this.
	if (!motion) throw namedException("No motion loaded!");
	if (motionList.find(name) == motionList.end()) throw namedException("Motion not found");
	motion->startMotion(motionList[name], false);

	// Check motion mode
	if (mode == 1) motionLoop = name;
	else if (mode == 2) motionLoop = "";
}

void live2love::Live2LOVE::setMotion()
{
	// clear motion
	if (!motion) throw namedException("No motion loaded!");
	motionLoop = "";
	motion->stopAllMotions();
}

void live2love::Live2LOVE::setExpression(const std::string& name)
{
	// No expression? Load one first!
	if (!expression) throw namedException("No expression loaded!");
	if (expressionList.find(name) == expressionList.end()) throw namedException("Expression not found");
	expression->startMotion(expressionList[name], false);
}

void live2love::Live2LOVE::loadMotion(const std::string& name, const std::pair<double, double>& fade, const std::string& path)
{
	initializeMotion();
	// Load file
	size_t motionSize;
	const void *motionData = loadFileData(L, path, &motionSize);
	live2d::AMotion *motion = live2d::Live2DMotion::loadMotion(motionData, motionSize);
	if (motion == nullptr) throw namedException("Failed to load motion");

	motion->setFadeIn(fade.first);
	motion->setFadeOut(fade.second);
	// Remove FileData
	lua_pop(L, 1);
	// Set motion
	if (motionList.find(name) != motionList.end()) delete motionList[name];
	motionList[name] = motion;
}

void live2love::Live2LOVE::loadExpression(const std::string& name, const std::string& path)
{
	initializeExpression();
	// Load file
	size_t exprSize;
	const void *exprData = loadFileData(L, path, &exprSize);
	live2d::framework::L2DExpressionMotion *expr = live2d::framework::L2DExpressionMotion::loadJson(exprData, exprSize);
	if (expr == nullptr) throw namedException("Failed to load expression");

	// Set expression
	if (expressionList.find(name) != expressionList.end()) delete expressionList[name];
	expressionList[name] = expr;
}

void live2love::Live2LOVE::loadPhysics(const std::string& path)
{
	// Load file
	size_t physicsSize;
	const void *physicsData = loadFileData(L, path, &physicsSize);
	physics = live2d::framework::L2DPhysics::load(physicsData, physicsSize);
	if (physics == nullptr) throw namedException("Failed to load physics");
}

inline void live2love::Live2LOVE::initializeMotion()
{
	if (!motion) motion = new live2d::framework::L2DMotionManager();
}

inline void live2love::Live2LOVE::initializeExpression()
{
	if (!expression) expression = new live2d::framework::L2DMotionManager();
}

int live2love::Live2LOVE::drawStencil(lua_State *L)
{
	// Upvalues:
	// 1. love.graphics.draw
	// 2. Mesh pointer
	// 3,4,5,6,7,8,9,10,11: draw args
	lua_checkstack(L, lua_gettop(L) + 15);
	Live2LOVEMesh *mesh = (Live2LOVEMesh*)lua_topointer(L, lua_upvalueindex(2));
	//for (int i = 0; i < 11; i++)
	//lua_pushvalue(L, lua_upvalueindex(i + 1));
	//lua_call(L, 10, 0);
	for (auto x: mesh->clipID)
	{
		lua_pushvalue(L, lua_upvalueindex(1)); // love.graphics.draw
		RefData::getRef(L, x->meshRefID); // Mesh
		for (int i = 2; i < 11; i++)
			lua_pushvalue(L, lua_upvalueindex(i + 1)); // the rest
		lua_call(L, 10, 0);
	}

	return 0;
}
