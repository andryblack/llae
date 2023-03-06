
package.path = package.path .. ';scripts/?.lua'

local fs = require 'llae.fs'
local async = require 'llae.async'
local os = require 'llae.os'
local path = require 'llae.path'

for file_no=1,4 do
	async.run(function()

		local fn = path.join(os.tmpdir(),'test-write-' .. file_no .. '.bin')
		print('start write file',fn)
		fs.unlink(fn)
		local fw = assert(fs.open(fn,fs.O_WRONLY|fs.O_CREAT))
		local d = string.rep('0123456789ABCDEF',1024*64)
		for i=1,16 do
			local s = assert(fw:write(d,d,d,d))
			assert(s==1024*1024*4)
			print('writed',file_no)
		end
		fw:close()
		print('end write file')
		

		local fr = assert(fs.open(fn,fs.O_RDONLY))
		for i=1,16 do
			local ch = assert(fr:read(1024*1024))
			assert(ch==d)
			ch = assert(fr:read(1024*1024))
			assert(ch==d)
			ch = assert(fr:read(1024*1024))
			assert(ch==d)
			ch = assert(fr:read(1024*1024))
			assert(ch==d)
			print('readed',file_no)
		end
		fr:close()
		print('end read file')


		fs.unlink(fn)
	end)
end
