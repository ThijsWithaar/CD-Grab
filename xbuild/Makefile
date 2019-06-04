all: qemu

clean:
	rm -rf qemu

qemu:
	sudo apt install libglib2.0-dev libpixman-1-dev
	git clone https://github.com/balena-io/qemu.git
	cd qemu && ./configure --target-list=arm-linux-user --static && make
	cp qemu/arm-linux-user/qemu-arm bin/qemu-arm-static
	strip bin/qemu-arm-static

.PHONY: clean qemu
