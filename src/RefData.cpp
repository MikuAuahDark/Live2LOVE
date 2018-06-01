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
#include <map>
#include <string>

// Lua
#include <lua.h>
#include <lauxlib.h>

// RefData
#include "RefData.h"

static std::map<std::string, int> namedRef;

// Helper to convert index
inline int toPositiveIndex(lua_State *L, int idx)
{
	return idx < 0 ? lua_gettop(L) + (idx + 1) : idx;
}

void RefData::getRef(lua_State *L, int refID)
{
	lua_pushstring(L, "Live2LOVE");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawgeti(L, -1, refID);
	lua_remove(L, -2); // remove Live2LOVE table
}

void RefData::getRef(lua_State *L, const std::string& name)
{
	int refID;
	if (namedRef.find(name) == namedRef.end())
		lua_pushnil(L);
	else
		getRef(L, namedRef[name]);
}

int RefData::setRef(lua_State *L, int luaval)
{
	luaval = toPositiveIndex(L, luaval);
	lua_pushstring(L, "Live2LOVE");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, luaval);
	int v = luaL_ref(L, -2);
	lua_pop(L, 1); // pop the table
	return v;
}
int RefData::setRef(lua_State *L, const std::string& name, int luaval)
{
	if (namedRef.find(name) != namedRef.end())
		delRef(L, namedRef[name]);

	return namedRef[name] = setRef(L, luaval);
}

void RefData::delRef(lua_State *L, int refID)
{
	lua_pushstring(L, "Live2LOVE");
	lua_rawget(L, LUA_REGISTRYINDEX);
	luaL_unref(L, -1, refID);
	lua_pop(L, 1);
}

void RefData::delRef(lua_State *L, const std::string& name)
{
	if (namedRef.find(name) != namedRef.end())
	{
		delRef(L, namedRef[name]);
		namedRef.erase(name);
	}
}
