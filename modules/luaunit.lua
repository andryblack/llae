
name = 'luaunit'
version = 'master'
url = 'https://github.com/bluebird75/luaunit.git'
dir =  name .. '-' .. version 

function install()
	download_git(url,{branch=version,dir=dir})
	install_script(dir..'/luaunit.lua','luaunit.lua') 
end

