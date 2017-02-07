// Copyright 2017 NXP
#include "c_connection.h"

void c_connection::bind_to(int rank){
    this->rank = rank;
}

void c_connection::kill(){
    send(SIGNAL_EXIT,SIGNAL_EXIT);
}

void c_connection::send(int int_a,int int_b){
    send_buffer.slot_a = int_a;
    send_buffer.slot_b = int_b;
    MPI_Isend((void*)(&send_buffer),2,MPI_INT,rank,0,MPI_COMM_WORLD,&send_request);
}

void c_connection::receive(){
    MPI_Irecv((void*)(&receive_buffer),2,MPI_INT,rank,0,MPI_COMM_WORLD,&receive_request);
}

void c_connection::wait_receive(){
    MPI_Status receive_status;
    MPI_Wait(&receive_request,&receive_status);
}

bool c_connection::check_receive(){
    int flag;
    MPI_Status status;
    MPI_Test(&receive_request,&flag,&status);
    return flag;
}

void c_connection::set_status(int status){
    this->status = status;
}

int c_connection::get_status(){
    return status;
}
    
int c_connection::slot_a(){
    return receive_buffer.slot_a;
}
    
int c_connection::slot_b(){
    return receive_buffer.slot_b;
}