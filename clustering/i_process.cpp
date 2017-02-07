// Copyright 2017 NXP
#include "i_process.h"

i_process::i_process(int rank){
    this->rank = rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
}