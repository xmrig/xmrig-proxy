find_path(EREDIS_INCLUDE_DIR NAMES eredis.h)
find_library(EREDIS_LIBRARY NAMES liberedis.so liberedis.dylib)

set(EREDIS_LIBRARIES ${EREDIS_LIBRARY})
set(EREDIS_INCLUDE_DIRS ${EREDIS_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EREDIS DEFAULT_MSG EREDIS_LIBRARY EREDIS_INCLUDE_DIR)
