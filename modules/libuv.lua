
name = 'libuv'
version = 'v1.35.0'
archive = name .. '-' .. version .. '.tar.gz'
url = 'https://dist.libuv.org/dist/'..version..'/' .. archive

dir = name .. '-' .. version

function install()
	download(url,archive)

	shell([[
tar -xzf ]] .. archive .. [[ || exit 1
	]])
	
	install_files {
		['build/include/uv.h'] = dir .. '/include/uv.h', 
	}
	
	install_files {
		['build/include/uv.h'] = dir .. '/include/uv.h', 
		['build/include/uv/aix.h'] = dir .. '/include/uv/aix.h',
		['build/include/uv/bsd.h'] = dir .. '/include/uv/bsd.h',
		['build/include/uv/darwin.h'] = dir .. '/include/uv/darwin.h',
		['build/include/uv/errno.h'] = dir .. '/include/uv/errno.h', 
		['build/include/uv/linux.h'] = dir .. '/include/uv/linux.h', 
		['build/include/uv/posix.h'] = dir .. '/include/uv/posix.h', 
		['build/include/uv/threadpool.h'] = dir .. '/include/uv/threadpool.h', 
		['build/include/uv/tree.h'] = dir .. '/include/uv/tree.h', 
		['build/include/uv/unix.h'] = dir .. '/include/uv/unix.h', 
		['build/include/uv/version.h'] = dir .. '/include/uv/version.h', 
		['build/include/uv/win.h'] = dir .. '/include/uv/win.h', 
	}
end


build_lib = {
	files = {},
	common_files = {'fs-poll.c','heap-inl.h','idna.c','inet.c','queue.h','random.c',
					'strscpy.c','strscpy.h','threadpool.c','timer.c','uv-common.c',
					'uv-common.h','uv-data-getter-setters.c','version.c'},
	unix_files = {'async.c','loop.c','fs.c','core.c','pipe.c','poll.c','signal.c',
					'stream.c','thread.c','process.c','proctitle.c','tcp.c','udp.c',
					'tty.c', 'getaddrinfo.c', 'loop-watcher.c' },
	macosx_files = {'darwin.c','fsevents.c','darwin-proctitle.c','random-getentropy.c',
					'bsd-ifaddrs.c','kqueue.c'},
	linux_files = {'linux-core.c','linux-inotify.c','linux-syscalls.c','linux-syscalls.h',
					'procfs-exepath.c','random-getrandom.c','random-sysctl-linux.c',
					'sysinfo-loadavg.c'},
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
	filter {}
]]
}

project_main = [[
	filter "system:linux"
		links{ 'pthread' }
	filter {}
]]