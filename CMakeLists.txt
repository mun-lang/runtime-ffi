cmake_minimum_required(VERSION 3.10)
project(MunRuntime VERSION 0.1.0 LANGUAGES C CXX)

# Determine platform (32/64)
if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(X64 ON)
else ()
    set(X86 ON)
endif ()

add_library(MunRuntime SHARED IMPORTED GLOBAL)

set_target_properties(MunRuntime PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

if (X64)
    if (WIN32)
        set(MUN_OS win64)
    elseif (UNIX AND NOT APPLE)
        set(MUN_OS linux64)
    elseif (UNIX AND APPLE)
        set(MUN_OS osx64)
    else ()
        message(ERROR "Unsupported operating system.")
    endif ()
else ()
    message(ERROR "64-bit operating systems are not supported at present.")
endif ()

set_target_properties(MunRuntime PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/${MUN_OS}/mun_runtime${CMAKE_SHARED_LIBRARY_SUFFIX}
        )

if (WIN32)
    set_target_properties(MunRuntime PROPERTIES
            IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/bin/${MUN_OS}/mun_runtime${CMAKE_SHARED_LIBRARY_SUFFIX}.lib
            )
endif ()

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()