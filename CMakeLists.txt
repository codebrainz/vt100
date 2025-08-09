# Minimum CMake version and project name
cmake_minimum_required(VERSION 3.10)

project(vt100
    VERSION 1.0.0
    DESCRIPTION "ANSI/VT100 parser and emulator library"
    LANGUAGES C)


# Options for tests and docs
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_DOCS "Build documentation with Doxygen" OFF)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source/header variables
set(VT100_SRC vt100.c)
set(VT100_HDR vt100.h)

add_library(vt100 STATIC ${VT100_SRC} ${VT100_HDR})

target_include_directories(vt100 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Use GNUInstallDirs for standard install locations
include(GNUInstallDirs)
install(TARGETS vt100
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${VT100_HDR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Only build test executable if enabled
if(BUILD_TESTS)
    add_executable(vt100_test vt100_test.c)
    target_link_libraries(vt100_test PRIVATE vt100)
    target_include_directories(vt100_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Doxygen documentation (optional, controlled by BUILD_DOCS)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
        set(DOXYGEN_PROJECT_NUMBER "${PROJECT_VERSION}")
        set(DOXYGEN_PROJECT_BRIEF "${PROJECT_DESCRIPTION}")
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs/reference")
        set(DOXYGEN_INPUT ".")
        set(DOXYGEN_FILE_PATTERNS "vt100.c vt100.h LICENSE.md README.md")
        configure_file(
            ${DOXYGEN_IN}
            ${DOXYGEN_OUT}
            @ONLY
        )
        add_custom_target(doc
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DOXYGEN_OUTPUT_DIRECTORY}"
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    endif()
endif()
