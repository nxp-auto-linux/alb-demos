// Copyright 2017 NXP
#include "c_master.h"
#include <thread>
#include <string>
#include <functional>

#define collisions_limit 5

ThreadData::ThreadData(Semaphore *_sem, c_master_puzzle* _puzzle):
    sem(_sem), is_running(true), next_progress(0), puzzle(_puzzle)
{
}

void ThreadData::run(int nr_boards)
{
    while(true) {
        sem->wait();
        progress = next_progress + 0;

        puzzle->report_progress(nr_boards);
        puzzle->update_smaller_image();
        puzzle->write_smaller_image("result_so_far.ppm");
        system("./demo_run.sh");

        if (progress >= jobs) {
            std::cerr << "Done !" << std::endl;
            break;
        }
    }
}

void ThreadData::set_state(bool running)
{
    is_running = running;
}

void ThreadData::set_progress(int _progress)
{
    next_progress = _progress;
}

static void plot_statistics(ThreadData *td, int nr_boards)
{
    td->run(nr_boards);
}

c_master::c_master(int rank,c_master_puzzle* puzzle)
    :i_process(rank),td(&sem, puzzle), job(plot_statistics, &td, (world_size-1)/4 +1)
{
    nb_boards = (world_size-1)/4 +1;
    this->puzzle = puzzle;
    init_connections();
    collisions_per_slave = new int[world_size-1];
    for(int i=0; i<world_size-1;i++)
	collisions_per_slave[i]=0;
}

c_master::~c_master()
{
}

void c_master::execute(){

    int step_value = puzzle->number_of_blocks / 20;
    int threshold = step_value;
    std::clock_t image_last_time = std::clock();
    std::clock_t image_this_time;

    std::clock_t progress_last_time = std::clock();
    std::clock_t progress_this_time;

    td.jobs = puzzle->number_of_blocks;

    while(!puzzle->is_solved()){
        if (puzzle->blocks_solved >= threshold) {
            threshold += step_value;
            td.set_progress(puzzle->blocks_solved);
            // Notify background thread to plot the statistics
            sem.notify();
        }

        for(int i=0; i<number_of_connections; i++){

            if(connections[i].get_status() == STATUS_DEAD){
                continue;
            }

            if(connections[i].get_status() == STATUS_READY){
                give_work(i);
                continue;
            }

            if(connections[i].get_status() == STATUS_BUSY){
                if(connections[i].check_receive()){
                    connections[i].set_status(STATUS_READY);
                    read_result(i);
                    continue;
                }
            }
        }

        image_this_time = std::clock();
        if(double(image_this_time - image_last_time) / CLOCKS_PER_SEC > 2.5){
            image_last_time = image_this_time;
        }

        progress_this_time = std::clock();
        if(double(progress_this_time - progress_last_time) / CLOCKS_PER_SEC > 0.4){
            puzzle->report_progress(nb_boards);
            progress_last_time = progress_this_time;
        }

    }

    std::cout << "Unusable answers received from slaves:" << collisions << "\n";

    for(int i=0; i<number_of_connections; i++){
        if(connections[i].get_status() == STATUS_DEAD)
            continue;

        connections[i].kill();
    }

    td.set_progress(puzzle->blocks_solved);
    sem.notify();
    td.set_state(false);
    sem.notify();
    job.join();
}

void c_master::init_connections(){
        connections = new c_connection[world_size-1];
        who_solves_what = new int[world_size-1];
        number_of_connections = world_size-1;

        for(int i=0; i<world_size-1; i++){
            connections[i].bind_to(i+1);
            who_solves_what[i] = -1;
        }
}


void c_master::read_result(int i){

    int result = connections[i].slot_a();
    if(puzzle->blk_is_used(result)){
	
        collisions_per_slave[i]++;
	if(collisions_per_slave[i] < collisions_limit){

        collisions++;

        int blk_above_id = puzzle->find_block_above(who_solves_what[i]);
        int blk_left_id = puzzle->find_block_left(who_solves_what[i]);

        int blk_above = puzzle->final_solution[blk_above_id];
        int blk_left = puzzle->final_solution[blk_left_id];

        if(blk_above_id == ERR_NOTFOUND) blk_above = ERR_NOTFOUND;
        if(blk_left_id == ERR_NOTFOUND) blk_left = ERR_NOTFOUND;

        connections[i].send(blk_above,blk_left);
        connections[i].set_status(STATUS_BUSY);
        connections[i].receive();
        return;
	}
    }
    puzzle->final_solution[who_solves_what[i]] = result;
    puzzle->blocks_solved++;
    collisions_per_slave[i] = 0;
}

void c_master::give_work(int i){

    int solvable = puzzle->find_next_solvable();

    if(solvable == ERR_NOTFOUND){
        return;
        connections[i].kill();
        connections[i].set_status(STATUS_DEAD);
    }
    else{

        int blk_above_id = puzzle->find_block_above(solvable);
        int blk_left_id = puzzle->find_block_left(solvable);

        int blk_above = puzzle->final_solution[blk_above_id];
        int blk_left = puzzle->final_solution[blk_left_id];

        if(blk_above_id == ERR_NOTFOUND) blk_above = ERR_NOTFOUND;
        if(blk_left_id == ERR_NOTFOUND) blk_left = ERR_NOTFOUND;

        connections[i].send(blk_above,blk_left);
        connections[i].set_status(STATUS_BUSY);
        who_solves_what[i] = solvable;
        connections[i].receive();
    }

}
