// Copyright 2017 NXP
#include "i_process.h"
#include "c_master_puzzle.cpp"
#include <ctime>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

class Semaphore {
public:
    Semaphore (int count_ = 0)
        : count(count_) {}

    inline void notify()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while(count == 0){
            cv.wait(lock);
        }
        count--;
    }

private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    int count;
};


struct ThreadData {
    Semaphore *sem;
    std::atomic_bool is_running;
    std::atomic_int progress;
    std::atomic_int next_progress;
    c_master_puzzle *puzzle;
    int jobs;

    ThreadData(Semaphore *_sem, c_master_puzzle* puzzle);

    void set_state(bool running);
    void set_progress(int progress);

    void run(int nr_boards);
};

class c_master : i_process{
private:

    int collisions = 0;
    std::thread job;

    Semaphore sem;
    ThreadData td;

    c_master_puzzle* puzzle;
    c_connection* connections;
    int number_of_connections;
    int* who_solves_what;

    int nb_boards;

    void init_connections();
    void read_result(int i);
    void give_work(int i);

public:
    void execute();
    c_master(int rank,c_master_puzzle* puzzle);
    ~c_master();
};
