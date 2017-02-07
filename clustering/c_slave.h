// Copyright 2017 NXP
#include "i_process.cpp"
#include "c_slave_puzzle.cpp"
#include <vector>

class c_slave : i_process{
private:
    c_slave_puzzle* puzzle;
    c_connection connection;
    std::vector<int> used_pieces;

public:
    void execute();
    c_slave(int in_rank,c_slave_puzzle* puzzle);
};