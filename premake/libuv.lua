local utils = require 'utils'

local _M = {
	name = 'libuv',
	version = 'v1.42.0',
	url = 'https://dist.libuv.org/dist/v1.42.0/libuv-v1.42.0.tar.gz',
	archive = 'tar.gz',
}

function _M.lib( root )
	_M.root = path.join(root,'build','extlibs','libuv-'.._M.version)
	
	utils.preprocess(
		path.join(_M.root,'include','uv.h'),
		path.join(root,'build','include','uv.h'),
		{replace={
			['UV_EXTERN'] = '',
		}})
	for _,f in ipairs(os.matchfiles(path.join(_M.root,'include','uv','*'))) do
		utils.install_header(f,path.join('uv',path.getname(f)))
	end
	project('llae-'.._M.name)
		kind 'StaticLib'
		targetdir 'lib'
		location 'build/project'
		
		includedirs {
			path.join(root,'build','include'),
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
		filter "system:linux"
			defines {
				'_POSIX_C_SOURCE=200112',
				'_GNU_SOURCE',
			}
			files {
				path.join(_M.root,'src','unix','linux-core.c'),
				path.join(_M.root,'src','unix','linux-inotify.c'),
				path.join(_M.root,'src','unix','linux-syscalls.c'),
				path.join(_M.root,'src','unix','linux-syscalls.h'),
				path.join(_M.root,'src','unix','procfs-exepath.c'),
				path.join(_M.root,'src','unix','random-getrandom.c'),
				path.join(_M.root,'src','unix','random-sysctl-linux.c'),
				path.join(_M.root,'src','unix','sysinfo-loadavg.c'),
				path.join(_M.root,'src','unix','epoll.c')
			}
		filter "system:windows"
			defines {
				'WIN32_LEAN_AND_MEAN',
				'_WIN32_WINNT=0x0601'
			}
			
			files {
				path.join(_M.root,'src','win','async.c'),
				path.join(_M.root,'src','win','core.c'),
				path.join(_M.root,'src','win','detect-wakeup.c'),
				path.join(_M.root,'src','win','dl.h'),
				path.join(_M.root,'src','win','error.c'),
				path.join(_M.root,'src','win','fs.c'),
				path.join(_M.root,'src','win','fs-event.c'),
				path.join(_M.root,'src','win','getaddrinfo.c'),
				path.join(_M.root,'src','win','getnameinfo.c'),
				path.join(_M.root,'src','win','handle.c'),
				path.join(_M.root,'src','win','loop-watcher.c'),
				path.join(_M.root,'src','win','pipe.c'),
				path.join(_M.root,'src','win','thread.c'),
				path.join(_M.root,'src','win','poll.c'),
				path.join(_M.root,'src','win','process.c'),
				path.join(_M.root,'src','win','process-stdio.c'),
				path.join(_M.root,'src','win','signal.c'),
				path.join(_M.root,'src','win','snprintf.c'),
				path.join(_M.root,'src','win','stream.c'),
				path.join(_M.root,'src','win','tcp.c'),
				path.join(_M.root,'src','win','tty.c'),
				path.join(_M.root,'src','win','udp.c'),
				path.join(_M.root,'src','win','util.c'),
				path.join(_M.root,'src','win','winapi.c'),
				path.join(_M.root,'src','win','winsock.c'),
			}
		filter {}
end

function _M.link(  )
	filter "system:linux"
		links{ 'pthread' }
	filter "system:windows"
		links{ 'ws2_32','userenv','iphlpapi','psapi' }
	filter {}
	links{ 'llae-'.._M.name }
end

return _M
