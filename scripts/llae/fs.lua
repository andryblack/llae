local uv = require 'uv'

local fs = {}

function fs.mkdir( dir )
	file.mkdir(dir)
end

function fs.copy( from , to )
	file.copyfile(from, to)
end

function fs.stat( p )
	return file.stat(p)
end

function fs.scandir( p )
	return file.scandir(p)
end

function fs.open( p )
	return file.open(p)
end

function fs.home(  )
	return '$HOME'
end

return fs