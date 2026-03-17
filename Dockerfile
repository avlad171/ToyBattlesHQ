FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    apt-utils \
    nano  \
    emacs \
    htop \
    cmake \
    git \
    pkg-config \
    libmariadb-dev \
    mariadb-server \
    openssl libssl-dev \
    libcrypto++ libcrypto++-dev \
    zlib1g-dev \
    curl \
    wget \
    libboost-all-dev \
    librange-v3-dev \
    libcurl4-openssl-dev \
    libasio-dev

# install ninja
RUN wget -qO /usr/local/bin/ninja.gz https://github.com/ninja-build/ninja/releases/latest/download/ninja-linux.zip
RUN gunzip /usr/local/bin/ninja.gz
RUN chmod a+x /usr/local/bin/ninja

# Set working directory
WORKDIR /app

# Install mariadb connector
RUN git clone --recursive https://github.com/MariaDB-Corporation/mariadb-connector-cpp.git
RUN mkdir build
WORKDIR /app/mariadb-connector-cpp
RUN cmake ../mariadb-connector-cpp/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONC_WITH_UNIT_TESTS=Off -DCMAKE_INSTALL_PREFIX=/usr/local -DINSTALL_CMAKE_CONFIG=ON -DWITH_SSL=OPENSSL
RUN cmake --build . --config RelWithDebInfo
RUN make -j$(nproc) && make install
WORKDIR /app


# Copy project source
COPY ./AuthServer ./AuthServer/
COPY ./CastServer ./CastServer/
COPY ./Common ./Common/
COPY ./MainServer ./MainServer/
COPY ./ExternalLibraries ./ExternalLibraries/
COPY ./GradedAccess ./GradedAccess/
COPY ./Setup ./Setup/
COPY ./Tools ./Tools/
COPY ./CMakeLists.txt .
COPY ./mariadb-connector-cpp-config.cmake /usr/lib/x86_64-linux-gnu/cmake/mariadb-connector-cpp/
#on macos you use /usr/local/lib/cmake
COPY ./microvolts-db.sql .
COPY ./RewardItemIDs.txt .

# Build project
RUN cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_MAKE_PROGRAM=/usr/local/bin/ninja -G Ninja \
    -DPython_EXECUTABLE=/usr/bin/python3 -DPython3_EXECUTABLE=/usr/bin/python3 \
    -S /app \
    -B /app/cmake-build-release
RUN cmake --build /app/cmake-build-release -j$(nproc) --config Release


# Default command (can be changed to your binary)
CMD ["/bin/bash"]