project(AuthServer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE AUTH_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_executable(AuthServer
    ${AUTH_SOURCES}
)

target_include_directories(AuthServer PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

if(APPLE) # on MacOS I need to manually build the maria db connector which doesn't have makefile module and I need to manually link its dependencies
    target_link_libraries(AuthServer
            PUBLIC
            Common
            OpenSSL::SSL
            OpenSSL::Crypto
            z
            mariadb-connector-cpp::mariadb-connector-cpp
            PkgConfig::Cryptopp
            bcrypt
            Boost::json
    )
elseif(UNIX) # on MacOS I need to manually build the maria db connector which doesn't have makefile module and I need to manually link its dependencies
    target_link_libraries(AuthServer
            PUBLIC
            Common
            OpenSSL::SSL
            OpenSSL::Crypto
            z
            mariadb-connector-cpp::mariadb-connector-cpp
            PkgConfig::Cryptopp
            bcrypt
            Boost::json
    )
else()
    target_link_libraries(AuthServer
            PUBLIC
            Common
            unofficial::mariadb-connector-cpp::mariadbcpp
            bcrypt
            Boost::json
    )
endif()

if(WIN32)
    target_link_libraries(MainServer PUBLIC wsock32 ws2_32)
endif()

set_target_properties(AuthServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

