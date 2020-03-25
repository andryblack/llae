
local _M = {
	name = 'llae-libuv',
	version = 'v1.35.0',
	url = 'https://dist.libuv.org/dist/v1.35.0/libuv-v1.35.0.tar.gz',
	archive = 'tar.gz',
}

function _M.lib(  )
	_M.root = path.join(root,'build','extlibs','libuv-'.._M.version)
	os.mkdir(path.join(root,'build','include','uv'))
	assert(os.copyfile(path.join(_M.root,'include','uv.h'),
			path.join(root,'build','include')))
	for _,f in ipairs(os.matchfiles(path.join(_M.root,'include','uv','*'))) do
		assert(os.copyfile(f,
			path.join(root,'build','include','uv',path.getname(f))))
	end
	project(_M.name)
		kind 'StaticLib'
		location 'build'
		targetdir 'lib'
		
		includedirs {
			path.join(_M.root,'include'),
			path.join(_M.root,'src'),
		}
		files{
			path.join(_M.root,'src','*.c'),
			path.join(_M.root,'src','*.h'),
		}

		filter "system:linux or macosx"
			defines{
				'_LARGEFILE_SOURCE',
				'_FILE_OFFSET_BITS=64',

			}
			files {
				path.join(_M.root,'src','unix','async.c'),
				path.join(_M.root,'src','unix','loop.c'),
				path.join(_M.root,'src','unix','fs.c'),
				path.join(_M.root,'src','unix','core.c'),
				path.join(_M.root,'src','unix','pipe.c'),
				path.join(_M.root,'src','unix','poll.c'),
				path.join(_M.root,'src','unix','signal.c'),
				path.join(_M.root,'src','unix','stream.c'),
				path.join(_M.root,'src','unix','thread.c'),
				path.join(_M.root,'src','unix','process.c'),
				path.join(_M.root,'src','unix','proctitle.c'),
				path.join(_M.root,'src','unix','tcp.c'),
				path.join(_M.root,'src','unix','udp.c'),
				path.join(_M.root,'src','unix','tty.c'),
				path.join(_M.root,'src','unix','getaddrinfo.c'),
				path.join(_M.root,'src','unix','loop-watcher.c'),
			}
		filter "system:macosx"
			defines {
				'_DARWIN_USE_64_BIT_INODE=1',
				'_DARWIN_UNLIMITED_SELECT=1'
			}
			files {
				path.join(_M.root,'src','unix','darwin.c'),
				path.join(_M.root,'src','unix','fsevents.c'),
            	path.join(_M.root,'src','unix','darwin-proctitle.c'),
            	path.join(_M.root,'src','unix','random-getentropy.c'),
            	path.join(_M.root,'src','unix','bsd-ifaddrs.c'),
            	path.join(_M.root,'src','unix','kqueue.c'),
			}
		filter {}
end

function _M.link(  )
	links{ _M.name }
end

return _M