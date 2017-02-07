// Copyright 2017 NXP
#include "i_puzzle.h"
#include <algorithm>
#include <vector>

class c_slave_puzzle : public i_puzzle{
private:
    bool* is_blk_used;
    int find_match(int blk_left_id,int blk_above_id,std::vector<int>* used_pieces);

public:
    c_slave_puzzle(const char* filename,int in_block_edge_length);

friend class c_slave;
};