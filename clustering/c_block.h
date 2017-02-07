// Copyright 2017 NXP
#pragma once
#include "c_rgb.h"
#include <assert.h>

class c_block{
private:
    rgb *top,*right,*bottom,*left;
    int edge_length;
public:
    void init(int width,int height, int edge_length,rgb* first_pixel);
    unsigned int compare_with(c_block* blk_left,c_block* blk_above, unsigned int current_min_dist);
    unsigned int compare_with_above(c_block* blk_above, unsigned int current_min_dist);
    unsigned int compare_with_leftside(c_block* blk__left, unsigned int current_min_dist);
};
