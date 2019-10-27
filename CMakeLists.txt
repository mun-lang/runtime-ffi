cmake_minimum_required(VERSION 3.10)
project(MunRuntime VERSION 0.1.0 LANGUAGES C)

# Determine platform (32/64)
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(X64 ON)
else()
    set(X86 ON)
endif()

add_library(MunRuntime SHARED IMPORTED GLOBAL)

set_target_properties(MunRuntime PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (X64)
    if(WIN32)
        set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/mun_runtime.dll)
    elseif(UNIX AND NOT APPLE)
        set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux64/mun_runtime.so)
    elseif(UNIX AND APPLE)
        set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/bin/osx64/mun_runtime.dylib)
    else()
        message(ERROR "Unsupported operating system.")
    endif()
else()
    message(ERROR "64-bit operating systems are not supported at present.")
endif()

set_target_properties(MunRuntime PROPERTIES
    IMPORTED_LOCATION ${SHARED_LIB}
)

if(WIN32)
    set_target_properties(MunRuntime PROPERTIES
        IMPORTED_IMPLIB ${SHARED_LIB}.lib
    )
endif()
