#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/mman.h>
//#include "zhelpers.h"
#include <math.h>
#include <fcntl.h>
#include <exception>

#include "stopWatch.h"



#ifndef _MAIA_CLIENT_H
#define _MAIA_CLIENT_H



#define ZMQ_DATA_PORT   "5556"
#define ZMQ_CNTL_PORT   "5555"
#define TOPIC_DATA      "data"
#define TOPIC_META      "meta"
#define TOPIC_FNUM      "fnum"
#define TOPIC_STRT      "strt"



#define MAX_MSG_WORDS 65536
#define DATA_SOCK_RCV_TIMEOUT_MS 100
#define CMD_SOCK_RCV_TIMEOUT_MS 10000


class maia_except : public std::exception
{
    public:
    
    maia_except(char *msg);
    virtual const char* what() const throw();
    
    private:
    char errormsg[256];
};

    
class maia_client{

    public:

    maia_client();
    ~maia_client();
    
    void stopDataWaitRcv(void);
    void setConnectString(char *connect_str);
    int initCommandClient(void);
    int initDataclient(void);
    int cntrlRecv(int maxnints, unsigned int *cmd_buf);
    int cntrlSend(int nints, unsigned int *payload);
    void destroyCtrl(void);
    void destroyData(void);
    int write(unsigned int addr, unsigned int value);
    int read(unsigned int addr, int *nbytes_rcv);
    int read(unsigned int addr);
    void setBurstlen(unsigned int value);
    void setBufSize(unsigned int value);
    void setRate(float rate);
    void setFrameLen(float value);
    void startFrame(void);
    unsigned int getFrameNum(void);
    void fifoReset(void);
    void triggerData(void);
    void getOneFrameToFile(char *filename, unsigned int maxevents);
    void getOneMessage(
        unsigned int *databuffer, 
        int *num_ints_rcvd,//num ints in mesage
        int *message_rcv_type,// 
        int *frame_number,
        unsigned int max_ints//max ints to rcv
        );


    enum message_type {
        message_meta,
        message_data,
        message_fnum,
        message_strt,
        message_undef
    };

  
      int dbg_timeout_counter;

  
    protected:
    
    
    enum fpga_cmds {
        CMD_REG_READ = 0,
        CMD_REG_WRITE = 1,
        CMD_START_DMA = 2
        };
        

    enum fpga_regs {
        FIFODATAREG  = 0x18,
        FIFORDCNTREG  = 0x19,
        FIFOCNTRLREG  = 0x1a,
        FRAMEACTIVEREG  = 0x34,
        FRAMELENREG  = 0xD8,
        FRAMENUMREG  = 0xD4,
        REG_STARTFRAME  = 0xD0,
        REG_FRAMENUM  = 0xD8,
        REG_FRAMELEN  = 0xD4,
        REG_FIFORST  = 0x68,
        REG_BURSTLEN  = 0x8C,
        REG_BUFFSIZE  = 0x90,
        REG_RATE  = 0x98,
        REG_SWDEBUG  = 0xE1
    };

    stopWatch *mytimer;
    void *ctrl_context;
    void *ctrl_socket;
    void *data_context;
    void *data_socket;
    
    volatile bool is_rcv_waiting;
    volatile bool is_writing_file;

    char connect_str[256];

    zmq_msg_t  *topic;
    zmq_msg_t  *msg;
    
    enum {data_buffer_size = 262144};
    unsigned int data_buffer[262144];


};


void *maia_dateReceiveDaemon(void *args);



#endif




