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
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// Live2LOVE
#include "Live2LOVE.h"

// RefData
#include "RefData.h"

// Live2D
#include "Id/CubismIdManager.hpp"
#include "CubismDefaultParameterId.hpp"

inline std::string fromCsmString(const Live2D::Cubism::Framework::csmString &str)
{
	return std::string(str.GetRawString(), (size_t) str.GetLength());
}

inline const Live2D::Cubism::Framework::CubismId *toCsmString(const std::string &str)
{
	Live2D::Cubism::Framework::csmString nstr(str.c_str(), str.length());
	return Live2D::Cubism::Framework::CubismFramework::GetIdManager()->GetId(nstr);
}

// Push "string"-wrapped pointer into the stack
static void encodePointer(lua_State *L, void *ptr)
{
	char temp[sizeof(void*)];
	memcpy(temp, &ptr, sizeof(void*));
	lua_pushlstring(L, temp, sizeof(void*));
}

// Pop "string"-wrapped pointer from the stack i
template<typename P = void*> static P decodePointer(lua_State *L, int i)
{
	size_t size;
	const char *ch = luaL_optlstring(L, i, "", &size);

	if (ch == nullptr || size == 0)
		return nullptr;

	return *((P *) ch);
}

namespace live2love
{

// clipping
static const char stencilFragment[] = R"(
vec4 effect(vec4 color, Image tex, vec2 tc, vec2 sc)
{
	if (Texel(tex, tc).a > 0.1) return vec4(1.0, 1.0, 1.0, 1.0);
	else discard;
}
)";
static int stencilFragRef = LUA_REFNIL;
static int stencilCount = 0;

// Sort operator, for std::sort
static bool compareDrawOrder(const Live2LOVEMesh *a, const Live2LOVEMesh *b)
{
	return a->renderOrder < b->renderOrder;
}

// Push the ByteData into stack.
template<class T> T* createData(lua_State *L, size_t amount)
{
	lua_checkstack(L, lua_gettop(L) + 8);

	size_t memalloc = sizeof(T) * amount;
	T *val = nullptr;

	// New file data
	RefData::getRef(L, "love.data.newByteData");
	lua_pushinteger(L, memalloc);
	lua_call(L, 1, 1);

	// Try Data:getFFIPointer first, it's safe from lightuserdata restrictions
	lua_getfield(L, -1, "getFFIPointer");
	if (lua_isnil(L, -1) == false)
	{
		lua_pushvalue(L, -2);
		lua_call(L, 1, 1);

		if (lua_isnil(L, -1) == false)
			val = *((T **) lua_topointer(L, -1));

		lua_pop(L, 1); // pop the FFI pointer/nil value
	}
	else
		lua_pop(L, 1); // pop nil value

	// If Data:getFFIPointer fails, use Data:getPointer
	// probably because FFI is disabled/unavailable or using version < 11.3
	if (val == nullptr)
	{
		lua_getfield(L, -1, "getPointer");
		lua_pushvalue(L, -2);
		lua_call(L, 1, 1);
		val = (T*)lua_topointer(L, -1);
		lua_pop(L, 1); // pop the pointer
	}

	// Leave the ByteData in stack
	return val;
}

Live2LOVE::Live2LOVE(lua_State *L, const void *buf, size_t size)
: moc(nullptr)
, model(nullptr)
, motion(nullptr)
, expression(nullptr)
, eyeBlink(nullptr)
, physics(nullptr)
, breath(nullptr)
, pose(nullptr)
, L(L)
, movementAnimation(true)
, eyeBlinkMovement(true)
, motionLoop("")
{
	// initialize clip fragment shader
	if (stencilFragRef == LUA_REFNIL)
	{
		RefData::getRef(L, "love.graphics.newShader");
		lua_pushstring(L, stencilFragment);
		lua_pcall(L, 1, 1, 0);
		if (lua_toboolean(L, -2) == 0)
		{
			namedException temp(lua_tostring(L, -1));
			lua_pop(L, 2);
			throw temp;
		}
		stencilFragRef = RefData::setRef(L, -1);
		lua_pop(L, 1);
	}

	// Init moc
	moc = CubismMoc::Create((csmByte *) buf, size);
	if (moc == nullptr)
		throw namedException("Failed to initialize moc");

	// Init model
	model = moc->CreateModel();
	if (model == nullptr)
		throw namedException("Failed to intialize model");
	
	// Get model dimensions
	csmVector2 tmpSizeInPixels;
	csmVector2 tmpOriginInPixels;
	csmReadCanvasInfo(model->GetModel(), &tmpSizeInPixels, &tmpOriginInPixels, &modelPixelUnits);
	modelWidth = tmpSizeInPixels.X;
	modelHeight = tmpSizeInPixels.Y;
	modelOffX = tmpOriginInPixels.X;
	modelOffY = tmpOriginInPixels.Y;

	// Update model
	model->Update();
	model->SaveParameters();
	// Setup mesh data
	setupMeshData();
	// Initialize default eye and breath
	loadEyeBlink();
	loadBreath();
}

