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

#define CMD_REG_READ  0
#define CMD_REG_WRITE 1
#define CMD_START_DMA 2

#define FIFODATAREG 0x18
#define FIFORDCNTREG 0x19
#define FIFOCNTRLREG 0x1a

#define FRAMEACTIVEREG 0x34
//#define FRAMENUMREG 0x36
//#define FRAMELENREG 0x35
#define FRAMELENREG 0xD8
#define FRAMENUMREG 0xD4


#define REG_STARTFRAME 0xD0
#define REG_FRAMENUM 0xD8
#define REG_FRAMELEN 0xD4

#define REG_FIFORST 0x68

//num xfers  
#define REG_BURSTLEN 0x8C
//in bytes
#define REG_BUFFSIZE 0x90

#define REG_RATE 0x98

#define REG_SWDEBUG 0xE1

volatile unsigned int *fpgabase;  //mmap'd fpga registers
unsigned int databuf[65536];

#define FIFO_LEN 65536
volatile unsigned int fpgafifo[FIFO_LEN];
volatile unsigned int fifo_wr_counter;
volatile unsigned int fifo_rd_counter;
volatile int is_fifo_enabled = 0;



int sim_framenum = 0;

pthread_mutex_t fpga_mutex=PTHREAD_MUTEX_INITIALIZER;


erfmath *mymath=0;

stopWatch *mytimer=0;

#define ZMQ_DATA_PORT   "5556"
#define ZMQ_CNTL_PORT   "5555"
#define TOPIC_DATA      "data"
#define TOPIC_META      "meta"

#define MAX_MSG_WORDS 65536

void *ctrl_context=0;
void *ctrl_socket=0;

void *data_context = 0;
void *data_socket;



