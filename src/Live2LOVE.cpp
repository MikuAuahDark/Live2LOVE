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

// STL
#include <algorithm>
#include <functional>
#include <exception>
#include <map>
#include <string>
#include <vector>

// Lua
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// Live2LOVE
#include "Live2LOVE.h"

// RefData
#include "RefData.h"

// Sort operator, for std::sort
static bool compareDrawOrder(const live2love::Live2LOVEMesh *a, const live2love::Live2LOVEMesh *b)
{
	return a->drawData->getDrawOrder(*a->modelContext, a->drawContext) < b->drawData->getDrawOrder(*b->modelContext, b->drawContext);
}

// Push the FileData into stack. fileSize must not be NULL
static const void *loadFileData(lua_State *L, const std::string& path, size_t *fileSize)
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

live2love::Live2LOVE::Live2LOVE(lua_State *L, const std::string& path)
: L(L)
{
	size_t modelSize;
	const void *modelData = loadFileData(L, path, &modelSize);
	// Init model
	model = Live2DModel::loadModel(modelData, modelSize);
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
		mesh->drawContext = modelContext->getDrawContext(i);
		mesh->modelContext = modelContext;
		mesh->drawDataIndex = i;
		// Create mesh table list
		int numPoints;
		int polygonCount;
		float opacity = ddtex->getOpacity(*modelContext, dctx);
		l2d_index *vertexMap = ddtex->getIndexArray(&polygonCount);
		l2d_pointf *uvmap = ddtex->getUvMap();
		l2d_pointf *points = model->getTransformedPoints(i, &numPoints);
		int verticesCount = polygonCount * 3;
		
		// Build mesh table
		lua_pushvalue(L, -1); // love.graphics.newMesh
		lua_createtable(L, verticesCount, 0); // Mesh table list
		for (int j = 0; j < verticesCount; j++)
		{
			l2d_index k = vertexMap[j];
			// Mesh table format: {x, y, u, v, r, g, b, a}
			// r, g, b will be 1
			lua_createtable(L, 8, 0);
			lua_pushnumber(L, opacity); // a
			lua_pushnumber(L, 1); lua_pushnumber(L, 1); lua_pushnumber(L, 1); // r,g,b
			lua_pushnumber(L, uvmap[k * 2 + 1]); // v
			lua_pushnumber(L, uvmap[k * 2 + 0]); // u
			lua_pushnumber(L, points[k * 2 + 1]); // y
			lua_pushnumber(L, points[k * 2 + 0]); // x
			lua_rawseti(L, -9, 1); // x
			lua_rawseti(L, -8, 2); // y
			lua_rawseti(L, -7, 3); // u
			lua_rawseti(L, -6, 4); // v
			lua_rawseti(L, -5, 5); // r
			lua_rawseti(L, -4, 6); // g
			lua_rawseti(L, -3, 7); // b
			lua_rawseti(L, -2, 8); // a
			lua_rawseti(L, -2, j + 1); // Set table
		}
		mesh->tableRefID = RefData::setRef(L, -1); // Add table reference
		lua_pushstring(L, "triangles"); // Mesh draw mode
		lua_pushstring(L, "stream"); // Mesh usage
		lua_call(L, 3, 1); // love.graphics.newMesh
		mesh->meshRefID = RefData::setRef(L, -1); // Add mesh reference

		// Pop the Mesh object
		lua_pop(L, 1);
		// Push to vector
		meshData.push_back(mesh);
	}
}

void live2love::Live2LOVE::update(double )
{
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
		// Get table mesh ref
		RefData::getRef(L, mesh->tableRefID);
		int polygonCount, meshLen;
		int opacity = mesh->drawData->getOpacity(*mesh->modelContext, mesh->drawContext);
		l2d_index *vertexMap = mesh->drawData->getIndexArray(&polygonCount);
		l2d_pointf *points = model->getTransformedPoints(mesh->drawDataIndex, &meshLen);
		meshLen = polygonCount * 3;

		// Update
		for (int j = 0; j < meshLen; j++)
		{
			l2d_index i = vertexMap[j];
			lua_rawgeti(L, -1, j + 1);
			lua_pushnumber(L, opacity); // a
			lua_pushnumber(L, points[i * 2 + 1]); // y
			lua_pushnumber(L, points[i * 2 + 0]); // x
			lua_rawseti(L, -4, 1); // x
			lua_rawseti(L, -3, 2); // y
			lua_rawseti(L, -2, 8); // a
			lua_pop(L, 1); // Remove modified table
		}

		// Call setVertices
		lua_call(L, 2, 0);
	}
	// Update draw order
	std::sort(meshData.begin(), meshData.end(), compareDrawOrder);
}

void live2love::Live2LOVE::draw(double x, double y, double r, double sx, double sy, double ox, double oy, double kx, double ky)
{
	lua_checkstack(L, lua_gettop(L) + 15);
	// Get love.graphics.draw
	RefData::getRef(L, "love.graphics.draw");
	// List mesh data
	for (auto mesh: meshData)
	{
		lua_pushvalue(L, -1);
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
	}
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

void live2love::Live2LOVE::loadMotion(const std::string& name, const std::string& path)
{
	// Load file
	size_t motionSize;
	const void *motionData = loadFileData(L, path, &motionSize);
	live2d::AMotion *motion = live2d::Live2DMotion::loadMotion(motionData, motionSize);
	// Remove FileData
	lua_pop(L, 1);
	// Set motion
	motionList[name] = motion;
}

void live2love::Live2LOVE::loadPhysics(const std::string& path)
{
	// Load file
	size_t physicsSize;
	const void *physicsData = loadFileData(L, path, &physicsSize);
	physics = live2d::framework::L2DPhysics::load(physicsData, physicsSize);
}