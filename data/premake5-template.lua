<% for _,mod in project:foreach_module() do %>
<% if mod.premake_setup then %>
-- module <%= mod.name %> / <%= mod.location %>
-- premake_setup >>>>>>
<%= template.compile(mod.premake_setup,{env=...})(mod) %>
-- premake_setup <<<<<<
<% end %>
<% end %>


solution '<%= project:name() %>'

	configurations { 'debug', 'release' }

	language 'c++'
	cppdialect "C++17"

	objdir 'objects' 
	
	filter{'system:macosx','gmake'}
		buildoptions { "-mmacosx-version-min=10.14" }
   		linkoptions  { "-mmacosx-version-min=10.14" }
   	filter{}

	configuration{ 'debug'}
		symbols "On"
	configuration{ 'release'}
		optimize 'Speed'
		symbols 'Off'
		visibility 'Hidden'
	configuration{}

	
	filter{'action:xcode4'}
	location '../project'
	filter{}

<% for _,mod in project:foreach_module() do %>
<% if mod.solution then %>
	-- module <%= mod.name %> / <%= mod.location %>
	-- solution
	<%= template.compile(mod.solution,{env=...})(mod) %>
<% end %>
<% end %>
<% local function make_path(mod,...)
		local t = {
			"modules",
			mod.name
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
	-- module <%= mod.name %> / <%= mod.location %>
		
		
		<% if mod.build_lib then %>
			project "module-<%= mod.name %>"
			kind 'StaticLib'
			targetdir 'lib'
			filter{'action:gmake or gmake2'}
				location 'project'
			filter{}
			<%= template.compile(mod.build_lib.project,{env=...}){
				module = mod,
				lib = mod.build_lib,
				format_mod_file = function(m,...)
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

		xcodebuildsettings{
   			MACOSX_DEPLOYMENT_TARGET='10.14'
   		}

		sysincludedirs {
			'include'
		}

		files {
			'src/*.cpp',--generated
		}



		includedirs {
			<% for _,mod in project:foreach_module() do if mod.includedir then %>
				'modules/<%=mod.name%>/<%= mod.includedir %>'<% end end %>
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
		<% end %>
		}

<% if project:get_premake() and project:get_premake().project then %>
	-- project premake project
	<%= template.compile(project:get_premake().project,{env=...}){
		format_file = function (...)
			local t = {
				'..'
			}
			for _,v in ipairs(table.pack(...)) do
				table.insert(t, v )
			end
			return "'" .. table.concat(t,'/') .. "'"
		end  
	} %>
	------
<% end %>
		

