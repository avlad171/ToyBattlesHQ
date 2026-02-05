cmake_minimum_required(VERSION 3.16)

project(MainServer LANGUAGES CXX)

file(GLOB_RECURSE MAIN_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

file(GLOB_RECURSE MAIN_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.h
    ${CMAKE_CURRENT_LIST_DIR}/*.hpp
)

add_executable(MainServer
    ${MAIN_SOURCES}
    ${MAIN_HEADERS}
)

find_package(CURL REQUIRED)

target_include_directories(MainServer
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

find_path(JWT_CPP_INCLUDE_DIRS "jwt-cpp/base.h")
target_include_directories(MainServer PUBLIC ${JWT_CPP_INCLUDE_DIRS})

target_link_libraries(MainServer
    PUBLIC
        Common
        mariadb-connector-cpp::mariadb-connector-cpp
        OpenSSL::SSL
        OpenSSL::Crypto
        jwt-cpp::jwt-cpp
        #cryptopp::cryptopp
        range-v3::range-v3
        Boost::json
        ${CURL_LIBRARIES}
)

target_include_directories(AuthServer PRIVATE
    ${CURL_INCLUDE_DIRS}
)

if (WIN32)
    target_link_libraries(MainServer PUBLIC wsock32 ws2_32)
endif()

if (MSVC)
    target_compile_options(MainServer PUBLIC /W0)
else()
    target_compile_options(MainServer PUBLIC -w)
endif()

set_target_properties(MainServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    TARGET MainServer
    PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E echo "Running generate_command_includes.py..."
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/generate_command_includes.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

if (MSVC)
    target_compile_options(MainServer PUBLIC /W0)
else()
    target_compile_options(MainServer PUBLIC -w)
endif()

set_target_properties(MainServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
