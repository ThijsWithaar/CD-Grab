FROM arm32v7/debian:buster-slim

# Install xbuild
# Qemu must come from: https://github.com/balena-io/qemu.git
COPY ./bin/qemu-arm-static /usr/bin/qemu-arm-static
COPY ./bin/xbuild /usr/bin/xbuild

# Switch sh for the xbuild wrapper
RUN mv /bin/sh /bin/sh.real && ln -s /usr/bin/xbuild /bin/sh

# To see what all happened:
RUN ls -al /bin/sh /usr/bin/xbuild /bin/sh.real /bin/dash

# Starting here would lock the file in /tmp down to root
#RUN ["xbuild", "start"]
