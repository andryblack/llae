name = 'premake'
version = '5.0.0-alpha14'

function install()
	download_git('https://github.com/premake/premake-core.git',{tag=version})
	shell [[

PLATFORM=`uname -s`

if [ "$PLATFORM" = "Darwin" ]; then
    xcode-select --install
    PLATFORM=osx
fi
if [ "$PLATFORM" = "Linux" ]; then
    PLATFORM=linux
fi

make -f Bootstrap.mak $PLATFORM

	]]
end