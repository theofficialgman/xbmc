set(SOURCES AddonVideoCodec.cpp
            DVDVideoCodec.cpp
            DVDVideoCodecFFmpeg.cpp)

set(HEADERS AddonVideoCodec.h
            DVDVideoCodec.h
            DVDVideoCodecFFmpeg.h)

if(NOT ENABLE_EXTERNAL_LIBAV)
  list(APPEND SOURCES DVDVideoPPFFmpeg.cpp)
  list(APPEND HEADERS DVDVideoPPFFmpeg.h)
endif()

if(NVMPI_FOUND)
  list(APPEND SOURCES NVV4L.cpp
                      nvidia/NvBuffer.cpp
                      nvidia/NvVideoDecoder.cpp
                      nvidia/NvVideoConverter.cpp
                      nvidia/NvUtils.cpp
                      nvidia/NvLogging.cpp
                      nvidia/NvElement.cpp
                      nvidia/NvV4l2Element.cpp
                      nvidia/NvV4l2ElementPlane.cpp
                      nvidia/NvElementProfiler.cpp)
  list(APPEND HEADERS NVV4L.h)
endif()

if(AML_FOUND)
  list(APPEND SOURCES AMLCodec.cpp
                      DVDVideoCodecAmlogic.cpp)
  list(APPEND HEADERS AMLCodec.h
                      DVDVideoCodecAmlogic.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES DXVA.cpp)
  list(APPEND HEADERS DXVA.h)
endif()

if(VDPAU_FOUND)
  list(APPEND SOURCES VDPAU.cpp)
  list(APPEND HEADERS VDPAU.h)
endif()

if(VAAPI_FOUND)
  list(APPEND SOURCES VAAPI.cpp)
  list(APPEND HEADERS VAAPI.h)
endif()

if(APPLE)
  list(APPEND SOURCES VTB.cpp)
  list(APPEND HEADERS VTB.h)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES MMALCodec.cpp)
  list(APPEND HEADERS MMALCodec.h)
  list(APPEND SOURCES MMALFFmpeg.cpp)
  list(APPEND HEADERS MMALFFmpeg.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES DVDVideoCodecAndroidMediaCodec.cpp)
  list(APPEND HEADERS DVDVideoCodecAndroidMediaCodec.h)
endif()

if(CORE_PLATFORM_NAME_LC STREQUAL gbm)
  list(APPEND SOURCES DVDVideoCodecDRMPRIME.cpp)
  list(APPEND HEADERS DVDVideoCodecDRMPRIME.h)
endif()

core_add_library(dvdvideocodecs)
