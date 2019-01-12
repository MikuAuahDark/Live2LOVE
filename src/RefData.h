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

#ifndef _L2L_REFDATA_
#define _L2L_REFDATA_

// Lua
#include <lua.h>

// Lua reference of functions
namespace RefData
{
	// Push new object
	void getRef(lua_State *L, int refID);
	// Push new object or nil
	void getRef(lua_State *L, const std::string& refName);
	// Returns new reference ID relative to Live2LOVE main table
	// Does not affect stack number
	int setRef(lua_State *L, int luaval);
	// Same as setRef, but store it in map
	int setRef(lua_State *L, const std::string& refName, int luaval);
	// Removes reference
	void delRef(lua_State *L, int refID);
	// Removes reference, or do nothing
	void delRef(lua_State *L, const std::string& refName);
};

#endif
