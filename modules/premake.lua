name = 'premake'
version = '5.0.0-beta1'
dir = name .. '-' .. version 
archive = dir .. '.tar.gz'
url = 'https://github.com/premake/premake-core/archive/refs/tags/v'..version..'.tar.gz'
hash = '56a4a6ddb40acbe099e7bd07118d2c5d'

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

	shell [[

PLATFORM=`uname -s`

if [ "$PLATFORM" = "Darwin" ]; then
    xcode-select --install
    PLATFORM=osx
fi
if [ "$PLATFORM" = "Linux" ]; then
    PLATFORM=linux
fi

make -C <%= dir %> -f Bootstrap1.mak $PLATFORM

	]]

	install_bin(dir .. '/bin/release/premake5')
end