namespace zmqapsclient 
{


int initCommandClient(char *connect_str)
{        
    char ctstr[256];

    printf("initCommandClient  \n");
    ctrl_context = zmq_ctx_new();
    ctrl_socket = zmq_socket(ctrl_context,ZMQ_REQ);
    
    
    sprintf(ctstr,"%s:%s",connect_str,ZMQ_CNTL_PORT);
        
    int rc = zmq_connect(
    	ctrl_socket,
        ctstr);
         
    //!!assert (rc == 0);
    if (rc!=0)
    {
    	//int errx =  zmq_errno();
    	printf("Connection error %s \n",zmq_strerror(errno));


    }
     printf("initCommandClient done: %s \n",ctstr);


}

int initDataclient(char *connect_str)
{
    char ctstr[256];
     printf("initDataclient  \n");
    
    data_context = zmq_ctx_new();
    data_socket = zmq_socket(data_context,ZMQ_SUB);
    sprintf(ctstr,"%s:%s",connect_str,ZMQ_DATA_PORT);
    int rc = zmq_connect(
        data_socket,
        ctstr);
        
    //!!assert (rc == 0);
  
    zmq_setsockopt(
        data_socket,
        ZMQ_SUBSCRIBE, 
        TOPIC_DATA,
        4);
        
    zmq_setsockopt(
        data_socket,
        ZMQ_SUBSCRIBE, 
        TOPIC_META,
        4);
     printf("initDataclient done  %s\n",ctstr);

}


int cntrlRecv(int maxnints, unsigned int *cmd_buf)
{

        int nbytes = zmq_recv(ctrl_socket,cmd_buf,sizeof(int)*maxnints, 0);
        
        return(nbytes);
        

}

int cntrlSend(int nints, unsigned int *payload)
{
    zmq_send(ctrl_socket, payload, sizeof(int)*nints, 0);
}

void destroy(void)
{
    zmq_ctx_destroy(data_context);
    zmq_ctx_destroy(ctrl_context);    
}

void write(unsigned int addr, unsigned int value)
{
    unsigned int payload[3];
    unsigned int ret_buf[3];
    
    
    payload[0] = 0x1;
    payload[1] = addr;
    payload[2] = value;
    

    cntrlSend(3,payload);
    cntrlRecv(3,ret_buf);

}

int read(unsigned int addr)
{

    unsigned int payload[3];
    unsigned int ret_buf[3];
    
    
    payload[0] = 0x0;
    payload[1] = addr;
    payload[2] = 0x0;
    

    cntrlSend(3,payload);
    cntrlRecv(3,ret_buf);
    return(ret_buf[2]);
}


void setBurstlen(unsigned int value)
{
    write(REG_BURSTLEN,value);
    printf("Burst length set to %d transfers\n", value);
}

void setBufSize(unsigned int value)
{
    write(REG_BUFFSIZE,value);
    printf("Buffer size set to %d bytes\n",  value);
}

void setRate(float rate)
{
    unsigned int n = (unsigned int)( (400e6 / (rate*1e6)) - 1);
    write(REG_RATE,n);
    printf("Rate set to %.2f MHz\n",(400e6/((float)n+1) / 1e6));
}

void setFrameLen(float value)
{
    write(REG_FRAMELEN,(unsigned int)(value*25000000.0));
    printf("Frame length set to %d secs\n",value);
}


void startFrame(void)
{
    write(REG_STARTFRAME,1);
    printf("New Frame Initiated...\n" );
}


unsigned int getFrameNum(void)
{
    unsigned int val = read(REG_FRAMENUM);
    printf("Frame Num=%d\n", val);
    return(val);
}


void fifoReset(void)
{
    printf("Resetting FIFO..\n");
    write(REG_FIFORST,4);
    write(REG_FIFORST,1);
}


void triggerData(void)
{
    unsigned int payload[3];
    unsigned int ret_buf[3];
    
    
    payload[0] = 0x2;
    payload[1] = 0x0;
    payload[2] = 0x0;
    

    cntrlSend(3,payload);
    cntrlRecv(3,ret_buf);
    
}

void getOneFrameToFile(char *filename, unsigned int maxevents)
{
    unsigned int nbr = 0;
    unsigned int totallen=0;

    char fullname[256];
    
        printf(" getOneFrameToFile\n");

//        start = datetime.datetime.now()
    sprintf(fullname, "%s.dat",filename);
    FILE *fd = fopen(
        fullname,
        "wb");
        
        
    printf("getOneFrameToFile\n");
    zmq_msg_t  topic, msg;
    void *msg_content;
    zmq_msg_init_size (&topic,4);
    zmq_msg_init_size(&msg,MAX_MSG_WORDS*4);
    int numwords;
    
    while(1)
    {
        printf(" to zmq_msg_recv\n");

        zmq_msg_recv(
            &topic,
            data_socket, 
            0);
        printf(" done zmq_msg_recv\n");

        const char *msg_address  = (const char*)zmq_msg_data(&topic);
        // we expect 4 chars. make 5th one a 0 to term a c string
        char topicstr[5];
        topicstr[4] =0;
        memcpy(topicstr,msg_address,4);
        
        
    printf("topic %s\n",topicstr);
            
        if (strcmp(topicstr,TOPIC_META)==0)
        {
                printf(" meta\n");

            numwords = zmq_msg_recv(&msg,data_socket,0); 
            char meta[16];
            unsigned int *metaint;
            sprintf(fullname, "%s.txt",filename);
            FILE *fd2 = fopen(
                fullname,
                "w");
                
            metaint =  (unsigned int*)zmq_msg_data(&msg);  
            sprintf(meta,"%x",*metaint);
            fputs(
                meta,
                fd2);
            fclose(fd2);
        
            break;
        }        
        else if (strcmp(topicstr,TOPIC_DATA)==0)
        {                
            printf("Event data received\n");

            numwords = zmq_msg_recv(&msg,data_socket,0);
            numwords=numwords/sizeof(int);
            void *dataptr = zmq_msg_data(&msg);
            fwrite(dataptr,sizeof(int),numwords,fd);
                       
            totallen = totallen + numwords;
            printf("Msg Num: %d, Msg len: %d, Tot len: %d\n",
                nbr,numwords,totallen);

            if (maxevents!=-1 && totallen > maxevents)
            {
                break;
            }

            nbr++;
 
        }  //   if (strcmp(msg_address,TOPIC_DATA)
        else
        {
            printf("Unexpected topic %s\n",topicstr);
        }
    }//while
           
    zmq_msg_close(&topic);
    zmq_msg_close(&msg);
        
    fclose(fd);
           
    printf("Total Image Size: %d (%d bytes), %d parts\n",
        totallen,
        totallen*sizeof(int),
        nbr
        );
    
      
}
        


};



void *dateReceiveDaemon(void *args)
{
    printf(" dateReceiveDaemon\n");

    zmqapsclient::initDataclient("tcp://127.0.0.1");
    printf("done initDataclient\n");
    
    int numfiles = 10;
    int filenum = 0;
    
    char fname[256];
    
    for (int k =0; k<numfiles;k++)
    {
    sprintf(fname,"test_%03d",k);
      

    zmqapsclient::getOneFrameToFile(fname, 0xffffffff);
    }
    printf("end dateReceiveDaemon\n");

}



/********************************************************
*  Main
*
********************************************************/
int main (void)
{
    char gg[200];
    //printf("hit return\n");
    //gets(gg);
    
    
    pthread_t evt_thread_pid; 
    int threadargs=0;

    pthread_create(&evt_thread_pid, NULL, dateReceiveDaemon, (void *)threadargs);

    zmqapsclient::initCommandClient("tcp://127.0.0.1");
    
    
    printf("To read \n");
    int aa = zmqapsclient::read(0);
    printf("Started client, ret %d\n",aa);
    
    
    zmqapsclient::setFrameLen(0.2);

    
    for (int k =0; k<10;k++)
    {
        printf("hit to start frame\n");
        gets(gg);
        zmqapsclient::startFrame();
    }

}
