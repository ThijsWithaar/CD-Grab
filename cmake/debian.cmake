# Allows installing of packages during configuration
# By default it is disabled, using the DEBIAN_INSTALL_DEPENDENCIES option
#
# Example:
# 	debian_add_package(libboost-dev)
#	find_package(Boost QUIET REQUIRED COMPONENTS program_options


function(is_debian ret)
	if(EXISTS /etc/debian_version)
		set(${ret} TRUE PARENT_SCOPE)
	else()
		set(${ret} FALSE PARENT_SCOPE)
	endif()
endfunction(is_debian)


function(debian_add_package)
	option(DEBIAN_INSTALL_DEPENDENCIES "Install 3rd party libs using apt-get" OFF)
	is_debian(isdeb)
	if(isdeb AND DEBIAN_INSTALL_DEPENDENCIES STREQUAL ON)
		foreach(name ${ARGN})
			execute_process(COMMAND "dpkg" "-s" "${name}" OUTPUT_FILE /dev/null ERROR_FILE /dev/null RESULT_VARIABLE ret)
			if(ret EQUAL "0")
				#message("${name} is installed (retcode ${ret})")
			else()
				message("Installing ${name}:")
				execute_process(COMMAND "sudo" "apt-get" "install" "${name}")
			endif()
		endforeach()
	endif(isdeb AND DEBIAN_INSTALL_DEPENDENCIES STREQUAL ON)
endfunction(debian_add_package)
