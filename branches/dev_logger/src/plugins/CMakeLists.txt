set(ALL_PLUGINS "")

include("plugin_list.cmake")

list(REMOVE_DUPLICATES ALL_PLUGINS)

# Check dependencies
foreach(plugin ${ALL_PLUGINS})
	if (PLUGIN_${plugin})
		foreach(dep ${${plugin}_DEPENDENCIES})
			if (NOT PLUGIN_${dep})
				message(WARNING "Plugin '${plugin}' requires '${dep}', but it is disabled. Please, set PLUGIN_${dep} to ON, unless you know what you are doing.")
			endif (NOT PLUGIN_${dep})
		endforeach(dep)
	endif (PLUGIN_${plugin})
endforeach(plugin)
