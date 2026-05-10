local uv = require 'llae.uv'

local llae = require 'llae'

---@class llae.os : uv.os
local _M = setmetatable({},{__index=uv.os})


_M.hostname = uv.os.gethostname
_M.at_exit = llae.at_exit
_M.stop = llae.stop


return _M