#!/usr/bin/env python3
import os
from collections import namedtuple

def run(cmd):
    print('# ' + cmd)
    os.system(cmd)


Config = namedtuple('Config', ('DOCKER_IMAGE','DOCKER_FILE','DOCKER_SYSTEM'))

configs = (
    Config('thijswithaar/ubuntu:devel', 'Dockerfile.ubuntu', 'Ubuntu'),
    Config('thijswithaar/fedora:rawhide', 'Dockerfile.fedora_rawhide', 'Fedora'),
    Config('thijswithaar/raspbian:buster', 'Dockerfile.raspbian', 'Raspbian.Buster'),
)

# Bootstrap the cloud-friendly cross-building environment:
#run('cd xbuild; make docker')

HOST_DIR = os.getcwd()
DOCKER_BUILD_DIR='/home/docker/build/CD-Grab'
for c in configs[2:]:
    BUILD_DIR = f'{DOCKER_BUILD_DIR}/build.{c.DOCKER_SYSTEM}'

    print(f'\n** Building {c.DOCKER_IMAGE:s}')
    run(f'docker container rm -f {c.DOCKER_SYSTEM}')
    run(f'docker build -f {c.DOCKER_FILE:s} -t {c.DOCKER_IMAGE} .')
    run(f'docker push {c.DOCKER_IMAGE}')

    run(f'docker run -it -d --user docker:docker -v {HOST_DIR}:{DOCKER_BUILD_DIR} -w {DOCKER_BUILD_DIR} --name {c.DOCKER_SYSTEM} {c.DOCKER_IMAGE}')
    #if c.DOCKER_FILE == 'Dockerfile.raspbian':
    #    run(f'docker exec {c.DOCKER_SYSTEM} xbuild start')
    run(f'docker exec {c.DOCKER_SYSTEM} cmake -GNinja -DCPACK_SYSTEM_NAME={c.DOCKER_SYSTEM} -DCMAKE_BUILD_TYPE=RELEASE -H{DOCKER_BUILD_DIR} -B{BUILD_DIR}')
    run(f'docker exec {c.DOCKER_SYSTEM} cmake --build {BUILD_DIR} -- package')