Live2LOVE::~Live2LOVE()
{
	// Delete all mesh
	for (auto mesh: meshData)
	{
		RefData::delRef(L, mesh->tableRefID);
		RefData::delRef(L, mesh->meshRefID);
		delete mesh;
	}

	// Delete all motions
	for (auto motion: motionList)
		CubismMotion::Delete(motion.second);

	// Delete all expressions
	for (auto exprs: expressionList)
		CubismExpressionMotion::Delete(exprs.second);

	CSM_DELETE(motion);
	CSM_DELETE(expression);
	CubismBreath::Delete(breath);
	CubismEyeBlink::Delete(eyeBlink);
	CubismPhysics::Delete(physics);
	moc->DeleteModel(model);
	CubismMoc::Delete(moc);
}

void Live2LOVE::setupMeshData()
{
	// Check stack
	lua_checkstack(L, 64);
	// Get drawable count
	int drawableCount = model->GetDrawableCount();
	meshData.reserve(drawableCount);
	// Get render order
	const csmInt32 *renderOrders = model->GetDrawableRenderOrders();
	// Push newMesh
	RefData::getRef(L, "love.graphics.newMesh");
	
	// Load mesh
	for (int i = 0; i < drawableCount; i++)
	{	
		// Create new mesh object
		Live2LOVEMesh *mesh = new Live2LOVEMesh;
		mesh->model = model;
		mesh->index = i; // assume 0-based ID
		mesh->textureIndex = model->GetDrawableTextureIndices(i);
		mesh->blending = model->GetDrawableBlendMode(i);
		mesh->renderOrder = renderOrders[i];

		// Create mesh table list
		int numPoints = mesh->numPoints = model->GetDrawableVertexCount(i);
		int indexCount = model->GetDrawableVertexIndexCount(i);
		const csmUint16 *vertexMap = model->GetDrawableVertexIndices(i);
		const csmVector2 *uvmap = model->GetDrawableVertexUvs(i);
		const csmVector2 *points = model->GetDrawableVertexPositions(i);
		
		// Build mesh
		lua_pushvalue(L, -1); // love.graphics.newMesh
		lua_pushinteger(L, numPoints);
		lua_pushstring(L, "triangles"); // Mesh draw mode
		lua_pushstring(L, "stream"); // Mesh usage
		lua_call(L, 3, 1); // love.graphics.newMesh
		mesh->meshRefID = RefData::setRef(L, -1); // Add mesh reference

		// Set index map
		lua_getfield(L, -1, "setVertexMap");
		lua_pushvalue(L, -2);
		csmUint16 *tempMap = createData<csmUint16>(L, indexCount);
		memcpy(tempMap, vertexMap, indexCount * sizeof(csmUint16));
		lua_pushstring(L, "uint16");
		lua_call(L, 3, 0); // tempMap is no longer valid

		// Pop the Mesh object
		lua_pop(L, 1);
		
		Live2LOVEMeshFormat *meshDataRaw = createData<Live2LOVEMeshFormat>(L, numPoints);
		for (int j = 0; j < numPoints; j++)
		{
			Live2LOVEMeshFormat& m = meshDataRaw[j];
			// Mesh table format: {x, y, u, v, r, g, b, a}
			// r, g, b will be 1
			// Textures in OpenGL are flipped but aren't in LOVE so the Y position is flipped
			// to take that into account.
			m.x = points[j].X * modelPixelUnits + modelOffX;
			m.y = points[j].Y * -modelPixelUnits + modelOffY;
			m.u = uvmap[j].X;
			m.v = 1.0f - uvmap[j].Y;
			m.r = m.g = m.b = m.a = 255; // set later
		}
		mesh->tableRefID = RefData::setRef(L, -1); // Add FileData reference
		mesh->tablePointer = meshDataRaw;
		lua_pop(L, 1); // pop the FileData reference

		// Push to vector
		meshData.push_back(mesh);
		meshDataMap[fromCsmString(model->GetDrawableId(i)->GetString())] = mesh;
	}

	const csmInt32 *clipCount = model->GetDrawableMaskCounts();
	const csmInt32 **clipMask = model->GetDrawableMasks();

	// Find clip ID list
	for (int i = 0; i < drawableCount; i++)
	{
		Live2LOVEMesh *mesh = meshData[i];

		if (clipCount[i] > 0)
		{
			for (unsigned int k = 0; k < clipCount[i]; k++)
				mesh->clipID.push_back(meshData[clipMask[i][k]]);
		}
	}
}

