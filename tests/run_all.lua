local g = _G
local remove = {}
local log = require 'llae.log'
local fs = require 'llae.fs'
local function is_script_file(k)
	local path = 'scripts'
	while true do
		local dot = string.find(k,'.',1,true)
		if not dot then
			break
		end
		path = path .. '/' .. string.sub(k,1,dot-1)
		k = string.sub(k,dot+1)
	end
	return fs.isfile(path .. '/' .. k .. '.lua')
end
for k,v in pairs(g.package.loaded) do
	if is_script_file(k) then
		--log.debug('skip loaded',k)
		table.insert(remove,k)
	end
end
for _,v in ipairs(remove) do
	g.package.loaded[v] = nil
end
g.package.path = 'scripts/?.lua'
g.package.preload['luaunit'] = function() 
	return dofile 'build/scripts/luaunit.lua' 
end
-- remove embedded searcher
table.remove(package.searchers,1)

local lu = require 'luaunit'

if args.verbose then
	lu:setVerbosity(lu.VERBOSITY_VERBOSE)
end

local async = require 'llae.async'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'
async.run(function()
	for _,v in ipairs(fs.scandir('tests')) do
		if v.isfile and v.name:sub(-4)=='.lua' and v.name:sub(1,5) == 'test_' then
			log.debug('found file',v.name)
			dofile(path.join('tests',v.name))
		end
	end
	os.exit( lu.LuaUnit.run() )
end)