local uv = require 'uv'
local _M = setmetatable({},{__index=uv})

_M.os = require 'uv.os'
_M.fs = require 'uv.fs'

return _M