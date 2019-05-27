FROM debian:sid

ARG DEBIAN_FRONTEND=noninteractive

# Add repositories
RUN apt update -q
RUN apt install -y wget gnupg ca-certificates software-properties-common
RUN wget https://apt.llvm.org/llvm-snapshot.gpg.key
RUN apt-key add llvm-snapshot.gpg.key
RUN apt-add-repository "deb http://apt.llvm.org/unstable/ llvm-toolchain main"

# Install latest version of build-tools and development libraries
RUN apt update -q
RUN apt upgrade -y
RUN apt install -y apt-utils build-essential git cmake ninja-build clang-9
RUN apt install -y libflac-dev libopus-dev libsoxr-dev qtbase5-dev qttools5-dev libboost-all-dev
RUN apt install -y libeigen3-dev

# Register the compiler
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-9 90
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-9 90
RUN update-alternatives --config clang
RUN update-alternatives --config clang++

# Catch2 is not yet in a repo, so build+install ourselves
RUN git clone https://github.com/catchorg/Catch2.git /catch2
RUN cmake -H/catch2 -B/catch2.build -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr
RUN cmake --build /catch2.build -- install

# TODO: Bring in the visual studio tools in wine.
# Then use
# https://github.com/paleozogt/MSVCDocker/blob/master/Dockerfile
