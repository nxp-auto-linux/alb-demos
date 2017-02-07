// Copyright 2017 NXP
#include <mpi.h>
#include <iostream>
#include "c_rgb.cpp"
#include "c_block.cpp"
#include "i_puzzle.cpp"
#include "c_master.cpp"
#include "c_slave.cpp"

int main(int argc,char* argv[])
{
    MPI_Init(NULL,NULL);
    int this_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &this_rank);

    if(this_rank == 0)
        (new c_master(this_rank,new c_master_puzzle(argv[1],atoi(argv[2]))))->execute();
    else
        (new c_slave(this_rank,new c_slave_puzzle(argv[1],atoi(argv[2]))))->execute();

    MPI_Finalize();
    return 0;
}