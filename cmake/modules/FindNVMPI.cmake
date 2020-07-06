find_path(NVMPI_INCLUDE_DIR NAMES NvVideoDecoder.h PATHS /usr/src/jetson_multimedia_api/include )
find_library(NVMPI_LIBRARY NAMES libnvbuf_utils nvbuf_utils PATH_SUFFIXES tegra)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NVMPI REQUIRED_VARS 
	NVMPI_INCLUDE_DIR
	NVMPI_LIBRARY)

if(NVMPI_FOUND)
  set(NVMPI_INCLUDE_DIRS ${NVMPI_INCLUDE_DIR})
  set(NVMPI_LIBRARIES ${NVMPI_LIBRARY} 
      CACHE STRING "nvmpi libraries" FORCE)
  list(APPEND NVMPI_DEFINITIONS -DHAVE_NVMPI=1 -DHAS_NVMPI=1)

  if(NOT TARGET NVMPI::NVMPI)
    add_library(NVMPI::NVMPI UNKNOWN IMPORTED)
    set_target_properties(NVMPI::NVMPI PROPERTIES
                                     IMPORTED_LOCATION "${NVMPI_LIBRARIES}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${NVMPI_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(NVMPI_INCLUDE_DIRS NVMPI_LIBRARIES)
