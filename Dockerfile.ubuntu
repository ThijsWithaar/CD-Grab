FROM ubuntu:devel

# tzdata likes to ask questions:
ARG DEBIAN_FRONTEND=noninteractive

# Install latest version of build-tools and development libraries
RUN apt update -q
RUN apt upgrade -y
RUN apt install -y build-essential git cmake ninja-build clang lld
RUN apt install -y libflac-dev libopus-dev libsoxr-dev qtbase5-dev qttools5-dev libboost-all-dev
RUN apt install -y doxygen

# Catch2 is not yet in a repo, so build+install ourselves
RUN git clone https://github.com/catchorg/Catch2.git /tmp/catch2
RUN cmake -H/tmp/catch2 -B/tmp/catch2.build -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr
RUN cmake --build /tmp/catch2.build -- install


# Add a non-root user
RUN useradd docker && echo "docker:docker" | chpasswd
RUN mkdir -p /home/docker && chown -R docker:docker /home/docker

# And match user/group of the travis CI, so volumes show up with correct rights
RUN groupadd -g 2000 travis && useradd -g travis -u 2000 travis && echo "travis:travis" | chpasswd
RUN mkdir -p /home/travis && chown -R travis:travis /home/travis

# set a locale
RUN apt-get install -y locales
RUN rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

WORKDIR /home/travis
USER travis
