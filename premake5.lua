


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

	local llae = require 'premake.llae'

	llae.lib()

	project 'llae-tool'
		kind 'ConsoleApp'
		targetdir 'bin'
		targetname 'llae'
		llae.embed({
			{ 'scripts/llae/*.lua', 'scripts' },
			{ 'scripts/net/*.lua', 'scripts' },
			{ 'scripts/tools/*.lua', 'scripts/tools' }
			})
		llae.exe()
	