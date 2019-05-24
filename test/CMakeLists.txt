project(CdGrabTest LANGUAGES CXX)

vcpkg_add_package(catch2)
find_package(Catch2 REQUIRED)

file(GLOB SRC CONFIGURE_DEPENDS *.cc *.h)

add_executable(${PROJECT_NAME}
	${SRC}
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		cdGrab
		Catch2::Catch2
)

include(Catch)
catch_discover_tests(${PROJECT_NAME})
