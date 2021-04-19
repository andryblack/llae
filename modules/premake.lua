name = 'premake'
version = '5.0.0-alpha16'
dir = name .. '-' .. version .. '-src'
archive = dir .. '.zip'
url = 'https://github.com/premake/premake-core/releases/download/v'..version..'/'..archive
hash = 'ba98e737ab2d148cbc13b068ac8ccca4'

function install()
end

function bootstrap()
	download(url,archive,hash)
	unpack_zip(archive)

	shell [[

PLATFORM=`uname -s`

if [ "$PLATFORM" = "Darwin" ]; then
    xcode-select --install
    PLATFORM=macosx
fi
if [ "$PLATFORM" = "Linux" ]; then
    PLATFORM=linux
fi

make -C <%= dir %>/build/gmake2.$PLATFORM config=release

	]]

	install_bin(dir .. '/bin/release/premake5')
end
