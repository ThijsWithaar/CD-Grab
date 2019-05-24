set(VCPKG_ROOT "C:/build/vcpkg" CACHE PATH "Location of VCPKG package repository (https://github.com/microsoft/vcpkg)")

set(VCPKG_EXE ${VCPKG_ROOT}/vcpkg.exe)

if(WIN32)
	if(NOT EXISTS ${VCPKG_ROOT}/bootstrap-vcpkg.bat)
		#execute_process(COMMAND git clone https://github.com/Microsoft/vcpkg.git ${VCPKG_ROOT})
		#execute_process(COMMAND ${VCPKG_ROOT}/bootstrap-vcpkg.bat)
		#execute_process(COMMAND ./vcpkg integrate install)
	endif()
endif()


function(vcpkg_add_package)
	if(WIN32)
		foreach(packageName ${ARGN})
			# try to check first:
			execute_process(COMMAND ${VCPKG_EXE} "list" "${name}" OUTPUT_FILE null RESULT_VARIABLE ret)
			#execute_process(COMMAND vcpkg install ${packageName}:x64-windows)\
			#message("#VCPKG: ${packageName} ${ret}")
		endforeach()
	endif(WIN32)
endfunction(vcpkg_add_package )
