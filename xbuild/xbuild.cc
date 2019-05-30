#include <stdio.h>
#include <string.h>

#include <vector>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


// Locations of the real and
const char sh_org[] = "/bin/sh.real";
const char qemu[] = "/usr/bin/qemu-arm-static";

// Use this file as marker to determine if we're cross-building.
// put it in tmp and make it world-adjustable.
const char xbuild_flag[] = "/tmp/__xbuild__";

const uid_t nobody = 65534;
const gid_t nogroup = 65534;


bool is_cross()
{
	return access(xbuild_flag, F_OK) != -1;
}


int xbuild_start()
{
	int handle = creat(xbuild_flag, S_IRWXU | S_IRWXG | S_IRWXO);
	close(handle);
	chown(xbuild_flag, nobody, nogroup);
	return handle > 0 ? 0 : -7;
}


int xbuild_stop()
{
	return remove(xbuild_flag);
}


int run_as_xbuild(int argc, char* argv[])
{
	if(argc < 2)
		return -1;

	const char* mode = argv[1];

	int r = -2;
	if (strcmp(mode, "start") == 0)
		r = xbuild_start();
	else if (strcmp(mode, "stop") == 0)
		r = xbuild_stop();

	printf("xbuild cross building: %i\n", is_cross());
	return r;
}


int run_as_shell(int argc, char* argv[])
{
	if(!is_cross())
	{
		if(access(sh_org, F_OK) == -1)
		{
			printf("xbuild: shell not found (%s)\n", sh_org);
			return -6;
		}
		return execv(sh_org, argv);
	}

	std::vector<const char *> p_qargv {"-execve", "-0", argv[0], sh_org};
	for(size_t n=1; n < argc; n++)
		p_qargv.push_back(argv[n]);
	char** p_arg = const_cast<char**>(p_qargv.data());

	printf("xbuild: calling %s as %s\n", sh_org, argv[0]);

	// If we're being called recursively, only the first call should be qemu-ed:
	xbuild_stop();
	int re = execv(qemu, p_arg);
	xbuild_start();

	return re;
}


int main(int argc, char* argv[])
{
	const char *bn = basename(argv[0]);

	// Provide a command-line interface to start/stop the emulation
	if(strcmp(bn, "xbuild") == 0)
		return run_as_xbuild(argc, argv);

	// The shell-switcheroo
	return run_as_shell(argc, argv);
}
