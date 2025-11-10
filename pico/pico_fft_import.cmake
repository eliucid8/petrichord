# This is a copy of <PICO_EXTRAS_PATH>/external/pico_extras_import.cmake

# This can be dropped into an external project to help locate pico_fft
# It should be include()ed prior to project()

if (DEFINED ENV{PICO_FFT_PATH} AND (NOT PICO_FFT_PATH))
    set(PICO_FFT_PATH $ENV{PICO_FFT_PATH})
    message("Using PICO_FFT_PATH from environment ('${PICO_FFT_PATH}')")
endif ()

if (DEFINED ENV{PICO_FFT_FETCH_FROM_GIT} AND (NOT PICO_FFT_FETCH_FROM_GIT))
    set(PICO_FFT_FETCH_FROM_GIT $ENV{PICO_FFT_FETCH_FROM_GIT})
    message("Using PICO_FFT_FETCH_FROM_GIT from environment ('${PICO_FFT_FETCH_FROM_GIT}')")
endif ()

if (DEFINED ENV{PICO_FFT_FETCH_FROM_GIT_PATH} AND (NOT PICO_FFT_FETCH_FROM_GIT_PATH))
    set(PICO_FFT_FETCH_FROM_GIT_PATH $ENV{PICO_FFT_FETCH_FROM_GIT_PATH})
    message("Using PICO_FFT_FETCH_FROM_GIT_PATH from environment ('${PICO_FFT_FETCH_FROM_GIT_PATH}')")
endif ()

if (NOT PICO_FFT_PATH)
    if (PICO_FFT_FETCH_FROM_GIT)
        include(FetchContent)
        set(FETCHCONTENT_BASE_DIR_SAVE ${FETCHCONTENT_BASE_DIR})
        if (PICO_FFT_FETCH_FROM_GIT_PATH)
            get_filename_component(FETCHCONTENT_BASE_DIR "${PICO_FFT_FETCH_FROM_GIT_PATH}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
        endif ()
        FetchContent_Declare(
                pico_fft
                GIT_REPOSITORY https://github.com/Googool/pico_fft.git
                GIT_TAG master
        )
        if (NOT pico_fft)
            message("Downloading PICO FFT")
            FetchContent_Populate(pico_fft)
            set(PICO_FFT_PATH ${pico_fft_SOURCE_DIR})
        endif ()
        set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_SAVE})
    else ()
        if (PICO_SDK_PATH AND EXISTS "${PICO_SDK_PATH}/../pico_fft")
            set(PICO_FFT_PATH ${PICO_SDK_PATH}/../pico_fft)
            message("Defaulting PICO_FFT_PATH as sibling of PICO_SDK_PATH: ${PICO_FFT_PATH}")
        else()
            if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/pico_fft")
                set(PICO_FFT_PATH ${CMAKE_CURRENT_LIST_DIR}/pico_fft)
            else()
                message(FATAL_ERROR
                        "PICO FFT location was not specified. Please set PICO_FFT_PATH or set PICO_FFT_FETCH_FROM_GIT to on to fetch from git."
                        )
            endif()
        endif()
    endif ()
endif ()

set(PICO_FFT_PATH "${PICO_FFT_PATH}" CACHE PATH "Path to the PICO FFT")
set(PICO_FFT_FETCH_FROM_GIT "${PICO_FFT_FETCH_FROM_GIT}" CACHE BOOL "Set to ON to fetch copy of PICO FFT from git if not otherwise locatable")
set(PICO_FFT_FETCH_FROM_GIT_PATH "${PICO_FFT_FETCH_FROM_GIT_PATH}" CACHE FILEPATH "location to download FFT")

get_filename_component(PICO_FFT_PATH "${PICO_FFT_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
if (NOT EXISTS ${PICO_FFT_PATH})
    message(FATAL_ERROR "Directory '${PICO_FFT_PATH}' not found")
endif ()

set(PICO_FFT_PATH ${PICO_FFT_PATH} CACHE PATH "Path to the PICO FFT" FORCE)

add_subdirectory(${PICO_FFT_PATH} pico_fft)