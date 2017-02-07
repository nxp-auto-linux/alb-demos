// Copyright 2017 NXP
#include "i_puzzle.h"
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <time.h>
#include <unistd.h>

class c_master_puzzle : i_puzzle{
private:
    int* final_solution;
    int blocks_solved;

    std::clock_t startup_moment = std::clock();

    rgb* original_smaller_image;
    rgb* updated_smaller_image;
    int smaller_width;
    int smaller_height;
    int smaller_blk_edge_length;
    bool* is_smaller_blk_updated;
    void load_smaller_image(const char* filename);

    bool is_solved();
    bool blk_is_used(int blk_id);
    int find_next_solvable();
    void convert_solution_to_image(rgb* o_image);
    void convert_and_save_as_ppm(const char* filename);
    void transfer_block(rgb* origin_img,int origin_blk,rgb* final_img,int final_blk);
    int find_block_above(int in_blk_id);
    int find_block_left(int in_blk_id);

public:
    c_master_puzzle(const char* filename,int in_block_edge_length);
    void report_progress(int number_of_boards);
    void update_smaller_image();
    void write_smaller_image(const char* filename);

friend class c_master;
};
