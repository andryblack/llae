
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

		local embedded_modules = {
			'local _M={}'
		}
		local files = os.matchfiles('modules/*.lua')
		for _,f in ipairs(files) do
			local n = path.getbasename(f)
			print('embedded module',n,f)
			table.insert(embedded_modules,'_M.' .. n .. '=[====[')
			table.insert(embedded_modules,assert(io.readfile(f)))
			table.insert(embedded_modules,']====]')
		end

		table.insert(embedded_modules,'return _M')

		project 'llae-tool'
			kind 'ConsoleApp'
			targetdir 'bin'
			targetname 'llae'
			llae.embed({
					{ 'scripts/llae/**.lua', 'scripts' },
					{ 'scripts/net/*.lua', 'scripts' },
					{ 'tools/*.lua', 'tools' },
				},{
					['embedded_modules.lua'] = table.concat(embedded_modules,'\n')
				})
			llae.exe()
end
	