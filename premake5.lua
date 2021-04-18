
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

if _ACTION ~= 'download' then
	solution 'llae'
		objdir 'build' 
		location 'build'
		llae.solution()

		
		configuration{ 'debug'}
			symbols "On"
		configuration{ 'release'}
			optimize "On"
		configuration{}


		llae.dependencies()
		llae.lib()

		project 'llae-tool'
			kind 'ConsoleApp'
			targetdir 'bin'
			targetname 'llae-bootstrap'
			llae.exe()
end
	