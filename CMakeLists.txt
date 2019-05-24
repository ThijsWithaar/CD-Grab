cmake_minimum_required(VERSION 3.12)

# Cmake needs a project name, otherwise Qt linking with winMain goes wrong...
project(CdGrabProject LANGUAGES CXX)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(sanitizers)

option(BUILD_SHARED_LIBS "Build shared library" ON)

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
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsoxr qt5 libboost")
	set(CPACK_GENERATOR "DEB")
endif()

include(CPack)
