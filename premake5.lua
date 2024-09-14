
local llae = require 'premake.llae'

newaction {
	trigger = 'download',
	shortname = 'download dependencies',
	description = 'download dependencies for build',
	execute = function()
		llae.download()
		os.exit(0)
	end,

}

newaction {
	trigger = 'unpack',
	shortname = 'unpack dependencies',
	description = 'unpack dependencies for build',
	execute = function()
		llae.unpack()
		os.writefile_ifnotequal([[
	LLAE_VERSION = "bootstrap"
]],'build/_build_config.lua')
		os.exit(0)
	end,

}

if _ACTION ~= 'download' and _ACTION ~= 'unpack' then
	solution 'llae'
		objdir 'build' 
		location 'build'
		llae.solution()

		
		filter{ 'configurations:debug'}
			symbols "On"
		filter{ 'configurations:release'}
			optimize "On"
		filter{}


		llae.dependencies()
		llae.lib()

		project 'llae-tool'
			kind 'ConsoleApp'
			targetdir 'bin'
			targetname 'llae-bootstrap'
			llae.exe()
end
	