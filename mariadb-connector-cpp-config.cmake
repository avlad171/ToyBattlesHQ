# FindMariaDBConnectorCpp.cmake
# Provides MariaDBConnectorCpp imported target on systems without a CMake config

# Usage:
#   find_package(MariaDBConnectorCpp REQUIRED)
#   target_link_libraries(my_target PRIVATE MariaDB::ConnectorCpp)

find_path(MARIADBCONNECTORCPP_INCLUDE_DIR
  NAMES mariadb/conncpp.hpp
  PATHS /usr/local/include
)

find_path(MARIADBC_INCLUDE_DIR
  NAMES mariadb/mysql.h
  PATHS /usr/local/include /usr/local/opt/mariadb-connector-c/include
)

if (NOT MARIADBCONNECTORCPP_INCLUDE_DIR OR NOT MARIADBC_INCLUDE_DIR)
    message(FATAL_ERROR "MariaDB Connector/C++ include not found. Install via source, Homebrew, apt, etc..")
endif()

find_library(MARIADBCONNECTORCPP_LIBRARY
  NAMES mariadb/libmariadbcpp.a
  PATHS /usr/local/lib
)

find_library(MARIADBC_LIBRARY
  NAMES libmariadb.a
  PATHS /usr/local/lib /usr/local/opt/mariadb-connector-c/lib
)

if (NOT MARIADBCONNECTORCPP_LIBRARY OR NOT MARIADBC_LIBRARY)
    message(FATAL_ERROR "MariaDB Connector/C++ lib not found. Install via source or Homebrew.")
endif()

# Create imported target
add_library(mariadb-connector-cpp SHARED IMPORTED)

set_target_properties(mariadb-connector-cpp PROPERTIES
    IMPORTED_LOCATION "${MARIADBCONNECTORCPP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${MARIADBCONNECTORCPP_INCLUDE_DIR};${MARIADBC_INCLUDE_DIR}"
)

set_target_properties(mariadb-connector-cpp PROPERTIES
    INTERFACE_LINK_LIBRARIES "${MARIADBC_LIBRARY}"
)

# Provide an alias to mimic standard CMake package style
add_library(mariadb-connector-cpp::mariadb-connector-cpp ALIAS mariadb-connector-cpp)

# Optional version info (if you want)
set(mariadb-connector-cpp_FOUND TRUE)
