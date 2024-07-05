cmake_minimum_required(VERSION 3.11)

project(cpp-lifetime)

include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

option(CMake_RUN_CLANG_TIDY "Run clang-tidy with the compiler." OFF)
if(CMake_RUN_CLANG_TIDY)
  if(CMake_SOURCE_DIR STREQUAL CMake_BINARY_DIR)
    message(FATAL_ERROR "CMake_RUN_CLANG_TIDY requires an out-of-source build!")
  endif()
  find_program(CLANG_TIDY_COMMAND NAMES clang-tidy)
  if(NOT CLANG_TIDY_COMMAND)
    message(WARNING "CMake_RUN_CLANG_TIDY is ON but clang-tidy is not found!")
    set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE)
  else()
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND};-header-filter='${CMAKE_SOURCE_DIR}/'")
  endif()
endif()
configure_file(.clang-tidy .clang-tidy COPYONLY)


add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(${PROJECT_NAME}
	INTERFACE
		cxx_std_11
)

if(BUILD_TESTING)

add_executable("${PROJECT_NAME}_tests" test/lifetime-test.cpp)
target_link_libraries("${PROJECT_NAME}_tests" 
        PRIVATE
            ${PROJECT_NAME}
    )
add_test(tests "${PROJECT_NAME}_tests")

add_executable("${PROJECT_NAME}_example_1" examples/example_1.cpp)
target_link_libraries("${PROJECT_NAME}_example_1"
        PRIVATE
            ${PROJECT_NAME}
    )
add_test(example_1 "${PROJECT_NAME}_example_1")

endif()


set (EXCLUDE_DIR "${CMAKE_BINARY_DIR}")
file (GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")
foreach (TMP_PATH ${SOURCE_FILES})
    message("${TMP_PATH} ${EXCLUDE_DIR}")
    string (FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        list (REMOVE_ITEM SOURCE_FILES ${TMP_PATH})
    endif ()
endforeach(TMP_PATH)

add_custom_target(
        format-all
        COMMAND clang-format
        --style=file
        -i
        ${SOURCE_FILES}
)

add_custom_target(
        tidy-all
        COMMAND clang-tidy
        -p ${CMAKE_BUILD_DIR}
        -header-filter='.*'
        ${SOURCE_FILES}
)