// Copyright 2017 NXP
#include "c_slave_puzzle.h"

c_slave_puzzle::c_slave_puzzle(const char* filename,int in_block_edge_length)
:i_puzzle(filename,in_block_edge_length)
{ 
    is_blk_used = new bool[number_of_blocks];
    for(int i=0; i< number_of_blocks; i++)
        is_blk_used[i] = false;
    is_blk_used[0] = true;

 }

int c_slave_puzzle::find_match(int blk_left_id,int blk_above_id,std::vector<int>* used_pieces){
    c_block* leftside_block = nullptr;
    c_block* above_block = nullptr;
    unsigned int l_dist, a_dist, tmp_dist;

    if(blk_left_id != -1)
        leftside_block = &blocks[blk_left_id];
    if(blk_above_id != -1)
        above_block = &blocks[blk_above_id];

    unsigned int smallest_distance = UINT_MAX;
    int best_match = -1;
    unsigned int best_left_dist = UINT_MAX;
    unsigned int best_above_dist = UINT_MAX;

    for(int i=1; i<number_of_blocks; i++){
        if(i == blk_left_id || i == blk_above_id)
            continue;

        if(is_blk_used[i] == true)
            continue;

        l_dist = 0;
        a_dist = 0;

        if (leftside_block != nullptr)
            l_dist = blocks[i].compare_with_leftside(leftside_block, smallest_distance);
        if (l_dist == UINT_MAX)
            continue;

        if (above_block != nullptr)
            a_dist = blocks[i].compare_with_above(above_block, smallest_distance - l_dist);
        if (a_dist == UINT_MAX)
            continue;

        tmp_dist = l_dist + a_dist;
        if (tmp_dist < smallest_distance) {
            smallest_distance = tmp_dist;
            best_left_dist = l_dist;
            best_above_dist = a_dist;
            best_match = i;
        } else if (tmp_dist == smallest_distance) {
            if (abs((int)l_dist - (int)a_dist) < abs((int)best_left_dist - (int)best_above_dist)) {
                best_left_dist = l_dist;
                best_above_dist = a_dist;
                best_match = i;
            }
        }

    }

    return best_match;
}
