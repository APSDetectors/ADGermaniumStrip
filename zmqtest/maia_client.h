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

#include "erfmath.h"
#include "stopWatch.h"






class maia_client{

    public:

    maia_client();
    ~maia_client();
    
    int initCommandClient(char *connect_str);
    int initDataclient(char *connect_str);
    int cntrlRecv(int maxnints, unsigned int *cmd_buf);
    int cntrlSend(int nints, unsigned int *payload);
    void destroy(void);
    void write(unsigned int addr, unsigned int value);
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





    #define ZMQ_DATA_PORT   "5556"
    #define ZMQ_CNTL_PORT   "5555"
    #define TOPIC_DATA      "data"
    #define TOPIC_META      "meta"

    #define MAX_MSG_WORDS 65536


    erfmath *mymath;
    stopWatch *mytimer;
    void *ctrl_context;
    void *ctrl_socket;
    void *data_context;
    void *data_socket;





};


void *maia_dateReceiveDaemon(void *args);








