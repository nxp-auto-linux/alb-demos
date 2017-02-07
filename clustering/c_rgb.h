// Copyright 2017 NXP
#pragma once
#include <math.h>

class rgb{
public:
    unsigned char r,g,b;
    static unsigned int manhattan_distance(rgb* in_a, rgb* in_b);
};