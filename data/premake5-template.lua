<% for _,mod in project:foreach_module() do %>
<% if mod.premake_setup then %>
-- module <%= mod.name %> / <%= get_relative(mod.location) %>
-- premake_setup >>>>>>
<%= template.compile(mod.premake_setup,{env=...})(mod) %>
-- premake_setup <<<<<<
<% end %>
<% end %>

<% macos_version = project:get_global_config('macos_version') or '10.15' %>
solution '<%= project:name() %>'

	configurations { 'debug', 'release' }

	language 'c++'
	cppdialect "C++17"

	objdir 'objects' 
	
	filter{'system:macosx','gmake'}
		buildoptions { "-mmacosx-version-min=<%= macos_version %>" }
   		linkoptions  { "-mmacosx-version-min=<%= macos_version %>" }
   	filter{}
   	xcodebuildsettings{
   		MACOSX_DEPLOYMENT_TARGET='<%= macos_version %>'
   	}
	filter{ 'configurations:debug'}
		symbols "On"
	filter{ 'configurations:release'}
		optimize 'Speed'
		symbols 'Off'
		visibility 'Hidden'
	filter{}

	
	filter{'action:xcode4'}
	location '../project'
	filter{}

<% for _,mod in project:foreach_module() do %>
<% if mod.solution then %>
	-- module <%= mod.name %> / <%= get_relative(mod.location) %>
	-- solution
	<%= template.compile(mod.solution,{env=...})(mod) %>
<% end %>
<% end %>
<% local function make_path(mod,first,...)
		assert(not path.isabsolute(first))
		local t = {
			get_relative(mod.location),
			first
		}
		for _,v in ipairs(table.pack(...)) do
			table.insert(t, v )
		end
		return "'" .. table.concat(t,'/') .. "'"
	end
%>

<% if project:get_premake() and project:get_premake().solution then %>
	-- project premake solution
	<%= template.compile(project:get_premake().solution,{env=...})() %>
	----------------------
<% end %>

	<% for _,mod in project:foreach_module() do %>
	-- module <%= mod.name %> / <%= get_relative(mod.location) %>
		
		
		<% if mod.build_lib then %>
	project "module-<%= mod.name %>"
		kind 'StaticLib'
		targetdir 'lib'
		filter{'action:gmake or gmake2'}
			location 'project'
		filter{}
<%= template.compile(mod.build_lib.project,{name=mod.name .. ':build_lib', env=...}){
				module = mod,
				lib = mod.build_lib,
				format_mod_file = function(m,...)
					local m = m:get_env()
					return make_path(m,m.dir,...)
				end,
				format_file = function (...)
					return make_path(mod,...)
				end  
			} %>
		<% end %>
	-- end module <%= mod.name %>
	<% end %>


	project '<%= project:name() %>'
<% if project:get_premake() and project:get_premake().kind then %>
		kind '<%= project:get_premake().kind %>'
<% else %>
		kind 'ConsoleApp'
<% end %>
		targetdir '../bin'
		targetname '<%= project:name() %>'
		filter{'action:gmake or gmake2'}
			location 'project'
		filter{}


		externalincludedirs {
			'include'
		}

		libdirs {
			'lib'
		}

		files {
			'src/*.cpp',--generated
		}



		externalincludedirs {
			<% for _,mod in project:foreach_module() do if mod.includedir then %>
				'<%= project.get_path(path.join('modules',mod.name),utils.replace_tokens(mod.includedir,mod)) %>',<%  
				elseif mod.includedirs then
					for __,idir in ipairs(mod.includedirs) do %>'<%= project.get_path(path.join('modules',mod.name),utils.replace_tokens(idir,mod)) %>',<% end
				end
			end %>
		}
		
		<% for _,mod in project:foreach_module() do if mod.project_main then %>
				<%= template.compile(mod.project_main,{env=...}){
					module = mod,
					format_file = function (...)
						return make_path(mod,...)
					end  
				} %> <% end end %>

		links {
		<% for _,mod in project:foreach_module_rev() do %>
			<% if mod.build_lib and not mod.build_lib.noautolink then %>"module-<%= mod.name %>",<%end%>
			<% if mod.libs then for __,l in ipairs(mod.libs) do %>"<%= l %>",<%end end%>
		<% end %>
		}

<% if project:get_premake() and project:get_premake().project then %>
	-- project premake project
	<%= template.compile(project:get_premake().project,{env=...}){
		format_file = function (first,...)
			assert(not path.isabsolute(first))
			local t = {
				'..',
				first
			}
			for _,v in ipairs(table.pack(...)) do
				table.insert(t, v )
			end
			return "'" .. table.concat(t,'/') .. "'"
		end  
	} %>
	------
<% end %>
		

