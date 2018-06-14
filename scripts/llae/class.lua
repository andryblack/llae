return function(base,name)
  base = base or {}
  -- inheristance via copy
  local class = {}
  for k, v in pairs(base) do
    class[k] = v
  end
  class.__index = class
  class.is_a = {[class] = true}
  if name then
    class.is_a[name] = true
    class._name = name
  end
  for c in pairs(base.is_a or {}) do
    class.is_a[c] = true
  end
  class.is_a[base] = true
  class.baseclass = base

  function class.new(...)
    local instance = setmetatable({},class)
    local init = instance._init
    if init then
      init(instance,...)
    end
    return instance
  end
  return class
end
