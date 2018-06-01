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

// Lua
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// Live2LOVE
#include "Live2LOVE.h"
using namespace live2love;

// RefData
#include "RefData.h"

#define L2L_TRYWRAP(expr) try { expr } catch(std::exception &x) { luaL_error(L, x.what()); }
int Live2LOVE_setTexture(lua_State *L)
{
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Check live2d tex number
	int texno = luaL_checkinteger(L, 2);
	// If it's userdata, then assume it's LOVE object
	luaL_checktype(L, 3, LUA_TUSERDATA);
	// Call
	L2L_TRYWRAP(l2l->setTexture(texno, 3););

	return 0;
}

int Live2LOVE_setParamValue(lua_State *L)
{
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get param name
	size_t nameLen; const char *name = luaL_checklstring(L, 2, &nameLen);
	// Get value
	double value = luaL_checknumber(L, 3);
	// Get weight
	double weight = luaL_optnumber(L, 4, 1.0);
	// Call
	L2L_TRYWRAP(l2l->setParamValue(std::string(name, nameLen), value, weight););

	return 0;
}

// Copypaste from Live2LOVE_setParamValue
int Live2LOVE_addParamValue(lua_State *L)
{
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get param name
	size_t nameLen; const char *name = luaL_checklstring(L, 2, &nameLen);
	// Get value
	double value = luaL_checknumber(L, 3);
	// Get weight
	double weight = luaL_optnumber(L, 4, 1.0);
	// Call
	L2L_TRYWRAP(l2l->addParamValue(std::string(name, nameLen), value, weight););

	return 0;
}

int Live2LOVE_getParamValue(lua_State *L)
{
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get param name
	size_t nameLen; const char *name = luaL_checklstring(L, 2, &nameLen);
	double value;
	// Call
	L2L_TRYWRAP(value = l2l->getParamValue(std::string(name, nameLen)););

	// Push value
	lua_pushnumber(L, value);
	return 1;
}

static std::map<std::string, int> motionMapMode = {
	{"normal", 0},
	{"loop", 1},
	{"preserve", 2}
};
int Live2LOVE_setMotion(lua_State *L)
{
	size_t motionNameLen;
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get string
	const char *motionName = luaL_checklstring(L, 2, &motionNameLen);
	// Get motion mode
	int mode;
	if (lua_isnoneornil(L, 3)) mode = 0;
	else if (lua_isnumber(L, 3)) mode = lua_tonumber(L, 3);
	else if (lua_isstring(L, 3))
	{
		std::string modeStr = std::string(lua_tostring(L, 3));
		if (motionMapMode.find(modeStr) == motionMapMode.end())
			luaL_argerror(L, 3, "Invalid mode");
		mode = motionMapMode[modeStr];
	}
	else luaL_typerror(L, 3, "string or number");
	if (mode < 0 || mode > 2) luaL_argerror(L, 3, "Invalid mode");
	// Call
	L2L_TRYWRAP(l2l->setMotion(std::string(motionName, motionNameLen), mode););

	return 0;
}

int Live2LOVE_loadMotion(lua_State *L)
{
	size_t motionNameLen, fileLen;
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get name and path
	const char *motionName = luaL_checklstring(L, 2, &motionNameLen);
	double fadeIn = luaL_checknumber(L, 3);
	double fadeOut = luaL_checknumber(L, 4);
	const char *file = luaL_checklstring(L, 5, &fileLen);
	L2L_TRYWRAP(l2l->loadMotion(
		std::string(motionName, motionNameLen),
		std::pair<double, double>(fadeIn, fadeOut),
		std::string(file, fileLen)
	););

	return 0;
}

// Just copypaste from Live2LOVE_setMotion
int Live2LOVE_setExpression(lua_State *L)
{
	size_t motionNameLen;
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get string
	const char *motionName = luaL_checklstring(L, 2, &motionNameLen);
	// Call
	L2L_TRYWRAP(l2l->setExpression(std::string(motionName, motionNameLen)););

	return 0;
}

// Just copypaste from Live2LOVE_loadMotion
int Live2LOVE_loadExpression(lua_State *L)
{
	size_t motionNameLen, fileLen;
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get name and path
	const char *motionName = luaL_checklstring(L, 2, &motionNameLen);
	const char *file = luaL_checklstring(L, 3, &fileLen);
	L2L_TRYWRAP(l2l->loadExpression(std::string(motionName, motionNameLen), std::string(file, fileLen)););

	return 0;
}

int Live2LOVE_loadPhysics(lua_State *L)
{
	size_t physLen;
	// Get udata
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	// Get name and path
	const char *phys = luaL_checklstring(L, 2, &physLen);
	L2L_TRYWRAP(l2l->loadPhysics(std::string(phys, physLen)););

	return 0;
}

int Live2LOVE_update(lua_State *L)
{
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	double dT = luaL_optnumber(L, 2, 0);
	L2L_TRYWRAP(l2l->update(dT););
	return 0;
}

