---Creates a new class with optional inheritance and type checking.
---Supports single inheritance with automatic method copying and type checking.
---@generic T, U
---@param base `U`? Base class to inherit from. Defaults to an empty table.
---@param name `T`? Optional name for the class (useful for type checking)
---@return {} : U A new class table with constructor, inheritance tracking, and base class reference
local class_func = function(base,name)
  -- inheristance via copy
  ---@diagnostic disable
  ---@type table
  local class = {}
  if base then
    for k, v in pairs(base) do
      class[k] = v
    end
  end
  class.__index = class
  ---@type table<string|type,boolean>
  class.is_a = {[class] = true}
  if name then
    class.is_a[name] = true
    class._name = name
  end
  if base then
    for c in pairs(base.is_a or {}) do
      class.is_a[c] = true
    end
    class.is_a[base] = true
    class.baseclass = base
  else 
    --class.baseclass = {}
  end

  function class.new(...)
    --- Creates a new instance of the class.
    --- If the class has an _init method, it's called with the provided arguments.
    ---@return table A new instance with the class as its metatable
    local instance = setmetatable({},class)
    ---@type fun(...)
    local init = instance._init
    if init then
      init(instance,...)
    end
    return instance
  end
  return class
end

return class_func