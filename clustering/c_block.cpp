// Copyright 2017 NXP
#include <climits>
#include "c_block.h"

void c_block::init(int width,int height, int edge_length,rgb* first_pixel){
    assert(edge_length >= 3);

    this->edge_length = edge_length;
    int number_of_pixels = 4*edge_length -4;

    top = new rgb[number_of_pixels];
    right = top + edge_length-1;
    bottom = right + edge_length-1;
    left = bottom + edge_length-1;

    rgb* this_pixel_from_image = first_pixel;
    for(int i=0; i<edge_length; i++){
        top[i] = *this_pixel_from_image;
        this_pixel_from_image++;
    }

    this_pixel_from_image--;
    for(int i=0; i<edge_length; i++){
        right[i] = *this_pixel_from_image;
        this_pixel_from_image += width;
    }
    
    this_pixel_from_image -= width;
    for(int i=0; i<edge_length; i++){
        bottom[i] = *this_pixel_from_image;
        this_pixel_from_image--;
    }

    this_pixel_from_image++;
    for(int i=0; i<edge_length; i++){
        left[i] = *this_pixel_from_image;
        this_pixel_from_image -= width;
    }
}

unsigned int c_block::compare_with_above(c_block* blk_above, unsigned int current_min_dist)
{
    unsigned int manhattan_distance_on_edge = 0;
    for(int i=0; i<edge_length; i++){
        manhattan_distance_on_edge += 
        rgb::manhattan_distance(this->top +i,blk_above->bottom + edge_length -i -1);
        if (manhattan_distance_on_edge > current_min_dist)
            return UINT_MAX;
    }

    return manhattan_distance_on_edge;
}

unsigned int c_block::compare_with_leftside(c_block* blk_left, unsigned int current_min_dist)
{
    unsigned int manhattan_distance_on_edge = 0;
    for(int i=0; i<edge_length; i++){
        manhattan_distance_on_edge += 
        rgb::manhattan_distance(blk_left->right +i,this->left + edge_length -i -1);
        if (manhattan_distance_on_edge > current_min_dist)
            return UINT_MAX;
    }

    return manhattan_distance_on_edge;
}

unsigned int c_block::compare_with(c_block* blk_left,c_block* blk_above, unsigned int current_min_dist){
    unsigned int distance1 = 0, distance2 = 0;

    assert(blk_left != nullptr || blk_above != nullptr);

    if (blk_left != nullptr) {
        distance1 = compare_with_leftside(blk_left, current_min_dist);
        if (distance1 == UINT_MAX) {
            return UINT_MAX;
        }
    }

    if (blk_above != nullptr) {
	assert(current_min_dist >= distance1);
        distance2 = compare_with_above(blk_above, current_min_dist - distance1);
        if (distance2 == UINT_MAX) {
            return UINT_MAX;
        }
    }

    return distance1 + distance2;
}
