FROM ubuntu:14.04

ARG cmake_version=3.20.6
ARG gcc_version=9

RUN apt-get update && apt-get install -y software-properties-common && \
    add-apt-repository ppa:ubuntu-toolchain-r/test -y

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-$gcc_version-multilib \
    g++-$gcc_version-multilib \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$gcc_version 60 --slave /usr/bin/g++ g++ /usr/bin/g++-$gcc_version && \
    update-alternatives --config gcc

RUN wget https://github.com/Kitware/CMake/releases/download/v$cmake_version/cmake-$cmake_version-linux-x86_64.tar.gz && \
    tar -xzvf cmake-$cmake_version-linux-x86_64.tar.gz && \
    cp -r cmake-$cmake_version-linux-x86_64/bin/* /usr/local/bin && \
    cp -r cmake-$cmake_version-linux-x86_64/share/* /usr/local/share && \
    rm -rf cmake-$cmake_version-linux-x86_64 cmake-$cmake_version-linux-x86_64.tar.gz

WORKDIR /ezhttp

CMD ["sh", "-c", "mkdir -p cmake-build-release && cd cmake-build-release && cmake .. -DCMAKE_BUILD_TYPE=Release && make easy_http"]
