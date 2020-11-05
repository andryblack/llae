solution '${project.name}'
	objdir 'build' 
		location 'build'

		configuration{ 'debug'}
			symbols "On"
		configuration{ 'release'}
			optimize "On"
		configuration{}


		llae.dependencies()
		llae.lib()

		project '${project.name}'
			kind 'ConsoleApp'
			targetdir 'bin'
			targetname '${project.name}'
			
			llae.exe()

