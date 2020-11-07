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
		<% if mod.solution then %>
			-- solution
			<%= template.compile(mod.solution,{env=...})(mod) %>
		<% end %>
		<% if mod.build_lib then %>
			project "module-<%= mod.name %>"
			kind 'StaticLib'
			targetdir 'lib'
			location 'project'
			files{
				<% for _,f in ipairs(mod.build_lib.files) do %>
					"modules/<%= mod.name %>/<%= f %>",<% end %>
			}
			<%= template.compile(mod.build_lib.project,{env=...})(mod) %>
		<% end %>
	-- end module <%= mod.name %>
	<% end %>


	project '<%= project:name() %>'
		kind 'ConsoleApp'
		targetdir 'bin'
		location 'project'
		targetname '<%= project:name() %>'
		
		llae.exe()

