-- Copyright (c) 2039 Dark Energy Processor Corporation
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

-- Example Live2LOVE using Miku model file
local love = require("love")
local Live2LOVE = require("Live2LOVE")
local mikuModel, mikuMotion
local motionStr = "List of motions (press key number to change):\n"

function love.load()
	-- Load model. loadModel expects model definition (JSON file)
	mikuModel = Live2LOVE.loadModel("miku/miku.model.json")
	-- Get list of motions
	mikuMotion = mikuModel:getMotionList()
	-- Format motions. Faster & better approach is possible to handle the strings.
	-- Note that the keyboard input only supports 10 keys (1-9, 0)
	for i = 1, 10 do
		if not(mikuMotion[i]) then break end
		motionStr = motionStr..string.format("%d. %s\n", i % 10, mikuMotion[i])
	end
end

function love.update(dt)
	-- Update model
	mikuModel:update(dt)
end

function love.draw()
	-- Draw model at (0,0)
	-- Note that the model drawing is AFFECTED by LOVE graphics state.
	-- This include transformation, shader, colors, FBOs, ... except blend modes.
	-- This is because the model is rendered entirely with LOVE built-in Mesh object
	-- instead of Live2d-supplied rendering function.
	mikuModel:draw(0, 0, 0, 0.8, 0.8)
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
end

function love.keyreleased(key)
	-- Only accept key numbers (not numlock one)
	local keynum = tonumber(key)
	if not(keynum) then return end
	if keynum < 0 and keynum > 9 then return end
	if keynum == 0 then keynum = 10 end
	
	-- Play just once
	if mikuMotion[keynum] then
		mikuModel:setMotion(mikuMotion[keynum], "normal")
	end
end
