// Copyright 2017 NXP
#include "i_puzzle.h"

i_puzzle::i_puzzle(const char* filename,int in_block_edge_length)
{
    this->block_edge_length = in_block_edge_length;    
    load_from_ppm_raw(filename);
}

rgb* i_puzzle::blk_to_pixel_pointer(rgb* image,int blk_id){
    rgb* first_pixel_in_block = image;
    first_pixel_in_block += (blk_id)/blocks_per_line * block_edge_length * width;
    first_pixel_in_block += (blk_id % blocks_per_line) * block_edge_length;
    return first_pixel_in_block;
}

void i_puzzle::load_from_ppm_raw(const char* filename){

    FILE* file = nullptr;
    file = fopen(filename,"r");
    assert(file != nullptr);

    /*"P3" and "Creator GIMP..." strings.*/
    char* not_used_char = new char[100];
	fgets(not_used_char,100,file);
	fgets(not_used_char,100,file);
    delete not_used_char;

	fscanf(file,"%d",&width);
	fscanf(file,"%d",&height);

    /*Highest value for one channel: 255*/
    int not_used_int;
	fscanf(file,"%d",&not_used_int);

    char crlf;
    fread(&crlf,1,1,file);

    blocks_per_line = width/block_edge_length;    
    blocks_per_column = height/block_edge_length;
    number_of_blocks = blocks_per_line*blocks_per_column;
    blocks = new c_block[number_of_blocks];
    

    int block_rows = height/block_edge_length; 
    int pixels_per_block_row = blocks_per_line * block_edge_length * block_edge_length; 

    int current_block_id = 0;
    image = new rgb[width*block_edge_length];  

    for(int k=0; k<block_rows; k++){
        for(int i=0;i<pixels_per_block_row;i++){
            fread(&image[i].r,sizeof(rgb),1,file);  
        }

        for(int j=0;j<blocks_per_line;j++){
            blocks[current_block_id].init(width,block_edge_length,block_edge_length
            ,blk_to_pixel_pointer(image,j));
            current_block_id++;
        }
    }

    delete image; //Pixels of large image no longer useful

    fclose(file);
}

void i_puzzle::load_from_ppm(const char* filename){
    FILE* file = nullptr;
    file = fopen(filename,"r");
    assert(file != nullptr);

    /*"P3" and "Creator GIMP..." strings.*/
    char* not_used_char = new char[100];
	fgets(not_used_char,100,file);
	fgets(not_used_char,100,file);
    delete not_used_char;

	fscanf(file,"%d",&width);
	fscanf(file,"%d",&height);

    /*Highest value for one channel: 255*/
    int not_used_int;
	fscanf(file,"%d",&not_used_int);

    image = new rgb[width*height];
    unsigned int uint_r,uint_g,uint_b;
    for(int i=0; i<width*height; i++){
        
        fscanf(file,"%d",&uint_r);
		fscanf(file,"%d",&uint_g);
		fscanf(file,"%d",&uint_b);

        image[i].r = static_cast<unsigned char>(uint_r);
        image[i].g = static_cast<unsigned char>(uint_g);
        image[i].b = static_cast<unsigned char>(uint_b);
    }

    fclose(file);
}