name = 'redis'

dependencies = {
	'llae',
}

function install()
	
end

cmodules = {
	'db.redis.resp',
}

build_lib = {
	project = [[
		includedirs{
			<%= format_mod_file(project:get_module('llae'),'src')%>,
		}
		externalincludedirs {
			'include',
		}
		files {
			<%= format_mod_file(project:get_module('llae'),'src','modules','db','redis.cpp') %>,
		}
]]
}
