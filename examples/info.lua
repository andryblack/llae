local os = require 'llae.os'
local uv = require 'llae.uv'

print('homedir',os.homedir())
print('tmpdir',os.tmpdir() )
print('hostname',os.hostname())
local u = assert(os.uname())
print('sysname',u.sysname)
print('release',u.release)
print('version',u.version)
print('machine',u.machine)

for _,interface in ipairs(uv.interface_addresses()) do
	print('interface:',interface.name, interface.internal and 'internal' or 'external')
	print('','address:',interface.address)
	print('','netmask:',interface.netmask)
end