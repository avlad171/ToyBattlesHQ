project(MainServer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE MAIN_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_executable(MainServer
    ${MAIN_SOURCES}
)

find_package(CURL REQUIRED)

target_include_directories(MainServer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

if(APPLE)
target_link_libraries(MainServer
    PUBLIC
        Common
        mariadb-connector-cpp::mariadb-connector-cpp
        OpenSSL::SSL
        OpenSSL::Crypto
        z
        jwt-cpp::jwt-cpp
        bcrypt
        range-v3::range-v3
        Boost::json
        ${CURL_LIBRARIES}
)
elseif(UNIX)
target_link_libraries(MainServer
        PUBLIC
        Common
        mariadb-connector-cpp::mariadb-connector-cpp
        OpenSSL::SSL
        OpenSSL::Crypto
        z
        jwt-cpp::jwt-cpp
        bcrypt
        range-v3::range-v3
        Boost::json
        ${CURL_LIBRARIES}
)
else()
target_link_libraries(MainServer
        PUBLIC
        Common
        unofficial::mariadb-connector-cpp::mariadbcpp
        OpenSSL::SSL
        OpenSSL::Crypto
        z
        jwt-cpp::jwt-cpp
        bcrypt
        range-v3::range-v3
        Boost::json
        ${CURL_LIBRARIES}
)
endif()

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

set_target_properties(MainServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
