name = 'premake'
version = 'v5.0.0-beta1'
dir = name .. '-' .. version 
archive = dir .. '.tar.gz'
url = 'https://github.com/premake/premake-core/archive/refs/tags/'..version..'.tar.gz'
hash = '56a4a6ddb40acbe099e7bd07118d2c5d'

function install()

end


function bootstrap()
	download(url,archive,hash)
	unpack_tgz(archive,dir,1)

	shell [[

PLATFORM=`uname -s`

if [ "$PLATFORM" = "Darwin" ]; then
    xcode-select --install
    PLATFORM=osx
fi
if [ "$PLATFORM" = "Linux" ]; then
    PLATFORM=linux
fi

make -C <%= dir %> -f Bootstrap.mak $PLATFORM

	]]

	install_bin(dir .. '/bin/release/premake5')
end