void Live2LOVE::update(double dt)
{
	// Motion update
	if (motion)
	{
		model->LoadParameters();
		if (motion->IsFinished() && motionLoop.length() > 0)
			// Revert
			motion->StartMotion(motionList[motionLoop], false, 1.0f);

		if (!motion->UpdateMotion(model, dt) && movementAnimation)
		{
			if (eyeBlink && eyeBlinkMovement)
				// Update eye blink
				eyeBlink->UpdateParameters(model, dt);
		}
		model->SaveParameters();
	}

	// Expression update
	if (expression)
		expression->UpdateMotion(model, dt);

	// Movement update
	if (movementAnimation)
	{
		// Breath update
		if (breath)
			breath->UpdateParameters(model, dt);

		// Physics update
		if (physics)
			physics->Evaluate(model, dt);
	}

	if (pose)
		pose->UpdateParameters(model, dt);

	model->Update();

	// If there's parameter change, set it
	for (auto& list: postParamUpdateList)
	{
		const CubismId *paramName = toCsmString(list.first);
		model->SetParameterValue(model->GetParameterIndex(paramName), list.second->first, list.second->second);

		delete list.second;
		list.second = nullptr;
	}
	postParamUpdateList.clear();

	// Update model
	model->Update();

	// Get render orders
	auto renderOrders = model->GetDrawableRenderOrders();

	// Update mesh data
	for (auto mesh: meshData)
	{
		// Set render order
		mesh->renderOrder = renderOrders[mesh->index];

		// Get mesh ref
		RefData::getRef(L, mesh->meshRefID);
		// Get "setVertices"
		lua_getfield(L, -1, "setVertices");
		lua_pushvalue(L, -2);
		// Get data mesh ref
		RefData::getRef(L, mesh->tableRefID);
		float visibility = model->GetDrawableDynamicFlagIsVisible(mesh->index) ? 1.0f : 0.0f;
		float opacity = visibility * model->GetDrawableOpacity(mesh->index);
		const csmVector2 *points = model->GetDrawableVertexPositions(mesh->index);

		// Update
		for (int i = 0; i < mesh->numPoints; i++)
		{
			Live2LOVEMeshFormat& m = mesh->tablePointer[i];
			m.x = points[i].X * modelPixelUnits + modelOffX;
			m.y = points[i].Y * -modelPixelUnits + modelOffY;
			m.a = (unsigned char) floor(opacity * 255.0f + 0.5f);
		}

		// Call setVertices
		lua_call(L, 2, 0);
	}
	// Update draw order
	std::sort(meshData.begin(), meshData.end(), compareDrawOrder);
}

