Alb-demos is a collection of C samples over the POSIX standard,
meant to illustrate some base functionalities over the Linux
BSP.
These samples had been structured in a modular way in order to
be not just easy to read but even easy to understand by the
users of our product.
Because the processor of the s32v234 platform is an ARM
Cortex-A53, we use the ARM compiler to generate the binary files
of our code examples.
In the root directory will be a global Makefile which will
recursively call each sample's Makefile. The common variables
will be defined in common.mk file.
In order to build all samples you can simply call "make" and
"make clean" to delete all the already compiled object files.
To personalize the build operation you can call "make sample_name"
and will be executed just the sample's specified Makefile.

Some examples of building samples:
* make CFLAGS="-I" CROSS_COMPILE="arch64-linux-gnu-"
* make SYSROOT="/opt/rootfs"
* make CFLAGS="-Wall -O2"  multicore

These samples are licenced under BSD-3-Clause
