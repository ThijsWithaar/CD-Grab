project(cdGrab LANGUAGES CXX VERSION 0.1)

set(CDGRAB_ResamplerLib Sox CACHE STRING "Resampling library to use")
set_property(CACHE CDGRAB_ResamplerLib PROPERTY STRINGS "Sox" "Samplerate")

include(debian)
include(vcpkg)

vcpkg_add_package(boost-beast boost-crc boost-filesystem boost-format boost-serialization boost-system)
find_package(Boost QUIET REQUIRED COMPONENTS filesystem serialization system)

vcpkg_add_package(libflac)
debian_add_package(libflac-dev)
find_package(Flac REQUIRED)

vcpkg_add_package(opus)
debian_add_package(libopus-dev)
find_package(Opus REQUIRED)

if(CDGRAB_ResamplerLib STREQUAL "Samplerate")
	debian_add_package(libsamplerate0-dev)
	find_package(Samplerate REQUIRED)
elseif(CDGRAB_ResamplerLib STREQUAL "Sox")
	debian_add_package(libsoxr-dev)
	find_package(Soxr REQUIRED)
endif()

find_package(Threads)


file(GLOB SRC CONFIGURE_DEPENDS *.cc *.h)
file(GLOB SRC_FIL CONFIGURE_DEPENDS file/*.cc file/*.h)
file(GLOB SRC_FUJI CONFIGURE_DEPENDS MtFuji/*.cc MtFuji/*.h)
file(GLOB SRC_LIN CONFIGURE_DEPENDS linux/*.cc linux/*.h)
file(GLOB SRC_WIN CONFIGURE_DEPENDS windows/*.cc windows/*.h)
if (WIN32)
	set(SRC_OS ${SRC_WIN})
elseif(UNIX)
	set(SRC_OS ${SRC_LIN})
endif()

add_library(${PROJECT_NAME} STATIC
	${SRC} ${SRC_FIL} ${SRC_FUJI} ${SRC_OS}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_GRABCD)

if (WIN32)
	target_include_directories(${PROJECT_NAME} PUBLIC windows)
	target_compile_definitions(${PROJECT_NAME} PUBLIC -DNOMINMAX -D_WIN32_WINNT=0x0501)
else()
	target_include_directories(${PROJECT_NAME} PUBLIC linux)
endif()

target_include_directories(${PROJECT_NAME}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(${PROJECT_NAME}
	PUBLIC
		Boost::boost
		Boost::filesystem
		Boost::serialization
		Boost::system
		FLAC::FLAC
		Opus::opus
		Threads::Threads
)


if(CDGRAB_ResamplerLib STREQUAL "Samplerate")
	target_compile_definitions(${PROJECT_NAME} PRIVATE OPUS_USE_SAMPLERATE)
	target_link_libraries(${PROJECT_NAME} PRIVATE Samplerate::Samplerate)
elseif(CDGRAB_ResamplerLib STREQUAL "Sox")
	target_compile_definitions(${PROJECT_NAME} PRIVATE OPUS_USE_SOXR)
	target_link_libraries(${PROJECT_NAME} PRIVATE Sox::Soxr)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT lib)