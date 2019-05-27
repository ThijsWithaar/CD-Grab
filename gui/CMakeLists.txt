
project(cdGrabUi LANGUAGES CXX DESCRIPTION "Gui for grabbing of audio CD's")

vcpkg_add_package(qt5)
debian_add_package(qtbase5-dev qttools5-dev)
find_package(Qt5 REQUIRED Core Gui Widgets LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON) # For linguist translations

include(GitVersion)
configure_file(version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

file(GLOB SRC CONFIGURE_DEPENDS *.cc *.h)
file(GLOB QTUI CONFIGURE_DEPENDS *.ui)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} dutch.ts)
configure_file(translations.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

message("CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR}")
message("QM_FILES : ${QM_FILES}")


add_executable(${PROJECT_NAME} WIN32
	${SRC}
	${QTUI}
	${QM_FILES}
	${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
	${CMAKE_CURRENT_BINARY_DIR}/version.cpp
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
