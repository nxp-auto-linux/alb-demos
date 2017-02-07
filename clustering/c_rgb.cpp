// Copyright 2017 NXP
#include "c_rgb.h"

unsigned int rgb::manhattan_distance(rgb* in_a, rgb* in_b){
    return abs((int)in_a->r - (int)in_b->r) + abs((int)in_a->g - (int)in_b->g) + abs((int)in_a->b - (int)in_b->b);
}