void Live2LOVE::draw(double x, double y, double r, double sx, double sy, double ox, double oy, double kx, double ky)
{
	// Damn the name is too long
	constexpr auto
		NormalBlending = Rendering::CubismRenderer::CubismBlendMode_Normal,
		AddBlending = Rendering::CubismRenderer::CubismBlendMode_Additive,
		MultiplyBlending = Rendering::CubismRenderer::CubismBlendMode_Multiplicative;

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

	// Blending mode
	auto blendMode = NormalBlending; // alpha,alphamultiply
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
			RefData::getRef(L, "love.graphics.stencil");
			// Push upvalues
			encodePointer(L, mesh);
			lua_pushnumber(L, x);
			lua_pushnumber(L, y);
			lua_pushnumber(L, r);
			lua_pushnumber(L, sx);
			lua_pushnumber(L, sy);
			lua_pushnumber(L, ox);
			lua_pushnumber(L, oy);
			lua_pushnumber(L, kx);
			lua_pushnumber(L, ky);
			lua_pushcclosure(L, Live2LOVE::drawStencil, 10); // push stencil draw function
			lua_pushstring(L, "increment");
			stencilCount = 0;
			lua_call(L, 2, 0); // love.graphics.stencil

			RefData::getRef(L, "love.graphics.setStencilTest");
			lua_pushstring(L, "gequal");
			lua_pushnumber(L, stencilCount);
			lua_call(L, 2, 0); // love.graphics.setStencilTest
			stencilSet = true;
		}

		auto meshBlendMode = mesh->blending;
		if (meshBlendMode != blendMode)
		{
			// Push love.graphics.setBlendMode
			lua_pushvalue(L, -4);
			switch (blendMode = meshBlendMode)
			{
				default:
				case NormalBlending:
				{
					// Normal blending (alpha,alphamultiply)
					lua_pushstring(L, "alpha");
					lua_pushstring(L, "alphamultiply");
					break;
				}
				case AddBlending:
				{
					// Add blending (add,alphamultiply)
					lua_pushstring(L, "add");
					lua_pushstring(L, "alphamultiply");
					break;
				}
				case MultiplyBlending:
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

		// If there's stencil, disable it.
		if (stencilSet)
		{
			RefData::getRef(L, "love.graphics.setStencilTest");
			lua_call(L, 0, 0);
		}
	}

	// Remove love.graphics.draw
	lua_pop(L, 1);
	// Reset blend mode
	lua_call(L, 2, 0);
}

