== Raspberries in the cloud ==

Most cloud continuous integration services provide x64-linux docker environments.
Although cross-compiling with clang/cmake is [relatively straightforward][1], there's one catch:

- QT.

As part of the build, QT runs the moc, uic and rcc executables.
Those have to *really* match the target system.

This folder contains the parts needed to run a arm-linux on x64 system, using qemu.
Some [tricks][2] are applied to not rely on kernel support for this.

The makefile copies qemu-arm-static and generates and uploads a docker container derived from
the arm32v7 debian-stretch image.

This docker image is then used as the other x64 base images:
	- dev-tools are added
	- referenced from travis or appveyor .yml scripts

[1]: http://amgaera.github.io/blog/2014/04/10/cross-compiling-for-raspberry-pi-on-64-bit-linux/
[2]: https://www.balena.io/blog/building-arm-containers-on-any-x86-machine-even-dockerhub/
