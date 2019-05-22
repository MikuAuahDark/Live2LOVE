-- Copyright (c) 2040 Dark Energy Processor Corporation
--
-- This software is provided 'as-is', without any express or implied
-- warranty.  In no event will the authors be held liable for any damages
-- arising from the use of this software.
--
-- Permission is granted to anyone to use this software for any purpose,
-- including commercial applications, and to alter it and redistribute it
-- freely, subject to the following restrictions:
--
-- 1. The origin of this software must not be misrepresented; you must not
--    claim that you wrote the original software. If you use this software
--    in a product, an acknowledgment in the product documentation would be
--    appreciated but is not required.
-- 2. Altered source versions must be plainly marked as such, and must not be
--    misrepresented as being the original software.
-- 3. This notice may not be removed or altered from any source distribution.

-- Example Live2LOVE
local love = require("love")
local Live2LOVE = require("Live2LOVE")
local modelObj, modelMotion, modelMesh
local motionStr = "List of motions (press key number to change):\n"
local modelMeshDrawIdx = 0

print("Live2D Version "..Live2LOVE.Live2DVersion)

function love.load()
	-- Load model. loadModel expects model definition (JSON file)
	--modelObj = Live2LOVE.loadModel("rev/model.model3.json")
	modelObj = Live2LOVE.loadModel("Res/Haru/Haru.model3.json")
	-- Get list of motions
	modelMotion = modelObj:getMotionList()
	-- Format motions. Faster & better approach is possible to handle the strings.
	-- Note that the keyboard input only supports 10 keys (1-9, 0)
	for i = 1, 10 do
		if not(modelMotion[i]) then break end
		motionStr = motionStr..string.format("%d. %s\n", i % 10, modelMotion[i])
	end
	print(string.format("Dimensions %gx%g", modelObj:getDimensions()))
	
	-- Get model mesh
	modelMesh = modelObj:getMesh()
end

function love.update(dt)
	-- Update model
	modelObj:update(dt)
end

function love.draw()
	-- Draw model at (0,0)
	-- Note that the model drawing is AFFECTED by LOVE graphics state.
	-- This include transformation, shader, colors, FBOs, ... except blend modes.
	-- This is because the model is rendered entirely with LOVE built-in Mesh object
	-- instead of Live2d-supplied rendering function.
	if modelMeshDrawIdx > 0 and love.keyboard.isDown("return") == false then
		love.graphics.draw(modelMesh[modelMeshDrawIdx], 400, 600, 0, 0.2, 0.2)
	else
		modelObj:draw(400, 600, 0, 0.2, 0.2)
	end
	-- Draw information
	local stats = love.graphics.getStats()
	love.graphics.print(string.format(
		"Live2LÖVE v%s using Live2D Cubism SDK v%s\nModel Drawcalls: %d\nFPS: %d",
		Live2LOVE._VERSION,
		Live2LOVE.Live2DVersion,
		stats.drawcalls,
		love.timer.getFPS()
	))
	-- Draw motion list string
	love.graphics.print(motionStr, 0, 50)
	love.graphics.print(modelMeshDrawIdx, 2, 600-16)
end

function love.keyreleased(key)
	-- Only accept key numbers (not numlock one)
	local keynum = tonumber(key)
	if not(keynum) then
		if key == "left" then
			modelMeshDrawIdx = (modelMeshDrawIdx - 1) % (#modelMesh + 1)
		elseif key == "right" then
			modelMeshDrawIdx = (modelMeshDrawIdx + 1) % (#modelMesh + 1)
		end
	else
		if keynum < 0 and keynum > 9 then return end
		if keynum == 0 then keynum = 10 end
		
		-- Play just once
		if modelMotion[keynum] then
			modelObj:setMotion(modelMotion[keynum], "normal")
		end
	end
end
