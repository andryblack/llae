local class = require 'llae.class'

---@class web.middle 
---@field protected _app web.application?
local middle = class(nil,'web.middle')

function middle:use(app)
end

return middle