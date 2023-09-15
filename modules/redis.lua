name = 'redis'

dependencies = {
	'llae',
}

function install()
	
end

cmodules = {
	'db.redis.native',
}

build_lib = {
	project = [[
		includedirs{
			<%= format_mod_file(project:get_module('llae'),'src')%>,
		}
		sysincludedirs {
			'include',
		}
		files {
			<%= format_mod_file(project:get_module('llae'),'src','modules','db','redis.cpp') %>,
		}
]]
}
