
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

local extlibs = { 'lua-5.3', 'libuv', 'yajl', 'openssl' }
local components = { 'common','lua','meta','uv','json','llae' }

local function pkgconfig_components(flag)
	local r = {}
	for _,v in ipairs(extlibs) do
		table.insert(r,pkgconfig(v,flag))
	end
	return r
end

function _M.lib(  )
	project 'llae'
		kind 'StaticLib'
		location 'build'
		targetdir 'lib'
		
		if os.istarget('macosx') then
			includedirs(pkgconfig_var('yajl','prefix')..'/include')
		end

		_M.compile()
		
		local fls = {}
		for _,c in ipairs(components) do
			table.insert(fls,path.join(_M.root,'src',c,'*.cpp'))
			table.insert(fls,path.join(_M.root,'src',c,'*.h'))
		end
		files(fls)

end

function _M.compile(  )
	buildoptions(pkgconfig_components('cflags'))
	includedirs{
		path.join(_M.root ,'src') 
	}
end

function _M.exe(  )
	_M.compile()
	files {
		path.join(_M.root,'src','main.cpp'),
	}
	_M.link()
end

function _M.link(  )
	linkoptions(pkgconfig_components('libs'))
	links {
		'llae'
	}
end

function _M.builddir( )
	local prj = configuration().name
	return path.getabsolute(path.join(_MAIN_SCRIPT_DIR, 'build', prj))
end

local function load_file(fn)
	local fname = path.getabsolute(fn)
	local f = io.open(fname, "rb")
	local s = assert(f:read("*all"))
	f:close()
	return s
end

local function output_script(result, data)
	local length = #data
	buffered.write(result,'\t')
	for i = 1, length do
		buffered.write(result, string.format("0x%02x,", data:byte(i)))
		if (i % 16 == 0) and i~=length then
			buffered.writeln(result)
			buffered.write(result,'\t')
		end
	end
	buffered.writeln(result)
end

function _M.embed( patterns )
	local result = buffered.new()
	buffered.writeln(result, "/* autogenerated embedded scripts */")
	buffered.writeln(result, '#include "lua/embedded.h"')
	

	local embedded_files = {}
	for _,v in ipairs(patterns) do
		--print('process pattern',v[1])
		local files = os.matchfiles(v[1])
		local baseDir = v[2] 
		for _,f in ipairs(files) do
			local dst_name = baseDir and path.getrelative(baseDir, f) or f
			buffered.writeln(result, "/* " .. dst_name .. " */")
			local id_name = dst_name:gsub('[%.%/]','_')
			buffered.writeln(result, "static const unsigned char " .. id_name .. '[] = {')
			local conent = load_file(f)
			output_script(result,conent)
			buffered.writeln(result,'};')
			table.insert(embedded_files,{
				id = id_name,
				name = dst_name:gsub('[%/]','.'):gsub('%.lua','')
				})
		end
	end
	

	buffered.writeln(result, "")
	
	buffered.writeln(result, " const lua::embedded_script lua::embedded_script::scripts[] = {")
	for _,v in ipairs(embedded_files) do
		buffered.writeln(result, "\t{")
		buffered.writeln(result, '\t\t"' .. v.name .. '",')
		buffered.writeln(result, '\t\t' .. v.id .. ',')
		buffered.writeln(result, '\t\tsizeof(' .. v.id .. ')')
		buffered.writeln(result, "\t},")
	end
	buffered.writeln(result, "\t{0,0,0}")
	buffered.writeln(result, "};")
	buffered.writeln(result, "")

	

	
	local dir = _M.builddir()
	os.mkdir(dir)
	local scriptsFile = path.join( dir, "embed_scripts.cpp")

	local output = buffered.tostring(result)

	files { scriptsFile }

	local f, err = os.writefile_ifnotequal(output, scriptsFile);
	if (f < 0) then
		error(err, 0)
	elseif (f > 0) then
		printf("Generated %s...", path.getrelative(os.getcwd(), scriptsFile))
	end
end

return _M