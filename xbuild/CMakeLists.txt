project(xbuild)

add_executable(xbuild xbuild.cc)

target_link_libraries(xbuild PUBLIC "-static")

set_target_properties(xbuild PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
)


add_custom_target(xbuild.docker
	COMMAND docker build -f Dockerfile -t thijswithaar/raspbian:xbuild .
	COMMAND docker push thijswithaar/raspbian:xbuild
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	DEPENDS xbuild
	COMMENT "Building Qemu docker image"
)
