name = 'premake'
version = '5.0.0-beta2'
dir = name .. '-' .. version 
archive = dir .. '.tar.gz'
url = 'https://github.com/premake/premake-core/archive/refs/tags/v'..version..'.tar.gz'
hash = '887b0bd36fcb58f9d67f7e299a408ad6'

function install()

end


function bootstrap()
	download(url,archive,hash)
	unpack_tgz(archive,dir,1)

	preprocess{
		insource = true,
		src =  dir .. '/Bootstrap.mak',
		dst = dir .. '/Bootstrap1.mak',
		-- replace_line = {
		-- 	[ [[	./build/bootstrap/premake_bootstrap --arch=$(PLATFORM) --to=build/bootstrap gmake2]] ]=
		-- 		[[	./build/bootstrap/premake_bootstrap --arch=$(PLATFORM) --to=build/bootstrap gmake2 --no-zlib=true --no-luasocket=true --no-curl=true]]
		-- }
	}

	preprocess{
		insource = true,
		remove_src = true,
		src = dir .. '/contrib/libzip/mkstemp.c',
		dst = dir .. '/contrib/libzip/mkstemp1.c',
		insert_before = {
			['#ifndef O_BINARY'] = [[

#ifdef __APPLE__
#include <unistd.h>
#endif


			]]
		}
	}

	preprocess{
		insource = true,
		remove_src = true,
		src = dir .. '/contrib/libzip/premake5.lua',
		dst = dir .. '/contrib/libzip/premake51.lua',
		replace_line = {
			['	filter "system:macosx"'] = [[

	filter "system:macosx"
			defines{ 'HAVE_UNISTD_H' }

			]]
		}
	}

	preprocess{
		insource = true,
		remove_src = true,
		src = dir .. '/contrib/libzip/premake51.lua',
		dst = dir .. '/contrib/libzip/premake5.lua',
	}


	local PLATFORM = exec_res('uname',{'-s'})
	if string.match(PLATFORM,'Darwin.*') then
		exec{
			bin = 'xcode-select',
			args = {'--install'}
		}
		PLATFORM = 'osx'
	elseif string.match(PLATFORM,"Linux.*") then
		PLATFORM = 'linux'
	end
	assert(exec{
		bin = 'make',
		args = {'-C',get_absolute_location(dir),'-f','Bootstrap1.mak',PLATFORM},
		name = 'build_premake5'
	})

	install_bin(dir .. '/bin/release/premake5')
end
