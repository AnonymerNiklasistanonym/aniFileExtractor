cmake_minimum_required(VERSION 3.14)

project(aniFileExtractor VERSION 1.0.0 LANGUAGES CXX)

# Set important paths
set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(INCLUDE_DIR "${PROJECT_DIR}/include")
set(SOURCE_DIR "${PROJECT_DIR}")
set(VENDOR_DIR_DATE "${PROJECT_DIR}/date")
set(VENDOR_DIR_FMT "${PROJECT_DIR}/fmt")
set(VENDOR_DIR_CSV_PARSER "${PROJECT_DIR}/fast-cpp-csv-parser")
set(VENDOR_DIR_GNUPLOT "${PROJECT_DIR}/gnuplot-iostream")

# Show or hide exact makefile commands
set(CMAKE_VERBOSE_MAKEFILE on)

# Find all source/header files
# > Header files
file(
    GLOB_RECURSE PROJECT_HEADER_FILES
    "${INCLUDE_DIR}/*.h"
    "${INCLUDE_DIR}/*.hpp"
    "${SOURCE_DIR}/*.h"
    "${SOURCE_DIR}/*.hpp"
)
# > Remove header files from the build/binary directory and print the other ones
message(STATUS "Project header files:")
foreach(PROJECT_HEADER_FILE ${PROJECT_HEADER_FILES})
    string(FIND ${PROJECT_HEADER_FILE} ${PROJECT_BINARY_DIR} EXCLUDE_DIR_FOUND_BIN)
    if ((NOT ${EXCLUDE_DIR_FOUND_BIN} EQUAL -1))
        list(REMOVE_ITEM PROJECT_HEADER_FILES ${PROJECT_HEADER_FILE})
    else()
        message(STATUS "- ${PROJECT_HEADER_FILE}")
    endif()
endforeach()
# > Source files
set(PROJECT_MAIN_SOURCE_FILE "${SOURCE_DIR}/${PROJECT_NAME}.cpp")
file(
    GLOB_RECURSE PROJECT_SOURCE_FILES
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.c"
)
# > Remove source files from the build/binary directory and print the other ones
message(STATUS "Project source files:")
foreach(PROJECT_SOURCE_FILE ${PROJECT_SOURCE_FILES})
    string(FIND ${PROJECT_SOURCE_FILE} ${PROJECT_BINARY_DIR} EXCLUDE_DIR_FOUND_BIN)
    if ((NOT ${EXCLUDE_DIR_FOUND_BIN} EQUAL -1))
        list(REMOVE_ITEM PROJECT_SOURCE_FILES ${PROJECT_SOURCE_FILE})
    else()
        message(STATUS "- ${PROJECT_SOURCE_FILE}")
    endif()
endforeach()
# > Extract main source file
get_filename_component(
    PROJECT_MAIN_SOURCE_FILE
    ${PROJECT_MAIN_SOURCE_FILE}
    ABSOLUTE
)
list(REMOVE_ITEM PROJECT_SOURCE_FILES "${PROJECT_MAIN_SOURCE_FILE}")

# Option: Statically check source code
option(CHECK_CODE "Statically check source code" OFF)
message(STATUS "Statically check source code: CHECK_CODE=${CHECK_CODE}")
find_program(CPPCHECK_EXECUTABLE "cppcheck")
if(CHECK_CODE)
    if(CPPCHECK_EXECUTABLE)
        list(
            APPEND
            CPPCHECK_ARGS
            --enable=warning,style,performance,portability,unusedFunction
            --std=c++20
            --verbose
            --error-exitcode=1
            --language=c++
            -I
            "${INCLUDE_DIR}"
            -I
            "${SOURCE_DIR}"
            ${PROJECT_MAIN_SOURCE_FILE}
            ${PROJECT_SOURCE_FILES}
            ${PROJECT_HEADER_FILES}
        )
        add_custom_target(
            "${PROJECT_NAME}-check-code"
            ALL
            COMMAND
            ${CPPCHECK_EXECUTABLE}
            ${CPPCHECK_ARGS}
            WORKING_DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT
            "Statically lint C++ code with CppCheck"
            VERBATIM
        )
    else()
        message(
            FATAL_ERROR
            "The program CppCheck (cppcheck or cppcheck.exe) was not found but is required"
        )
    endif()
endif()

# Option: Format source code
option(FORMAT_CODE "Format source code" OFF)
message(STATUS "Format source code: FORMAT_CODE=${FORMAT_CODE}")
find_program(ASTYLE_EXECUTABLE "astyle")
if(FORMAT_CODE)
    if(ASTYLE_EXECUTABLE)
        list(
            APPEND
            ASTYLE_ARGS
            --style=1tbs
            --lineend=linux
            --suffix=none
            --pad-oper
            --unpad-paren
            --break-closing-brackets
            --align-pointer=name
            --align-reference=name
            --indent-preproc-define
            --indent-switches
            --indent-col1-comments
            --keep-one-line-statements
            --keep-one-line-blocks
            --pad-header
            --convert-tabs
            --min-conditional-indent=0
            --indent=spaces=4
            --close-templates
            --add-brackets
            --break-after-logical
            --max-code-length=100
            ${PROJECT_MAIN_SOURCE_FILE}
            ${PROJECT_SOURCE_FILES}
            ${PROJECT_HEADER_FILES}
        )
        add_custom_target(
            "${PROJECT_NAME}-format-code"
            ALL
            COMMAND
            ${ASTYLE_EXECUTABLE}
            ${ASTYLE_ARGS}
            WORKING_DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT
            "Format C++ code with Astyle"
            VERBATIM
        )
    else()
        message(
            FATAL_ERROR
            "The program Astyle (astyle or astyle.exe) was not found but is required"
        )
    endif()
endif()

add_executable(${PROJECT_NAME} ${PROJECT_MAIN_SOURCE_FILE} ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})

# Set library source files compilation flags for different compilers
target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
    -pedantic
    -Wall
    -Werror
    # Disable errors for unused functions and parameters but still show warnings
    -Wno-error=unused-function
    -Wno-error=unused-parameter
    -Wextra
    -Wformat
    >
    $<$<CXX_COMPILER_ID:MSVC>:
    /Wall
    /pedantic
    /W4
    >
)

# Set C++ version for the project
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Set Debug post fix to differentiate a debug and release executable
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")
