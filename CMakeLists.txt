cmake_minimum_required(VERSION 3.10)
project(MunRuntime VERSION 0.1.0 LANGUAGES C CXX)

option(mun_build_examples "Build all of Mun's own examples." OFF)
option(mun_build_tests "Build all of Mun's own tests." OFF)

# Determine platform (32/64)
if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(X64 ON)
else ()
    set(X86 ON)
endif ()

add_library(MunRuntime SHARED IMPORTED GLOBAL)

set_target_properties(MunRuntime PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/external/md5/include"
)

if (X64)
    if (WIN32)
        set(mun_os win64)
    elseif (UNIX AND NOT APPLE)
        set(mun_os linux64)
    elseif (UNIX AND APPLE)
        set(mun_os osx64)
    else ()
        message(ERROR "Unsupported operating system.")
    endif ()
else ()
    message(ERROR "64-bit operating systems are not supported at present.")
endif ()

set_target_properties(MunRuntime PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/${mun_os}/${CMAKE_SHARED_LIBRARY_PREFIX}mun_runtime${CMAKE_SHARED_LIBRARY_SUFFIX}
)

if (WIN32)
    set_target_properties(MunRuntime PROPERTIES
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/bin/${mun_os}/mun_runtime${CMAKE_SHARED_LIBRARY_SUFFIX}.lib
)
endif ()

if (mun_build_examples)
    add_subdirectory(examples)
endif ()

include(CTest)
if (mun_build_tests)
    add_subdirectory(tests)
endif ()
