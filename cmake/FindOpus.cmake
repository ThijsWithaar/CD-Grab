include (FindPackageHandleStandardArgs)

find_path(OPUS_INCLUDE_DIR NAMES "opus/opus.h")
find_library(OPUS_LIBRARY NAMES opus)

add_library(Opus::opus UNKNOWN IMPORTED)
set_target_properties(Opus::opus PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OPUS_INCLUDE_DIR}")
set_target_properties(Opus::opus PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${OPUS_LIBRARY}")

mark_as_advanced(OPUS_INCLUDE_DIR OPUS_LIBRARY)
find_package_handle_standard_args (Opus DEFAULT_MSG OPUS_INCLUDE_DIR OPUS_LIBRARY)