void Live2LOVE::setTexture(int live2dtexno, int loveimageidx)
{
	// List mesh
	for (auto mesh: meshData)
	{
		if (mesh->textureIndex == live2dtexno - 1)
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

void Live2LOVE::setAnimationMovement(bool a)
{
	movementAnimation = a;
}

void Live2LOVE::setEyeBlinkMovement(bool a)
{
	eyeBlinkMovement = a;
}

bool Live2LOVE::isAnimationMovementEnabled() const
{
	return movementAnimation;
}

bool Live2LOVE::isEyeBlinkEnabled() const
{
	return eyeBlinkMovement;
}

void Live2LOVE::setParamValue(const std::string& name, double value, double weight)
{
	const CubismId *paramName = CubismFramework::GetIdManager()->GetId(name.c_str());
	model->SetParameterValue(model->GetParameterIndex(paramName), value, weight);
}

void Live2LOVE::setParamValuePost(const std::string& name, double value, double weight)
{
	postParamUpdateList[name] = new std::pair<double, double>(value, weight);
}

void Live2LOVE::addParamValue(const std::string& name, double value, double weight)
{
	const CubismId *paramName = CubismFramework::GetIdManager()->GetId(name.c_str());
	model->AddParameterValue(paramName, (float) value, (float) weight);
}

void Live2LOVE::mulParamValue(const std::string& name, double value, double weight)
{
	const CubismId *paramName = CubismFramework::GetIdManager()->GetId(name.c_str());
	model->MultiplyParameterValue(paramName, value, weight);
}

double Live2LOVE::getParamValue(const std::string& name) const
{
	const CubismId *paramName = CubismFramework::GetIdManager()->GetId(name.c_str());
	return model->GetParameterValue(paramName);
}

std::vector<Live2LOVEParamDef> Live2LOVE::getParamInfoList()
{
	csmModel *mdl = model->GetModel();
	int paramCount = csmGetParameterCount(mdl);
	const char **paramIDs = csmGetParameterIds(mdl);
	const float *minVals = csmGetParameterMinimumValues(mdl);
	const float *maxVals = csmGetParameterMaximumValues(mdl);
	const float *defVals = csmGetParameterDefaultValues(mdl);

	// Initialize vector
	std::vector<Live2LOVEParamDef> params;
	params.reserve(paramCount);

	for (int i = 0; i < paramCount; i++)
		params.push_back({paramIDs[i], minVals[i], maxVals[i], defVals[i]});
	
	return params;
}

std::vector<const std::string*> Live2LOVE::getExpressionList() const
{
	std::vector<const std::string*> value = {};

	for (auto& x: expressionList)
		value.push_back(&x.first);

	return value;
}

std::vector<const std::string*> Live2LOVE::getMotionList() const
{
	std::vector<const std::string*> value = {};

	for (auto& x: motionList)
		value.push_back(&x.first);

	return value;
}

std::pair<float, float> Live2LOVE::getDimensions() const
{
	return std::pair<float, float>(modelWidth, modelHeight);
}

void Live2LOVE::setMotion(const std::string& name, MotionModeID mode)
{
	// No motion? well load one first before using this.
	if (!motion)
		throw namedException("No motion loaded!");
	else if (motionList.find(name) == motionList.end())
		throw namedException("Motion not found");

	CubismMotion *targetMotion = motionList[name];
	motion->StartMotion(targetMotion, false, targetMotion->GetFadeInTime());

	// Check motion mode
	if (mode == 1) motionLoop = name;
	else if (mode == 2) motionLoop = "";
}

void Live2LOVE::setMotion()
{
	// clear motion
	if (!motion)
		throw namedException("No motion loaded!");

	motionLoop = "";
	motion->StopAllMotions();
}

void Live2LOVE::setExpression(const std::string& name)
{
	// No expression? Load one first!
	if (!expression)
		throw namedException("No expression loaded!");
	else if (expressionList.find(name) == expressionList.end())
		throw namedException("Expression not found");

	CubismExpressionMotion *expr = expressionList[name];
	expression->StartMotion(expressionList[name], false, expr->GetFadeInTime());
}

void Live2LOVE::loadMotion(const std::string& name, const std::pair<double, double>& fade, const void *buf, size_t size)
{
	initializeMotion();

	// Load file
	CubismMotion *motion = CubismMotion::Create((csmByte *) buf, size);
	if (motion == nullptr)
		throw namedException("Failed to load motion");

	motion->SetFadeInTime(fade.first);
	motion->SetFadeOutTime(fade.second);

	// Set motion
	if (motionList.find(name) != motionList.end())
		CubismMotion::Delete(motionList[name]);

	motionList[name] = motion;
}

void Live2LOVE::loadExpression(const std::string& name, const void *buf, size_t size)
{
	initializeExpression();

	// Load file
	CubismExpressionMotion *expr = CubismExpressionMotion::Create((csmByte *) buf, size);
	if (expr == nullptr) throw namedException("Failed to load expression");

	// Set expression
	if (expressionList.find(name) != expressionList.end())
		CubismExpressionMotion::Delete(expressionList[name]);

	expressionList[name] = expr;
}

void Live2LOVE::loadPhysics(const void *buf, size_t size)
{
	// Load file
	if (physics)
		CubismPhysics::Delete(physics);

	physics = CubismPhysics::Create((csmByte *) buf, size);
	if (physics == nullptr)
		throw namedException("Failed to load physics");
}

void Live2LOVE::loadPose(const void *buf, size_t size)
{
	if (pose)
		CubismPose::Delete(pose);

	pose = CubismPose::Create((csmByte *) buf, size);
	if (pose == nullptr)
		throw namedException("Failed to load pose");
}

void Live2LOVE::initializeMotion()
{
	if (!motion)
		motion = CSM_NEW CubismMotionManager();
}

void Live2LOVE::initializeExpression()
{
	if (!expression)
		expression = CSM_NEW CubismMotionManager();
}

void Live2LOVE::loadEyeBlink(const std::vector<std::string> &names)
{
	if (!eyeBlink)
		eyeBlink = CubismEyeBlink::Create();

	csmVector<CubismIdHandle> tmpNames;
	
	for(const std::string &name: names)
		tmpNames.PushBack(toCsmString(name));

	eyeBlink->SetParameterIds(tmpNames);
}

void Live2LOVE::loadEyeBlink()
{
	if (!eyeBlink)
		eyeBlink = CubismEyeBlink::Create();

	static bool idInitialized = false;
	static csmVector<CubismIdHandle> eyeDefault;

	if (!idInitialized)
	{
		eyeDefault.PushBack(toCsmString(DefaultParameterId::ParamEyeLOpen));
		eyeDefault.PushBack(toCsmString(DefaultParameterId::ParamEyeROpen));
		idInitialized = true;
	}

	eyeBlink->SetParameterIds(eyeDefault);
}

void Live2LOVE::loadBreath(const std::vector<Live2LOVEBreath> &names)
{
	if (!breath)
		breath = CubismBreath::Create();

	csmVector<CubismBreath::BreathParameterData> tmpNames;
	
	for(const Live2LOVEBreath &param: names)
		tmpNames.PushBack({
			toCsmString(param.paramName),
			(float) param.offset,
			(float) param.peak,
			(float) param.cycle,
			(float) param.weight
		});

	breath->SetParameters(tmpNames);
}

void Live2LOVE::loadBreath()
{
	if (!breath)
		breath = CubismBreath::Create();

	static bool idInitialized = false;
	static csmVector<CubismBreath::BreathParameterData> breathDefault;

	if (!idInitialized)
	{
		breathDefault.PushBack(CubismBreath::BreathParameterData(toCsmString(DefaultParameterId::ParamAngleX), 0.0f, 15.0f, 6.5345f, 0.5f));
		breathDefault.PushBack(CubismBreath::BreathParameterData(toCsmString(DefaultParameterId::ParamAngleY), 0.0f, 8.0f, 3.5345f, 0.5f));
		breathDefault.PushBack(CubismBreath::BreathParameterData(toCsmString(DefaultParameterId::ParamAngleZ), 0.0f, 10.0f, 5.5345f, 0.5f));
		breathDefault.PushBack(CubismBreath::BreathParameterData(toCsmString(DefaultParameterId::ParamBodyAngleX), 0.0f, 4.0f, 15.5345f, 0.5f));
		idInitialized = true;
	}

	breath->SetParameters(breathDefault);
}

std::pair<float, float> Live2LOVE::getModelCenterPosition()
{
	return std::pair<float, float>(modelOffX, modelOffY);
}

int Live2LOVE::drawStencil(lua_State *L)
{
	int t = lua_gettop(L);
	lua_checkstack(L, t + 16);

	// Get current shader
	RefData::getRef(L, "love.graphics.getShader");
	lua_call(L, 0, 1); // shader = stack 1

	// Set shader
	RefData::getRef(L, "love.graphics.setShader");
	RefData::getRef(L, stencilFragRef);
	lua_call(L, 1, 0); // love.graphics.setShader(stencilFragRef)

	// get love.graphics.draw
	RefData::getRef(L, "love.graphics.draw");

	// Upvalues:
	// 1. Mesh pointer
	// 2-10: draw args
	Live2LOVEMesh *mesh = decodePointer<Live2LOVEMesh *>(L, lua_upvalueindex(1));

	drawStencil2(L, mesh);

	// Reset shader
	RefData::getRef(L, "love.graphics.setShader");
	lua_pushvalue(L, -3);
	lua_call(L, 1, 0);
	lua_pop(L, 2); // love.graphics.draw and used shader

	return 0;
}

void Live2LOVE::drawStencil2(lua_State *L, Live2LOVEMesh *mesh)
{
	for (auto x: mesh->clipID)
	{
		if (x->clipID.size() > 0)
			drawStencil2(L, x);

		lua_pushvalue(L, -1); // love.graphics.draw
		RefData::getRef(L, x->meshRefID); // Mesh
		for (int i = 2; i <= 10; i++)
			lua_pushvalue(L, lua_upvalueindex(i)); // the rest/upvalues

		lua_call(L, 10, 0);
		stencilCount++;
	}
}

} /* live2love */
