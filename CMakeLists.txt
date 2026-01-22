cmake_minimum_required(VERSION 3.1)
project(SAPPOROBDD2 VERSION 2.0.0 LANGUAGES CXX)

# C++11 required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(SBDD2_BUILD_TESTS "Build tests" ON)
option(SBDD2_BUILD_SHARED "Build shared library" ON)
option(SBDD2_BUILD_STATIC "Build static library" ON)

# Thread support
find_package(Threads REQUIRED)

# GMP support (optional, auto-detected)
option(SBDD2_USE_GMP "Enable GMP for exact counting (auto-detected)" ON)
set(SBDD2_HAS_GMP OFF)

if(SBDD2_USE_GMP)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GMP QUIET gmp gmpxx)
        if(GMP_FOUND)
            set(SBDD2_HAS_GMP ON)
            message(STATUS "GMP found - exact_count() enabled")
        endif()
    endif()

    if(NOT SBDD2_HAS_GMP)
        # Try manual detection
        find_library(GMP_LIBRARY gmp)
        find_library(GMPXX_LIBRARY gmpxx)
        find_path(GMP_INCLUDE_DIR gmpxx.h)

        if(GMP_LIBRARY AND GMPXX_LIBRARY AND GMP_INCLUDE_DIR)
            set(SBDD2_HAS_GMP ON)
            set(GMP_LIBRARIES ${GMP_LIBRARY} ${GMPXX_LIBRARY})
            set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})
            message(STATUS "GMP found (manual) - exact_count() enabled")
        endif()
    endif()

    if(NOT SBDD2_HAS_GMP)
        message(STATUS "GMP not found - exact_count() disabled")
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SBDD2_SOURCES
    src/dd_node.cpp
    src/dd_manager.cpp
    src/dd_base.cpp
    src/dd_node_ref.cpp
    src/bdd.cpp
    src/zdd.cpp
    src/zdd_helper.cpp
    src/unreduced_bdd.cpp
    src/unreduced_zdd.cpp
    src/qdd.cpp
    src/pidd.cpp
    src/seqbdd.cpp
    src/gbase.cpp
    src/bddct.cpp
    src/io.cpp
)

# Header files
set(SBDD2_HEADERS
    include/sbdd2/types.hpp
    include/sbdd2/dd_node.hpp
    include/sbdd2/dd_manager.hpp
    include/sbdd2/dd_base.hpp
    include/sbdd2/dd_node_ref.hpp
    include/sbdd2/bdd.hpp
    include/sbdd2/zdd.hpp
    include/sbdd2/zdd_helper.hpp
    include/sbdd2/unreduced_bdd.hpp
    include/sbdd2/unreduced_zdd.hpp
    include/sbdd2/qdd.hpp
    include/sbdd2/pidd.hpp
    include/sbdd2/seqbdd.hpp
    include/sbdd2/gbase.hpp
    include/sbdd2/bddct.hpp
    include/sbdd2/io.hpp
    include/sbdd2/exception.hpp
    include/sbdd2/sbdd2.hpp
)

# Static library
if(SBDD2_BUILD_STATIC)
    add_library(sbdd2_static STATIC ${SBDD2_SOURCES})
    target_link_libraries(sbdd2_static PRIVATE Threads::Threads)
    set_target_properties(sbdd2_static PROPERTIES OUTPUT_NAME sbdd2)

    if(SBDD2_HAS_GMP)
        target_compile_definitions(sbdd2_static PUBLIC SBDD2_HAS_GMP)
        target_include_directories(sbdd2_static PUBLIC ${GMP_INCLUDE_DIRS})
        target_link_libraries(sbdd2_static PUBLIC ${GMP_LIBRARIES})
    endif()
endif()

# Shared library
if(SBDD2_BUILD_SHARED)
    add_library(sbdd2_shared SHARED ${SBDD2_SOURCES})
    target_link_libraries(sbdd2_shared PRIVATE Threads::Threads)
    set_target_properties(sbdd2_shared PROPERTIES OUTPUT_NAME sbdd2)

    if(SBDD2_HAS_GMP)
        target_compile_definitions(sbdd2_shared PUBLIC SBDD2_HAS_GMP)
        target_include_directories(sbdd2_shared PUBLIC ${GMP_INCLUDE_DIRS})
        target_link_libraries(sbdd2_shared PUBLIC ${GMP_LIBRARIES})
    endif()
endif()

# Alias target for easier linking
if(SBDD2_BUILD_STATIC)
    add_library(sbdd2 ALIAS sbdd2_static)
elseif(SBDD2_BUILD_SHARED)
    add_library(sbdd2 ALIAS sbdd2_shared)
endif()

# Tests
if(SBDD2_BUILD_TESTS)
    enable_testing()

    # Try to find GTest
    find_package(GTest QUIET)

    if(GTest_FOUND)
        message(STATUS "Google Test found, building tests")
        add_subdirectory(tests)
    else()
        message(STATUS "Google Test not found, tests will not be built")
    endif()
endif()

# Examples
option(SBDD2_BUILD_EXAMPLES "Build examples" ON)
if(SBDD2_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install
install(DIRECTORY include/sbdd2 DESTINATION include)

if(SBDD2_BUILD_STATIC)
    install(TARGETS sbdd2_static
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

if(SBDD2_BUILD_SHARED)
    install(TARGETS sbdd2_shared
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()