int Live2LOVE_draw(lua_State *L)
{
	Live2LOVE *l2l = *(Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	double x = luaL_optnumber(L, 2, 0);
	double y = luaL_optnumber(L, 3, 0);
	double r = luaL_optnumber(L, 4, 0);
	double sx = luaL_optnumber(L, 5, 1);
	double sy = luaL_optnumber(L, 6, 1);
	double ox = luaL_optnumber(L, 7, 0);
	double oy = luaL_optnumber(L, 8, 0);
	double kx = luaL_optnumber(L, 9, 0);
	double ky = luaL_optnumber(L, 10, 0);
	L2L_TRYWRAP(l2l->draw(x, y, r, sx, sy, ox, oy, kx, ky););
	return 0;
}

int Live2LOVE___gc(lua_State *L)
{
	Live2LOVE **x = (Live2LOVE**)luaL_checkudata(L, 1, "Live2LOVE");
	delete *x;
	return 0;
}

static std::map<std::string, lua_CFunction> Live2LOVE_methods = {
	{"setTexture", Live2LOVE_setTexture},
	{"setParamValue", Live2LOVE_setParamValue},
	{"setMotion", Live2LOVE_setMotion},
	{"setExpression", Live2LOVE_setExpression},
	{"loadMotion", Live2LOVE_loadMotion},
	{"loadExpression", Live2LOVE_loadExpression},
	{"loadPhysics", Live2LOVE_loadPhysics},
	{"getParamValue", Live2LOVE_getParamValue},
	{"update", Live2LOVE_update},
	{"draw", Live2LOVE_draw},
	{"__gc", Live2LOVE___gc}
};

// Load moc file (constructor)
int Live2LOVE_Live2LOVE(lua_State *L)
{
	size_t pathSize;
	Live2LOVE *l2l = nullptr;
	// Get path
	const char *path = luaL_checklstring(L, 1, &pathSize);
	// Call constructor
	L2L_TRYWRAP(l2l = new Live2LOVE(L, path););
	// Create new user data
	Live2LOVE **obj = (Live2LOVE**)lua_newuserdata(L, sizeof(Live2LOVE*));
	*obj = l2l;
	// Set userdata metatable
	luaL_getmetatable(L, "Live2LOVE");
	lua_setmetatable(L, -2);
	// Return Live2LOVE object
	return 1;
}

extern "C" int LUALIB_API luaopen_Live2LOVE(lua_State *L)
{
	// Initialize Live2D
	{unsigned int err; live2d::Live2D::init();
	if ((err = live2d::Live2D::getError()) != live2d::Live2D::L2D_NO_ERROR)
		luaL_error(L, "Live2D initialize error: %u",err);
	}

	// Create new Live2LOVE metatable
	luaL_newmetatable(L, "Live2LOVE");
	// Setup function methods
	lua_pushstring(L, "__index");
	lua_createtable(L, 0, 0);
	// Export methods
	for (auto& x: Live2LOVE_methods)
	{
		lua_pushlstring(L, x.first.c_str(), x.first.length());
		lua_pushcfunction(L, x.second);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3); // For the Live2LOVE metatable. set __index to table
	lua_pop(L, 1); // Remove the metatable from stack for now.

	// Setup needed LOVE functions
	lua_getfield(L, LUA_GLOBALSINDEX, "package");
	lua_getfield(L, -1, "loaded");
	lua_remove(L, -2); // remove package
	lua_getfield(L, -1, "love");
	lua_remove(L, -2); // remove loaded

	// Check if LOVE module is loaded. If it's not then throw error.
	if (lua_isnil(L, -1))
		luaL_error(L, "love module is not loaded!");

	lua_getfield(L, -1, "graphics");
	// Check if love.graphics module is loaded.
	if (lua_isnil(L, -1))
		luaL_error(L, "love.graphics module is not loaded!");

	// Get love.graphics.draw
	lua_getfield(L, -1, "draw");
	RefData::setRef(L, "love.graphics.draw", -1);
	lua_pop(L, 1); // pop the function
	// Get love.graphics.newMesh
	lua_getfield(L, -1, "newMesh");
	RefData::setRef(L, "love.graphics.newMesh", -1);
	lua_pop(L, 2); // pop the function and the graphics table

	// Setup newFileData
	lua_getfield(L, -1, "filesystem"); // assume it's always available
	lua_getfield(L, -1, "newFileData");
	RefData::setRef(L, "love.filesystem.newFileData", -1);
	lua_pop(L, 3); // pop newFileData, filesystem, and love

	// Export table
	lua_createtable(L, 0, 0);
	lua_pushstring(L, "loadMocFile");
	lua_pushcfunction(L, Live2LOVE_Live2LOVE);
	lua_rawset(L, -3);
	lua_pushstring(L, "_VERSION");
	lua_pushstring(L, "0.1.0");
	lua_rawset(L, -3);
	lua_pushstring(L, "Live2DVersion");
	lua_pushstring(L, live2d::Live2D::getVersionStr());
	lua_rawset(L, -3);
	return 1;
}
