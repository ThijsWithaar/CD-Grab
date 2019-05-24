include (FindPackageHandleStandardArgs)

find_path(SAMPLERATE_INCLUDE_DIR NAMES "samplerate.h")
find_library(SAMPLERATE_LIBRARY NAMES samplerate)

add_library(Samplerate::Samplerate UNKNOWN IMPORTED)
set_target_properties(Samplerate::Samplerate PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SAMPLERATE_INCLUDE_DIR}")
set_target_properties(Samplerate::Samplerate PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${SAMPLERATE_LIBRARY}")

mark_as_advanced(SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARY)
find_package_handle_standard_args (Samplerate DEFAULT_MSG SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARY)
