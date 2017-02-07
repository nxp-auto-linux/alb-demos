// Copyright 2017 NXP
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "c_block.h"

#define ERR_NOTFOUND -1

class i_puzzle{
protected:
    rgb* image;
    int height;
    int width;

    c_block* blocks;
    int number_of_blocks;
    int blocks_per_line;
    int block_edge_length;
    int blocks_per_column;

    rgb* blk_to_pixel_pointer(rgb* in_image,int in_block_id);
    int find_match(int blk_left,int blk_above);
    void load_from_ppm(const char* filename);
    void load_from_ppm_raw(const char* filename);

public:
    i_puzzle(const char* filename,int in_block_edge_length);
};