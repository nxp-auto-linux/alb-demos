// Copyright 2017 NXP
#define SIGNAL_EXIT  -10
#define STATUS_DEAD    2
#define STATUS_BUSY    3
#define STATUS_READY   4

struct buffer_t{
    int slot_a;
    int slot_b;
};

class c_connection{
private:
    int rank = 0;
    int status = STATUS_READY;
    MPI_Request send_request;
    MPI_Request receive_request;
    struct buffer_t send_buffer;
    struct buffer_t receive_buffer;

public:
    void kill();
    void send(int int_a,int int_b);
    void receive();
    void wait_receive();
    bool check_receive();

    int slot_a();
    int slot_b();
    int get_status();
    void set_status(int status);
    void bind_to(int rank);
};