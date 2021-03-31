include(ExternalProject)

set(mun_executable_path "" CACHE FILEPATH "Location of the Mun executable. This or mun_executable_url is required for building the Mun test snippets")
set(mun_executable_url "" CACHE FILEPATH "Location of the Mun executable on the web. This or mun_executable_path is required for building the Mun test snippets")

set(mun_examples_path "" CACHE FILEPATH "Location of the Mun example source. This is required for building the Mun test snippets")

# Download the mun binary
add_executable(mun_binary IMPORTED)
string(TOUPPER ${mun_os} mun_os_uppercase)
if (mun_executable_path)
    set_target_properties(mun_binary PROPERTIES IMPORTED_LOCATION ${mun_executable_path})
elseif (mun_executable_url)
    ExternalProject_Add(
            mun_binary_download
            PREFIX ${CMAKE_CURRENT_BINARY_DIR}/mun
            URL ${mun_executable_url}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
    )

    ExternalProject_Get_Property(mun_binary_download source_dir)
    set_target_properties(mun_binary PROPERTIES IMPORTED_LOCATION ${source_dir}/mun${CMAKE_EXECUTABLE_SUFFIX})
    add_dependencies(mun_binary mun_binary_download)
else ()
    message(FATAL_ERROR "You must specify either the mun_executable_url or mun_executable_path to be able to test the Mun Runtime")
endif ()

if (NOT mun_examples_path)
    message(FATAL_ERROR "You must specify the mun_examples_path to be able to test the Mun Runtime")
endif ()

# Find all mun files
set(mun_folder ${CMAKE_CURRENT_SOURCE_DIR}/..)
file(GLOB_RECURSE mun_test_files RELATIVE ${mun_examples_path} ${mun_examples_path}/*.toml)
set(mun_tests)

foreach (mun_file ${mun_test_files})
    get_filename_component(mun_file_dir ${mun_file} DIRECTORY)
    set(abs_package_dir ${mun_examples_path}/${mun_file_dir})
    set(abs_toml_file ${abs_package_dir}/mun.toml)
    set(mun_binary_file mod.munlib)
    set(mun_binary_path ${abs_package_dir}/target/${mun_binary_file})
    add_custom_command(
            OUTPUT ${mun_binary_path}
            COMMAND mun_binary build --manifest-path "${abs_toml_file}"
            WORKING_DIRECTORY ${abs_package_dir}
            MAIN_DEPENDENCY ${abs_toml_file})
    list(APPEND mun_tests ${mun_binary_path})
endforeach ()

add_custom_target(mun_test_munlibs ALL
        DEPENDS ${mun_tests})

# Add the tests
add_executable(MunRuntimeTests
    catch_main.cc
    marshal.cc
    runtime.cc
    extern.cc
)

target_include_directories(MunRuntimeTests PRIVATE ${mun_folder}/external/catch2/single_include ${mun_folder}/include)
target_link_libraries(MunRuntimeTests MunRuntime)
add_dependencies(MunRuntimeTests mun_test_munlibs)
target_compile_definitions(MunRuntimeTests PRIVATE -DMUN_TEST_DIR="${mun_examples_path}/")
set_property(TARGET MunRuntimeTests PROPERTY CXX_STANDARD 17)

add_custom_command(TARGET MunRuntimeTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:MunRuntime>
        $<TARGET_FILE_DIR:MunRuntimeTests>)

list(APPEND CMAKE_MODULE_PATH "${mun_folder}/external/catch2/contrib")
include(Catch)
catch_discover_tests(MunRuntimeTests)
