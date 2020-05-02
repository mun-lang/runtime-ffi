include(ExternalProject)

set(MUN_EXECUTABLE_PATH "" CACHE FILEPATH "Location of the Mun executable. This or MUN_EXECUTABLE_URL is required for building the Mun test snippets")
set(MUN_EXECUTABLE_URL "" CACHE FILEPATH "Location of the Mun executable on the web. This or MUN_EXECUTABLE_PATH is required for building the Mun test snippets")

# Download the mun binary
add_executable(mun_binary IMPORTED)
string(TOUPPER ${MUN_OS} MUN_OS_UPPERCASE)
if (MUN_EXECUTABLE_PATH)
    set_target_properties(mun_binary PROPERTIES IMPORTED_LOCATION ${MUN_EXECUTABLE_PATH})
elseif (MUN_EXECUTABLE_URL)
    ExternalProject_Add(
            mun_binary_download
            PREFIX ${CMAKE_CURRENT_BINARY_DIR}/mun
            URL ${MUN_EXECUTABLE_URL}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
    )

    ExternalProject_Get_Property(mun_binary_download source_dir)
    set_target_properties(mun_binary PROPERTIES IMPORTED_LOCATION ${source_dir}/mun${CMAKE_EXECUTABLE_SUFFIX})
    add_dependencies(mun_binary mun_binary_download)
else ()
    message(FATAL_ERROR "You must specify either the MUN_EXECUTABLE_URL or MUN_EXECUTABLE_PATH to be able to test the Mun Runtime")
endif ()

# Find all mun files
set(MUN_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(MUN_TEST_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
set(MUN_TEST_BIN_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/tests)
file(GLOB_RECURSE mun_test_files RELATIVE ${MUN_TEST_FOLDER} ${MUN_TEST_FOLDER}/*.mun)
set(mun_tests)

foreach (mun_file ${mun_test_files})
    get_filename_component(mun_filename ${mun_file} NAME_WE)
    get_filename_component(mun_file_dir ${mun_file} DIRECTORY)
    set(abs_mun_file ${MUN_TEST_FOLDER}/${mun_file})
    set(mun_binary_file ${mun_filename}.munlib)
    set(mun_binary_path ${MUN_TEST_BIN_FOLDER}/${mun_file_dir}/${mun_binary_file})
    file(MAKE_DIRECTORY ${MUN_TEST_BIN_FOLDER}/${mun_file_dir})
    add_custom_command(
            OUTPUT ${mun_binary_path}
            COMMAND mun_binary build "${abs_mun_file}"
            WORKING_DIRECTORY ${MUN_TEST_BIN_FOLDER}/${mun_file_dir}
            MAIN_DEPENDENCY ${abs_mun_file})
    list(APPEND mun_tests ${mun_binary_path})
endforeach ()

add_custom_target(mun_test_munlibs ALL
        DEPENDS ${mun_tests})

# Add the tests
add_executable(MunRuntimeTests catch_main.cpp md5.cpp runtime.cpp)

target_include_directories(MunRuntimeTests PRIVATE ${MUN_FOLDER}/external/catch2/single_include ${MUN_FOLDER}/include)
target_link_libraries(MunRuntimeTests MunRuntime)
add_dependencies(MunRuntimeTests mun_test_munlibs)
target_compile_definitions(MunRuntimeTests PRIVATE -DMUN_TEST_DIR="${MUN_TEST_BIN_FOLDER}/")
set_property(TARGET MunRuntimeTests PROPERTY CXX_STANDARD 17)

add_custom_command(TARGET MunRuntimeTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:MunRuntime>
        $<TARGET_FILE_DIR:MunRuntimeTests>)

list(APPEND CMAKE_MODULE_PATH "${MUN_FOLDER}/external/catch2/contrib")
include(Catch)
catch_discover_tests(MunRuntimeTests)