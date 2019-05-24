
project(cdGrabUi LANGUAGES CXX DESCRIPTION "Gui for grabbing of audio CD's" VERSION 0.1)

vcpkg_add_package(qt5)
debian_add_package(qtbase5-dev)

find_package(Qt5 REQUIRED Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)


file(GLOB SRC CONFIGURE_DEPENDS *.cc *.h)
file(GLOB QTUI CONFIGURE_DEPENDS *.ui)

add_executable(${PROJECT_NAME} WIN32
	${SRC}
	${QTUI}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		Qt5::Gui
		Qt5::Widgets
		cdGrab
)

if(UNIX)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)
endif(UNIX)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT ui)

# Copy the .dlls from the vcpkg build as well
if(WIN32)
	install(
		DIRECTORY ${PROJECT_BINARY_DIR}/Release/
		DESTINATION bin
		COMPONENT ui
		FILES_MATCHING PATTERN "*.dll" PATTERN "*.conf"
	)
endif()
