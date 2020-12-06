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
	cppdialect "C++11"

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

	<% for _,mod in project:foreach_module() do %>
	-- module <%= mod.name %> / <%= mod.location %>
		
		
		<% if mod.build_lib then %>
			project "module-<%= mod.name %>"
			kind 'StaticLib'
			targetdir 'lib'
			location 'project'
			<%= template.compile(mod.build_lib.project,{env=...}){
				module = mod,
				lib = mod.build_lib,
				format_file = function (...)
					return make_path(mod,...)
				end  
			} %>
		<% end %>
	-- end module <%= mod.name %>
	<% end %>


	project '<%= project:name() %>'
		kind 'ConsoleApp'
		targetdir '../bin'
		location 'project'
		targetname '<%= project:name() %>'

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
			<% if mod.build_lib then %>"module-<%= mod.name %>",<%end%>
		<% end %>
		}


		

