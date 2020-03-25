
local _M = {}

_M.root = './'

local components = { 'common','lua','meta','uv','json','llae' }

local extlibs = {
	require 'lua',
	require 'yajl',
	require 'libuv',
	require 'mbedtls',
}

function _M.extract_zip( zip_file, extlibs_folder )
	assert(zip.extract(zip_file, extlibs_folder))
end

function _M.extract_tar_gz( zip_file, extlibs_folder )
	local cmd = 'tar -xzf ' .. zip_file .. ' -C ' .. extlibs_folder
	assert(os.execute(cmd))
end

function _M.download(  )
	local extlibs_folder = path.join(_M.root,'build','extlibs')
	os.mkdir(extlibs_folder)
	for _,ext in ipairs(extlibs) do
		if ext.url then
			local zip_file = path.join(_M.root,'build','extlibs',ext.name..'-'..ext.version) .. '.' .. ext.archive 
			if not os.isfile(zip_file) then
				local res,code = http.download(ext.url,
					zip_file,
					{})
				if res ~= 'OK' then
					error('failed download ' .. ext.name .. ' ' .. res)
				end
			end
			_M['extract_'..string.gsub(ext.archive,'%.','_')](zip_file,extlibs_folder)
		end
	end
end

function _M.dependencies(  )
	for _,ext in ipairs(extlibs) do
		ext.lib(_M.root)
	end
end

function _M.lib(  )
	project 'llae-lib'
		kind 'StaticLib'
		location 'build'
		targetdir 'lib'

		_M.compile()
		
		local fls = {}
		for _,c in ipairs(components) do
			table.insert(fls,path.join(_M.root,'src',c,'*.cpp'))
			table.insert(fls,path.join(_M.root,'src',c,'*.h'))
		end
		files(fls)

end

function _M.compile(  )
	includedirs{
		path.join(_M.root, 'build','include'),
		path.join(_M.root, 'src') 
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
	for _,ext in ipairs(extlibs) do
		ext.link()
	end
	links {
		'llae-lib'
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