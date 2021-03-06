


solution 'llae'
	configurations { 'debug', 'release' }
	language 'c++'
	objdir 'build' 
	location 'build'
	cppdialect "C++11"

	
	configuration{ 'debug'}
		symbols "On"
	configuration{ 'release'}
		optimize "On"
	configuration{}

	local llae = require 'premake.llae'

	llae.lib()

	project 'llae-run'
		kind 'ConsoleApp'
		targetdir 'bin'
		llae.exe()
	