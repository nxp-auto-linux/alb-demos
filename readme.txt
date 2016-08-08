Alb-demos is a collection of C samples over the POSIX standard,
meant to illustrate some base functionalities over the Linux
BSP.
These samples had been structured in a modular way in order to
be not just easy to read but even easy to understand by the
users of our product.
Because the processor of the s32v234-evb platform is an ARM
Cortex-A53, we use the ARM compiler to generate the binary files
of our code examples.
Each sample will have a  Makefile in whitch will set up a (CC)
variable with the path of the ARM compiler from the user station.
