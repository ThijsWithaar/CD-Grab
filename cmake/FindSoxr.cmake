include (FindPackageHandleStandardArgs)

set(SOXR_PATH "C:/build/vcpkg/packages/soxr_x64-windows")

find_path(SOXR_INCLUDE_DIR NAMES "soxr.h" HINTS ${SOXR_PATH}/include)

find_library(SOXR_LIBRARY NAMES soxr HINTS ${SOXR_PATH}/lib)

add_library(Sox::Soxr UNKNOWN IMPORTED)
set_target_properties(Sox::Soxr PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SOXR_INCLUDE_DIR}")
set_target_properties(Sox::Soxr PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${SOXR_LIBRARY}")

mark_as_advanced(SOXR_INCLUDE_DIR SOXR_LIBRARY)
find_package_handle_standard_args (Soxr DEFAULT_MSG SOXR_INCLUDE_DIR SOXR_LIBRARY)
