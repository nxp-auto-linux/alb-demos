// Copyright 2017 NXP
#pragma once
#include "c_connection.cpp"

class i_process{
protected:
    int rank;
    int world_size;

public:
    i_process(int in_ranke);
    virtual void execute() = 0;
};