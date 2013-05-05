#!/usr/bin/env sh

. $HOME/repo/lfs/_work_rpib_home/sdk/build/env.sh

$LFS_SDK_CROSS_COMPILE\gcc -Wall -O2 -I. main.c masl.c
