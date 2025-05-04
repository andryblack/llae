local g = _G
local remove = {}
local log = require 'llae.log'
local fs = require 'llae.fs'
local path = require 'llae.path'
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
else
	log.redirect('build/test.log.txt',true)
end

local bypassargs = {}
local i = 1
while args[i] do
	bypassargs[i] = args[i]
	i = i + 1
end


local async = require 'llae.async'
local fs = require 'llae.fs'
local path = require 'llae.path'
async.run(function()
	for _,v in ipairs(fs.scanfiles_r('tests')) do
		local filename = path.basename(v)
		if path.extension(v)=='lua' and filename:sub(1,5) == 'test_' then
			log.debug('found file',v)
			dofile(path.join('tests',v))
		end
	end
	os.exit( lu.LuaUnit.run(table.unpack(bypassargs)) )
end)