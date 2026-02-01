project(CastServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


file(GLOB_RECURSE CAST_SOURCES CONFIGURE_DEPENDS 
	${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_executable(CastServer
    ${CAST_SOURCES}
)


target_include_directories(CastServer PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

if(UNIX) # on MacOS I need to manually build the maria db connector which doesn't have makefile module and I need to manually link its dependencies
    target_link_libraries(CastServer
    PUBLIC
        Common
        OpenSSL::SSL
        OpenSSL::Crypto
        z
        mariadb-connector-cpp::mariadb-connector-cpp
    )

elseif (WIN32)
    target_link_libraries(CastServer
    PUBLIC
        Common
        unofficial::mariadb-connector-cpp::mariadbcpp
        wsock32
        ws2_32
    )
endif()

set_target_properties(CastServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

