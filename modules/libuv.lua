
name = 'libuv'
version = 'v1.51.0'
archive = name .. '-' .. version .. '.tar.gz'
url = 'https://dist.libuv.org/dist/'..version..'/' .. archive
hash = '5e0109e19c3fed3a8cbecb958de39afa'
dir = name .. '-' .. version

function install()
	download(url,archive,hash)
	unpack_tgz(archive)

	isolate(dir .. '/include/uv',{'*.h'},{
		['build/modules/libuv/'.. dir .. '/include'] = 'llae-private/'
	})
	install_files {
		['build/include/llae-private/uv.h'] = dir .. '/include/uv.h', 
		['build/include/llae-private/uv/aix.h'] = dir .. '/include/uv/aix.h',
		['build/include/llae-private/uv/bsd.h'] = dir .. '/include/uv/bsd.h',
		['build/include/llae-private/uv/darwin.h'] = dir .. '/include/uv/darwin.h',
		['build/include/llae-private/uv/errno.h'] = dir .. '/include/uv/errno.h', 
		['build/include/llae-private/uv/linux.h'] = dir .. '/include/uv/linux.h', 
		['build/include/llae-private/uv/posix.h'] = dir .. '/include/uv/posix.h', 
		['build/include/llae-private/uv/threadpool.h'] = dir .. '/include/uv/threadpool.h', 
		['build/include/llae-private/uv/tree.h'] = dir .. '/include/uv/tree.h', 
		['build/include/llae-private/uv/unix.h'] = dir .. '/include/uv/unix.h', 
		['build/include/llae-private/uv/version.h'] = dir .. '/include/uv/version.h', 
		['build/include/llae-private/uv/win.h'] = dir .. '/include/uv/win.h', 
	}
	isolate(dir .. '/src',{'*.c','*.h'},{
		['build/include/llae-private'] = 'llae-private/'
	})
	isolate(dir .. '/src/unix',{'*.c','*.h'},{
		['build/include/llae-private'] = 'llae-private/'
	})
	isolate(dir .. '/src/win',{'*.c','*.h'},{
		['build/include/llae-private'] = 'llae-private/'
	})

end


build_lib = {
	files = {},
	common_files = {'fs-poll.c','heap-inl.h','idna.c','inet.c','queue.h','random.c',
					'strscpy.c','strscpy.h','threadpool.c','timer.c','uv-common.c',
					'uv-common.h','uv-data-getter-setters.c','version.c', 'strtok.c'},
	unix_files = {'async.c','loop.c','fs.c','core.c','pipe.c','poll.c','signal.c',
					'stream.c','thread.c','process.c','proctitle.c','tcp.c','udp.c',
					'tty.c', 'getaddrinfo.c', 'loop-watcher.c', 'random-devurandom.c' },
	macosx_files = {'darwin.c','fsevents.c','darwin-proctitle.c','random-getentropy.c',
					'bsd-ifaddrs.c','kqueue.c'},
	linux_files = {'linux.c','procfs-exepath.c','proctitle.c','random-getrandom.c','random-sysctl-linux.c'},
	windows_files = {'async.c','core.c','detect-wakeup.c','dl.c','error.c','fs.c',
					'fs-event.c','getaddrinfo.c','getnameinfo.c','handle.c','loop-watcher.c',
					'pipe.c','thread.c','poll.c','process.c','process-stdio.c','signal.c',
					'snprintf.c','stream.c','tcp.c','tty.c','udp.c','util.c',
					'winapi.c','winsock.c'},
	project = [[
	files {
				<% for _,f in ipairs(lib.common_files) do %>
					<%= format_file(module.dir,'src',f) %>,<% end %>
			}
	includedirs{
		'include',
		<%= format_file(module.dir,'src') %>
	}
	filter "system:linux or macosx"
			defines{
				'_LARGEFILE_SOURCE',
				'_FILE_OFFSET_BITS=64',
			}
			files {
				<% for _,f in ipairs(lib.unix_files) do %>
					<%= format_file(module.dir,'src','unix',f) %>,<% end %>
			}
	filter "system:macosx"
			defines {
				'_DARWIN_USE_64_BIT_INODE=1',
				'_DARWIN_UNLIMITED_SELECT=1'
			}
			files {
				<% for _,f in ipairs(lib.macosx_files) do %>
					<%= format_file(module.dir,'src','unix',f) %>,<% end %>
			}
	filter "system:linux"
			defines {
				'_POSIX_C_SOURCE=200112',
				'_GNU_SOURCE',
			}
			files {
				<% for _,f in ipairs(lib.linux_files) do %>
					<%= format_file(module.dir,'src','unix',f) %>,<% end %>
			}
	filter "system:windows"
			defines {
				'WIN32_LEAN_AND_MEAN',
				'_WIN32_WINNT=0x0602',
			}
			files {
				<% for _,f in ipairs(lib.windows_files) do %>
					<%= format_file(module.dir,'src','win',f) %>,<% end %>
			}
	filter {}
]]
}

project_main = [[
	filter "system:linux"
		links{ 'pthread','dl' }
	filter "system:windows"
		links{ 'psapi','user32','advapi32','iphlpapi','userenv','ws2_32','dbghelp','ole32' }
	filter {}
]]
