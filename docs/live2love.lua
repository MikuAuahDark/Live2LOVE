---
-- Live2LOVE is a LÖVE module to load and render Live2D models
-- which uses [love.filesystem](https://love2d.org/wiki/love.filesystem) module to
-- load files, so it works even in fused mode.
--
-- Live2LOVE uses [love.graphics](https://love2d.org/wiki/love.graphics) to do the
-- whole rendering. This provides some advantages like you can apply transformation,
-- using Shader, render to Canvas, and more. At least LOVE 11.0 is required to use
-- this module!
--
-- Note that Live2D SDK is required to compile this module, as per README.md says.
-- @module Live2LOVE

--- Load cubism model file without additioal setup.
-- Only use this if your model file does lack of model definition
-- or your library (or your responsibility) to control the paths.
-- @tparam string moc Model file path.
-- @treturn Live2LOVEModel Model object
-- @raise error when the model file is not recognized.
function loadMocFile(moc)
end

--- Load model definition and fully initialize model.
-- Most user should use this function. This is recommended
-- way and most of the model preparation is handled.
-- by this function.
-- @tparam string model Model definition file path (JSON).
-- @treturn Live2LOVEModel Model object.
-- @raise error when it fails to load (due to many factor).
function loadModel(model)
end

--- This is model object
-- @type Live2LOVEModel

--- Set parameter value of model.
-- @tparam string name Parameter name.
-- @tparam number value Parameter value.
-- @tparam[opt] number weight Parameter weight (defaults to 1).
function setParamValue(name, value, weight)
end

--- Retrieve LÖVE Mesh object of specified index or all Mesh objects.
-- @tparam[opt] number index Index to get it's Mesh data (defaults to nil).
-- @return List of Mesh objects (in a table) or specified Mesh object for specified index.
-- @raise error when index is out of range.
function getMesh(index)
end

--- Update model.
-- @tparam number dT Time elapsed since last frame in seconds.
function update(dT)
end

--- Draw model.
-- Drawing Live2D model object is done using love.graphics.draw,
-- which means that, for example, current transformation stack and
-- Shader affects the model rendering.
-- 
-- Note that if you're rendering the model into Canvas, the Canvas
-- must have stencil buffer to be set (or available), or you'll getting
-- error that stencil buffer is not set!
function draw()
end

--- Set model expression.
-- @tparam string name Expression name.
-- @raise error when there are no expressions loaded, initialization failure, or expression with specified name does not exist.
function setExpression(name)
end

--- Set model motion.
-- @tparam string name Motion name.
-- @param[opt] mode How to handle the motion.  
-- 1. "normal" (or 1) will play the motion for once then revert back to previous motion.  
-- 2. "loop" (or 2) will play the motion in loop. That's it. It plays the motion again when it's finished.  
-- 3. "preserve" (or 3) will play the motion for once and stays that way.  
-- If absent, "normal" mode is used.
-- @raise error when there are no motions loaded, initialization failure, or motion with specified name does not exist.
function setMotion(name, mode)
