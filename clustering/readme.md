# Copyright 2017 NXP

## Description
This demo runs a puzzle-solving application in parralel on multiple hosts, showcasing the gain
in performance i.e. almost linear scalability.

## Hardware requirements
A monitor with HDMI interface connected to a s32v234 board, which we'll call the "master" from now on.
One or two addidtional s32v234 boards, which will be called "slaves". All s32v234 boards must be interconnected 
via Ethernet cable, so a switch will be required.

## Software requirements
**1.** The MPICH2 implementation of the MPI protocol, found here: http://www.mpich.org/. Install with: 
    
    ./configure --prefix=/usr/local --disable-fortran; 
    make; 
    make install

Note: MPICH2 might be included in your LinuxBSP build. Try running: 

    mpiexec --version

**2.** Every board must have a static ip adress. It is recommended that all the ip adresses belong to the
same network.

**3.** The */etc/hosts* file on each s32v234 board must contain the ip addresses and hostnames of ALL the boards.
It is recommended that each board has a different hostname (*/etc/hostname*).

**4.** Passwordless SSH must be configured. For this, on "master" generate a pair of cryptographic keys:

    ssh-keygen -t dsa 

Then copy the public key from the master to all the slaves:

    scp ~/.ssh/id_dsa.pub root@slavehostname:~/id_dsa.pub
    ssh root@slavehostname
    cat id_dsa.pub >> ~/.ssh/authorized_keys

Refer to this for more info: http://unix.stackexchange.com/questions/29386/how-do-you-copy-the-public-key-to-a-ssh-server

Afterwards, ssh from the master to each slave and from each slave to the master to make sure the hosts
get added to the known_hosts list.

**5.** Gnuplot and ImageMagick

Gnuplot can be found here: http://www.gnuplot.info/download.html

ImageMagick can be found here: http://www.imagemagick.org/script/binary-releases.php

Note: Both of these might be included in your LinuxBSP version. Try running: 

    which convert.im6
    which gnuplot

**6.** Edit lines 4-8 of run_mpi_demo.sh. 

    # IP of the boards we'll be running MPI on
    PCIE1=192.168.10.2
    PCIE2=192.168.10.3
    PCIE3=192.168.10.4
    # In summary:
    MPI_MASTER=${PCIE1}

Make sure PCIE1-3 variables are initialized with the ip addresses
of your boards. Also make sure that MPI_MASTER is initialized with the PCIEX variable that stores the ip
address of your "master" board.

Alternatively, assign 192.168.10.2/24 to your "master", 192.168.10.3/24 and 192.168.10.4/24 to
your "slave" boards.

**7.** Obtain and prepare input images

The demo requires an image that has certain attributes. The choice of it can
influence the precision of the algorithm. This is not entirely relevant, since
regardless of the image chosen, given the correct dimensions, the application
performs the same amount of work, and the purpose of this demo is performance
gains from distribution, not puzzle-solving precision.

The algorithm works by comparing the smoothness of chromatic transition from
one puzzle piece to another, under the assumption that if two pieces belong
together, there will be a low change in colour from the edge of one to the
edge of the other.

If the image is too monotone (i.e. a picture of a bright sky, or the ocean),
the demo will not be able to tell the difference between two equally fit
candidates for a position. Also, if the image is too noisy (think TV static),
the wrong candidate might be chosen as in this case our assumption that
chromatically similar pieces belong together no longer holds true.

As a guideline, we have seen good results with some images of a city, as seen
from above, and with some close-ups of integrated circuit boards. We recommend
something similar to this: https://imgur.com/IYm23vD

As for dimensions, the image should be resized such that both width and height
are multiples of 100 pixels. Also, the higher the resolution, the better. We
have found great results with 5600 x 3800.

This image should be divided into blocks of 100 x 100 pixels and shuffled, such
that only the top left block remains in place. This step is not actually
mandatory. The applications performs about the same amount of work regardless
of wether or not the image is indeed shuffled. For this, we generally use
GIMP with the 'Tile Shuffle' script mentioned here:
https://www.cartographersguild.com/showthread.php?t=4008

At last, rename the image into 'input.ppm', and make an exact copy of it at
1/10 the resolution and name it 'input.ppm.small'. GIMP can convert images
to .ppm format.

Finally, you will need a 'white.png' image, which would be used as background
when rendering the demo results.

**8.** Obtain alb-fb-apps binaries

The demo requires the fb_chess binary from the alb-fb-apps repository to
be present. Therefore it is required to clone the repository at:
https://source.codeaurora.org/external/autobsps32/alb-fb-apps, and build it.
Then copy the 'fb_chess' binary here, in the same directory as the rest of the
files.

## Running the demo

**1.** Make sure the above requirements have been met.

**2.** On master run `make` to build the application

**3.** Using scp, send the "*mpi_demo*", "*input.ppm*" and "*input.ppm.small*" to every slave

**4.** Execute run_mpi_demo.sh. You should see the graph on the monitor.

NOTE: Problems in performance or scalability may appear if the SD card used has a low read/write speed.

More info about using MPI can be found here: http://mpitutorial.com/tutorials/

