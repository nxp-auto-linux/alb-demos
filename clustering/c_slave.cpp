// Copyright 2017 NXP
#include "c_slave.h"

c_slave::c_slave(int in_rank,c_slave_puzzle* puzzle) 
:i_process(in_rank)
{ 
    this->puzzle = puzzle;
}

void c_slave::execute(){

    while(true){
        connection.receive();
        connection.wait_receive();

        if(connection.slot_a() == SIGNAL_EXIT || connection.slot_b() == SIGNAL_EXIT)
            break;
        
        int result = puzzle->find_match(connection.slot_b(),connection.slot_a(),&used_pieces);
        assert(result != ERR_NOTFOUND);

        puzzle->is_blk_used[result] = true;
        connection.send(result,0);
    }
}