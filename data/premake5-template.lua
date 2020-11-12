solution '<%= project:name() %>'

	configurations { 'debug', 'release' }

	language 'c++'
	cppdialect "C++11"

	objdir 'objects' 
	
	configuration{ 'debug'}
		symbols "On"
	configuration{ 'release'}
		optimize 'Speed'
		symbols 'Off'
		visibility 'Hidden'
	configuration{}


	<% for _,mod in project:foreach_module() do %>
	-- module <%= mod.name %> / <%= mod.location %>
		<% local function make_path(components)
				local t = {
					"path.join('modules'",
					"'" .. mod.name .. "'"
				}
				for _,v in ipairs(components) do
					table.insert(t,"'" .. v .. "'")
				end
				return table.concat(t,',') .. ')'
			end
		%>
		<% if mod.solution then %>
			-- solution
			<%= template.compile(mod.solution,{env=...})(mod) %>
		<% end %>
		<% if mod.build_lib then %>
			project "module-<%= mod.name %>"
			kind 'StaticLib'
			targetdir 'lib'
			location 'project'
			<%= template.compile(mod.build_lib.project,{env=...}){
				module = mod,
				lib = mod.build_lib,
				format_file = function (...)
					return make_path(table.pack(...))
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

		sysincludedirs {
			'include'
		}
		includedirs {
			<% for _,mod in project:foreach_module() do if mod.includedir then %>
				'modules/<%=mod.name%>/<%= mod.includedir %>'<% end end %>
		}
		files {
			'src/**.cpp',
		}

		links {
		<% for _,mod in project:foreach_module() do %>
			<% if mod.build_lib then %>"module-<%= mod.name %>",<%end%>
		<% end %>
		}


		

