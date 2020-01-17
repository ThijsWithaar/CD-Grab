project(nrg2dts LANGUAGES CXX DESCRIPTION "Extract DTS files from an image" VERSION 0.1)

find_package(Boost REQUIRED COMPONENTS iostreams program_options)

file(GLOB SRC CONFIGURE_DEPENDS *.cc *.h)


add_executable(${PROJECT_NAME}
	${SRC}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		cdGrab
		Boost::iostreams
		Boost::program_options
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT ui)
