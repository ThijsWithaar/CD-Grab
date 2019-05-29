#!/usr/bin/env python3
import os
from collections import namedtuple

Config = namedtuple('Config', ('DOCKER_IMAGE','DOCKER_FILE','DOCKER_SYSTEM'))


configs = (
    Config('thijswithaar/ubuntu:local', 'Dockerfile.ubuntu', 'ubuntu'),
    Config('thijswithaar/fedora:local', 'Dockerfile.fedora_rawhide', 'fedora'),
)


def run(cmd):
    print('# ' + cmd)
    return os.system(cmd)

HOST_DIR = os.getcwd()
DOCKER_BUILD_DIR='/home/docker/build/CD-Grab'
for c in configs:

    print(f'\n** Building {c.DOCKER_IMAGE:s}')
    run(f'docker stop {c.DOCKER_SYSTEM}')
    run(f'docker container rm {c.DOCKER_SYSTEM}')
    run(f'docker build -f {c.DOCKER_FILE:s} -t {c.DOCKER_IMAGE} .')
    run(f'docker run -it -d --user docker:docker -v {HOST_DIR}:{DOCKER_BUILD_DIR} -w {DOCKER_BUILD_DIR} --name {c.DOCKER_SYSTEM} {c.DOCKER_IMAGE}')
    run(f'docker exec {c.DOCKER_SYSTEM} cmake -DCPACK_SYSTEM_NAME={c.DOCKER_SYSTEM} -DCMAKE_BUILD_TYPE=RELEASE -H{DOCKER_BUILD_DIR} -B{DOCKER_BUILD_DIR}/build.{c.DOCKER_SYSTEM}')
    run(f'docker exec {c.DOCKER_SYSTEM} cmake --build {DOCKER_BUILD_DIR}/build.{c.DOCKER_SYSTEM} -- package')
