cmake_minimum_required(VERSION 3.2)

if(NOT DEFINED ENV{TRAVIS_BUILD_NUMBER})
    message("TRAVIS_BUILD_NUMBER environment set to default")
    set(ENV{TRAVIS_BUILD_NUMBER} "1")
endif()

project(bulkmt VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

find_package(Threads)

configure_file(version.h.in version.h)

add_library(bulk_lib bulk.h bulk.cpp)

add_executable(bulkmt main.cpp)

set_target_properties(bulk_lib bulkmt
        PROPERTIES
                CXX_STANDARD 14
                CXX_STANDARD_REQUIRED ON
                CMAKE_CXX_EXTENSIONS OFF
                COMPILE_OPTIONS -Wpedantic -Wall -Wextra
)

target_link_libraries(bulkmt bulk_lib
    ${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS bulkmt RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT edkolotilov@gmail.com)

include (CPack)
