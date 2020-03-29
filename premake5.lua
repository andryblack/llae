
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
		configurations { 'debug', 'release' }
		language 'c++'
		objdir 'build' 
		location 'build'
		cppdialect "C++14"

		
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
			targetname 'llae'
			llae.embed({
				{ 'scripts/llae/**.lua', 'scripts' },
				{ 'scripts/net/*.lua', 'scripts' },
				{ 'scripts/tools/*.lua', 'scripts/tools' }
				})
			llae.exe()
end
	