project(Common LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_library(Common STATIC
    ${COMMON_SOURCES}
)

add_subdirectory(${CMAKE_SOURCE_DIR}/ExternalLibraries/libbcrypt
        ${CMAKE_BINARY_DIR}/libbcrypt_build)

set_target_properties(Common PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
)

target_include_directories(Common
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_SOURCE_DIR}/ExternalLibraries
        ${CMAKE_SOURCE_DIR}/Common/include/Utils
        ${Boost_INCLUDE_DIRS}
)

if(APPLE) # on MacOS PkgConfig::Cryptopp
    target_link_libraries(Common
            PUBLIC
            PkgConfig::Cryptopp
            curl
    )
elseif(UNIX)
    target_link_libraries(Common
            PUBLIC
            PkgConfig::Cryptopp
            curl
    )
else()
    target_link_libraries(Common
            PUBLIC
            cryptopp::cryptopp
            curl
    )
endif()


if (WIN32)
    target_link_libraries(Common PUBLIC wsock32 ws2_32)
endif()
