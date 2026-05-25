cmake_minimum_required(VERSION 3.27)

if (WIN32)
    add_compile_options(/wd4068)
endif ()

include(ExternalProject)
include(FetchContent)

option(STATIC_BUILD "Build static libraries" ON)
option(USE_LIBCXX   "Use libc++" ON)

add_custom_target(clean_objects
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/cmake/CleanObjects.cmake"
    COMMENT "Removing all object (.o) files"
)

project(ntgcalls VERSION 2.2.1.1 LANGUAGES C CXX)
include(cmake/VersionUtil.cmake)

if (MACOS)
    enable_language(OBJCXX)
endif ()

find_package(Threads REQUIRED)

get_filename_component(DEPS_DIR     "deps" REALPATH)
get_filename_component(android_loc  "android" REALPATH)
get_filename_component(jni_loc      "${android_loc}/app/src/main/jni" REALPATH)
get_filename_component(props_loc    "version.properties" REALPATH)

include(cmake/FindGLib.cmake)
include(cmake/FindX11.cmake)
include(cmake/FindMesa.cmake)
include(cmake/FindWebRTC.cmake)
include(cmake/FindLibCXX.cmake)
include(cmake/FindBoost.cmake)
include(cmake/FindOpenH264.cmake)
include(cmake/FindFFmpeg.cmake)

# pybind11
if (IS_PYTHON)
    set(PYBIND11_FINDPYTHON "NEW")
    if (EXISTS "${CMAKE_SOURCE_DIR}/deps/pybind11/CMakeLists.txt")
        add_subdirectory(deps/pybind11)
    else ()
        message(STATUS "deps/pybind11 is missing; fetching pybind11 during build")
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG stable
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(pybind11)
    endif ()
endif ()

# Oboe
if (ANDROID)
    if (EXISTS "${CMAKE_SOURCE_DIR}/deps/oboe/CMakeLists.txt")
        add_subdirectory(deps/oboe)
    else ()
        message(STATUS "deps/oboe is missing; fetching oboe during build")
        FetchContent_Declare(
            oboe
            GIT_REPOSITORY https://github.com/google/oboe.git
            GIT_TAG 1.10.0
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(oboe)
    endif ()
    setup_platform_flags(oboe OFF)
endif ()

add_subdirectory(wrtc)
add_subdirectory(ntgcalls)
