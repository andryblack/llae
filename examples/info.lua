local os = require 'llae.os'

print('homedir',os.homedir())
print('tmpdir',os.tmpdir() )
print('hostname',os.hostname())
local u = assert(os.uname())
print('sysname',u.sysname)
print('release',u.release)
print('version',u.version)
print('machine',u.machine)