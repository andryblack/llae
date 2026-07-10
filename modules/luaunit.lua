
name = 'luaunit'
version = 'main'
url = 'https://github.com/bluebird75/luaunit.git'
dir =  name .. '-' .. version 

git_source {
	url = url,
	branch = version,
	dir = dir,
}

function install()
	download_git()
	install_script(dir..'/luaunit.lua','luaunit.lua') 
end

