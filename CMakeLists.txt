cmake_minimum_required(VERSION 3.10)

# Cmake needs a project name, otherwise Qt linking with winMain goes wrong...
project(CdGrabProject LANGUAGES CXX VERSION 0.1)

option(BUILD_SHARED_LIBS "Build shared library" ON)

if(EXISTS "/etc/debian_version")
	set(DEBIAN TRUE CACHE BOOL "Enable debian specific options: 64-bit lib name and .deb generation")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(sanitizers)


enable_testing()

add_subdirectory(libCdGrab)
add_subdirectory(gui)
add_subdirectory(test)


export(TARGETS
	cdGrab
	cdGrabUi
	FILE "${PROJECT_BINARY_DIR}/CdGrabTargets.cmake"
)

set(CPACK_PACKAGE_NAME "CdGrab")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Audio-CD grabber, with error detection")
set(CPACK_PACKAGE_CONTACT "thijs.withaar@gmail.com")
if(UNIX)
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libopus libflac libsoxr qt5 libboost-filesystem (>=1.67.0)")
	if(DEBIAN)
		set(CPACK_GENERATOR "DEB")
	else()
		set(CPACK_GENERATOR "RPM")
	endif()
endif()

include(CPack)
