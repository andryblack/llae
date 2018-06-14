
local _M = {}

local pkgconfig = function(module,var)
	local res,status = os.outputof("pkg-config " .. module .. " --" .. var)
	assert(status==0,"not found package " .. module .. " var " .. var)
	return res
end

local pkgconfig_var = function(module,var)
	local res,status = os.outputof("pkg-config " .. module .. " --variable=" .. var)
	assert(status==0,"not found package " .. module .. " var " .. var)
	return res
end

_M.root = './'

_M.pkgconfig = pkgconfig
_M.pkgconfig_var = pkgconfig

function _M.lib(  )
	project 'llae'
		kind 'StaticLib'
		location 'build'
		targetdir 'lib'
		
		if os.istarget('macosx') then
			includedirs(pkgconfig_var('yajl','prefix')..'/include')
		end

		buildoptions{ 
			pkgconfig('lua-5.3','cflags'),
			pkgconfig('libuv','cflags'),
			pkgconfig('yajl','cflags'),
			pkgconfig('openssl','cflags'),
		}
		
		files {
			path.join(_M.root,'src','**.cpp'),
			path.join(_M.root,'src','**.h')
		}
end

function _M.link(  )
	linkoptions { 
		pkgconfig('lua-5.3','libs'),
		pkgconfig('libuv','libs'),
		pkgconfig('yajl','libs'),
		pkgconfig('openssl','libs'),
	}
	links {
		'llae'
	}
end

return _M