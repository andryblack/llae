name = 'llae'
version = 'develop'

function install()
	download_git('https://github.com/andryblack/llae.git',{branch=version})
	shell[[
cd src
${LLAE_PROJECT_ROOT}/bin/premake5${LLAE_EXE} download || exit 1
${LLAE_PROJECT_ROOT}/bin/premake5${LLAE_EXE} gmake
make -C build config=release
	]]
	install_bin('src/bin/llae')
end