// Copyright 2017 NXP
#include "c_master_puzzle.h"

int c_master_puzzle::find_next_solvable(){
    int i, j, t, blk;
    for (t = 0; t <= blocks_per_line + blocks_per_column - 2; t++) {
        for (i = 0; i < blocks_per_line; i++) {
            j = t - i;
            if (j >= 0 && j < blocks_per_column) {
                blk = j * blocks_per_line + i;
                if (final_solution[blk] != -1)
                    continue;
                if (blk % blocks_per_line != 0 && final_solution[blk - 1] < 0)
                    continue;
                if (blk > blocks_per_line && final_solution[blk - blocks_per_line] < 0)
                    continue;
                final_solution[blk] = -2;
                return blk;
            }
        }
    }

    return -1;
}

void c_master_puzzle::convert_and_save_as_ppm(const char* filename){
    FILE* file = nullptr;
    file = fopen(filename,"w");
    assert(file != nullptr);

    fprintf(file,"P3\n");
	fprintf(file,"#OpenMPI Demo output image \n");
	fprintf(file,"%d %d\n",width,height);
	fprintf(file,"%d\n",255);

    rgb* output_image = new rgb[width*height];
    convert_solution_to_image(output_image);

	int number_of_pixels = width * height;
	for(int i=0; i < number_of_pixels; i++){
        fprintf(file,"%d\n%d\n%d\n",output_image[i].r,
                output_image[i].g,output_image[i].b);
	}

	fclose(file);
}

/*Load a smaller image to be displayed on the screen*/
void c_master_puzzle::load_smaller_image(const char* filename){
    FILE* file = nullptr;
    file = fopen(filename,"r");
    assert(file != nullptr);

    /*"P3" and "Creator GIMP..." strings.*/
    char* not_used_char = new char[100];
	fgets(not_used_char,100,file);
	fgets(not_used_char,100,file);
    delete not_used_char;

	fscanf(file,"%d",&smaller_width);
	fscanf(file,"%d",&smaller_height);
    smaller_blk_edge_length = smaller_width/blocks_per_line;

    /*Highest value for one channel: 255*/
    int not_used_int;
	fscanf(file,"%d",&not_used_int);

    char crlf;
    fread(&crlf,1,1,file);

    original_smaller_image = new rgb[smaller_width * smaller_height];  
    for(int i=0; i<smaller_width * smaller_height; i++)
            fread(&original_smaller_image[i].r,sizeof(rgb),1,file);  

    updated_smaller_image = new rgb[smaller_width * smaller_height];
    memset(updated_smaller_image,255,smaller_width * smaller_height * sizeof(rgb));

    fclose(file);
}

void c_master_puzzle::update_smaller_image(){
    for(int i=0; i<number_of_blocks; i++){
        if(final_solution[i] < 0)
            continue;
        if(is_smaller_blk_updated[i] == true)
            continue;

        transfer_block(original_smaller_image,final_solution[i],updated_smaller_image,i);
        is_smaller_blk_updated[i] = true;
        
    }
}

void c_master_puzzle::transfer_block(rgb* origin_img,int origin_blk,rgb* final_img,int final_blk){


    int small_blocks_per_line = smaller_width/smaller_blk_edge_length;
    
    rgb* this_origin_pixel = origin_img;
    this_origin_pixel += (origin_blk)/small_blocks_per_line * smaller_blk_edge_length * smaller_width;
    this_origin_pixel += (origin_blk % small_blocks_per_line) * smaller_blk_edge_length;

    rgb* this_final_pixel = final_img;
    this_final_pixel += (final_blk)/small_blocks_per_line * smaller_blk_edge_length * smaller_width;
    this_final_pixel += (final_blk % small_blocks_per_line) * smaller_blk_edge_length;

    for(int y=0; y<smaller_blk_edge_length; y++){
        for(int x=0; x<smaller_blk_edge_length; x++){
            *this_final_pixel = *this_origin_pixel;
            this_origin_pixel++;
            this_final_pixel++;
        }

        this_final_pixel += smaller_width - smaller_blk_edge_length;
        this_origin_pixel += smaller_width - smaller_blk_edge_length;
    }
}

void c_master_puzzle::write_smaller_image(const char* filename){
    FILE* file = nullptr;
    file = fopen(filename,"wb");
    assert(file != nullptr);

    fprintf(file,"P6\n");
	fprintf(file,"#OpenMPI Demo output image \n");
	fprintf(file,"%d %d\n",smaller_width,smaller_height);
	fprintf(file,"%d\n",255);

    fwrite(updated_smaller_image,sizeof(rgb),smaller_width * smaller_height,file);
	fclose(file);
}


void c_master_puzzle::convert_solution_to_image(rgb* o_image){
    for(int i=0; i<number_of_blocks; i++){
        transfer_block(image,final_solution[i],o_image,i);
    }
}

bool c_master_puzzle::is_solved(){
    if(blocks_solved < number_of_blocks)
        return false;
    else
        return true;
}

c_master_puzzle::c_master_puzzle(const char* filename,int in_block_edge_length)
:i_puzzle(filename,in_block_edge_length)
{
    final_solution = new int[number_of_blocks];
    is_smaller_blk_updated = new bool[number_of_blocks];
    for(int i=0; i<number_of_blocks; i++){
        final_solution[i] = -1;
        is_smaller_blk_updated[i] = false;
    }

    blocks_solved = 1;
    final_solution[0] = 0;

    std::string small_filename = std::string(filename);
    small_filename += std::string(".small");
    load_smaller_image(small_filename.c_str());
}

bool c_master_puzzle::blk_is_used(int blk_id){
    for(int i=0; i<number_of_blocks; i++)
        if(final_solution[i] == blk_id)
            return true;
    return false;
}

int c_master_puzzle::find_block_above(int in_blk_id){
    int block_above = in_blk_id - blocks_per_line;
    if(block_above < 0)
        return ERR_NOTFOUND;
    else
        return block_above;
}

int c_master_puzzle::find_block_left(int in_blk_id){
    if(in_blk_id % blocks_per_line == 0)
        return ERR_NOTFOUND;
    else
        return in_blk_id - 1;
}   

void c_master_puzzle::report_progress(int number_of_boards){
    static std::string* filename = nullptr;

    if(filename == nullptr){
        time_t now = time(0);
        struct tm time_struct = *localtime(&now);

        char* buffer = new char[5];
        sprintf(buffer,"%d",number_of_boards);
        std::string nb_boards = std::string(buffer) + std::string("_board_");
        sprintf(buffer,"%d",time_struct.tm_hour);
        std::string hour = std::string(buffer) + std::string(":");
        sprintf(buffer,"%d",time_struct.tm_min);
        std::string minute = std::string(buffer) + std::string(":");
        sprintf(buffer,"%d",time_struct.tm_sec);
        std::string second = std::string(buffer);
        delete buffer;

        filename = new std::string;
        *filename = std::string("demo_") + nb_boards + hour + minute + second;
    }

    FILE* report_fd = fopen(filename->c_str(),"a");

    double time_since_startup = double(std::clock() - startup_moment) / CLOCKS_PER_SEC;
    int progress_since_startup = double((double)blocks_solved/(double)number_of_blocks) * 100;
    fprintf(report_fd,"%f %d\n",time_since_startup,progress_since_startup);

    fclose(report_fd);

}
