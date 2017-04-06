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


#define CMD_REG_READ  0
#define CMD_REG_WRITE 1
#define CMD_START_DMA 2

#define FIFODATAREG 24
#define FIFORDCNTREG 25
#define FIFOCNTRLREG 26

#define FRAMEACTIVEREG 52
#define FRAMENUMREG 54
#define FRAMELENREG 53

volatile unsigned int *fpgabase;  //mmap'd fpga registers
int databuf[65536];




/*****************************************
* fifo_enable 
*  FIFO Control Register
*     bit 0 = enable 
*     bit 1 = testmode
*     bit 2 = rst 
******************************************/
void fifo_enable()
{
   int rdback, newval;

   //read current value of register 
   rdback = fpgabase[FIFOCNTRLREG];
   newval = rdback | 0x1;
   fpgabase[FIFOCNTRLREG] = newval;

}

/*****************************************
* fifo_disable 
*  FIFO Control Register
*     bit 0 = enable 
*     bit 1 = testmode
*     bit 2 = rst 
******************************************/
void fifo_disable()
{
   int rdback, newval;

   //read current value of register 
   rdback = fpgabase[FIFOCNTRLREG];
   newval = rdback & ~0x1;
   fpgabase[FIFOCNTRLREG] = newval;

}


/****************************************** 
* fifo reset
*  FIFO Control Register
*     bit 0 = enable 
*     bit 1 = testmode
*     bit 2 = rst 
******************************************/
void fifo_reset()
{
   int rdback, newval;

   //read current value of register 
   rdback = fpgabase[FIFOCNTRLREG];
   newval = rdback | 0x4;
   //set reset high 
   fpgabase[FIFOCNTRLREG] = newval;
   //set reset low
   newval = rdback & 0x3;
   fpgabase[FIFOCNTRLREG] = newval;


}

/*****************************************
* check_framestatus 
*    Check is frame is Active
******************************************/
int check_framestatus()
{

   //read current value of register 
   return fpgabase[FRAMEACTIVEREG];

}

/*****************************************
* get_framenum 
*   Read the current frame Number
*   Framenumber is incremented by FPGA at end of frame
* 
******************************************/
int get_framenum()
{

   //read current value of register 
   return fpgabase[FRAMENUMREG];

}


/*****************************************
* get_framelen 
*   Read the current frame length 
*   Counter is 25MHz, so 40ns period 
* 
******************************************/
float get_framelen()
{

   //read current value of register 
   return fpgabase[FRAMELENREG] / 25000000.0;

}


/*****************************************
*   Write Register
*     
******************************************/
void write_reg(int addr, int val)
{

   fpgabase[addr >> 2] = val;
   printf("Write Reg: Addr=0x%x  Val=0x%x\n",addr,val); 

}

/*****************************************
*   Read Register 
*     
******************************************/
int read_reg(int addr)
{
   int rdback;

   rdback = fpgabase[addr >> 2];
   printf("Read Reg: Addr=0x%x  Val=0x%x\n",addr,rdback); 
   return rdback;
}



/*****************************************
* fifo_numwords 
*  Reads back number of words in fifo 
******************************************/
int fifo_numwords()
{

   return fpgabase[FIFORDCNTREG];

}


/*****************************************
* fifo_getdata  
*  Reads back number of words in fifo 
******************************************/
int fifo_getdata(int len, int *data)
{

   int i;


   for (i=0;i<len;i++) {
      data[i] = fpgabase[FIFODATAREG];
      //printf("i=%d\t0x%x\t0x%x\t0x%x\t0x%x\n",i,data[0],data[1],data[2],data[3]);
   }


   return 0;

}


/*******************************************
* mmap fpga register space 
* returns pointer fpgabase
*   
********************************************/
void mmap_fpga()
{
   int fd;

   fd = open("/dev/mem",O_RDWR|O_SYNC);
   if (fd < 0) {
      printf("Can't open /dev/mem\n");
      exit(1);
   }

   fpgabase = (unsigned int *) mmap(0,255,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0x43C00000);

   if (fpgabase == NULL) {
      printf("Can't map FPGA space\n");
      exit(1);
   }

}




/*******************************************
* Event Publish 
*  
*   Thread which handles checking if there
*   is data in the fifo, and if so sends it out
*   to the client
*******************************************/
void *event_publish(void *args)
{

    int i, quad, chipnum, chan, pd, td, ts, addr;
    void *context = zmq_ctx_new();
    void *publisher = zmq_socket(context, ZMQ_PUB);
  
    int rc = zmq_bind(publisher,"tcp://*:5556");  
    assert (rc == 0);
    zmq_msg_t  topic, msg;
    int numwords, prevframestat, framestat, curframenum;
    float evtrate, framelen;
    int evttot;


    printf("Starting Data Thread...\n");
    framestat = 0;
    curframenum = get_framenum();
    evttot = 0;

    while (1) {
        prevframestat = framestat;
        framestat = check_framestatus(); 
        //printf("PrevFrameStat=%d\tFrameStatus=%d\n",prevframestat,framestat); 
        if (framestat == 1) {
           if ((prevframestat == 0) && (framestat == 1))           
              printf("Frame Started...\n"); 
           numwords = fifo_numwords();
           if (numwords > 0) {
              //printf("Numwords in FIFO: %d\n",numwords);
              fifo_getdata(numwords,databuf); 
              evttot += numwords/2; 
              for (i=0;i<numwords;i=i+2) { 
                  //printf("%x\t%x\n",databuf[i+0],databuf[i+1]); 
                  //quad = (databuf[i] & 0x60000000) >> 29;
                  //chipnum = (databuf[i] & 0x18000000) >> 27;
                  //chan = (databuf[i] & 0x07C00000) >> 22;
                  //printf("Address: Quad=%d\tChipNum=%d\tChan=%d\n",quad,chipnum,chan);
                  addr = (databuf[i] & 0x7FC00000) >> 22;
                  //printf("Address: %d\n",addr); 
                  pd = (databuf[i] & 0xFFF);
                  td = (databuf[i] & 0x3FF000) >> 12; 
                  ts = (databuf[i+1] & 0x1FFFFFFF);
                  //printf("PD: %d\n",pd);
                  //printf("TD: %d\n",td);
                  //printf("Timestamp: %u\n",ts);
              }
              zmq_msg_init_size(&topic,4);
              memcpy(zmq_msg_data(&topic), "data", 4);
              zmq_msg_send(&topic,publisher,ZMQ_SNDMORE);

              zmq_msg_init_size(&msg,numwords*4);
              memcpy(zmq_msg_data(&msg), databuf, numwords*4);
              int size = zmq_msg_send(&msg,publisher,0);
              //printf("Bytes Sent: %d\n",size);
              //printf("\n\n"); 
           
              zmq_msg_close(&msg);
           }
        }
        else
            if ((prevframestat == 1) && (framestat == 0)) {          
                printf("Frame Complete...\n");
                framelen = get_framelen(); 
                evtrate = evttot / framelen;
                
                printf("Total Events in Frame=%d\t Rate=%.1f\n",evttot,evtrate); 
                evttot = 0; 
                zmq_msg_init_size(&topic,4);
                memcpy(zmq_msg_data(&topic), "meta", 4);
                zmq_msg_send(&topic,publisher,ZMQ_SNDMORE);

                zmq_msg_init_size(&msg,4);
                curframenum = get_framenum();
                memcpy(zmq_msg_data(&msg), &curframenum, 4);
                int size = zmq_msg_send(&msg,publisher,0);
                //printf("Bytes Sent: %d\n",size);
                //printf("\n\n"); 
           
                zmq_msg_close(&msg);
             } 

        usleep(2000);

    }   

    pthread_exit(NULL);
}



/********************************************************
*  Main
*
********************************************************/
int main (void)
{
    pthread_t evt_thread_pid; 
    int cmdbuf[3];
    int nbytes;
    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    int rc = zmq_bind(responder, "tcp://*:5555");
    assert (rc == 0);

    int threadargs=0;
    int cmd, addr, value;
 
    mmap_fpga(); 

    printf("Maia Zserver...\n"); 

    fifo_disable();
    fifo_reset();
    fifo_enable();

    //start the event data publisher thread    
    pthread_create(&evt_thread_pid, NULL, event_publish, (void *)threadargs);

 
    while (1) {
        if ((nbytes = zmq_recv(responder, cmdbuf, sizeof(int)*3, 0)) < 0) {
            break;
        }
        printf("recv: %i bytes, %x %x %x\n", nbytes, cmdbuf[0], cmdbuf[1], cmdbuf[2]);
        cmd = cmdbuf[0];
        addr = cmdbuf[1];
        value = cmdbuf[2];
        
        switch (cmd) {
            case CMD_REG_READ:
                // do read register
                value = read_reg(addr);
                cmdbuf[2] = value;
                zmq_send(responder, cmdbuf, sizeof(cmdbuf), 0);
                break;
                
            case CMD_REG_WRITE:
                // do write
                write_reg(addr, value);
                zmq_send(responder, cmdbuf, sizeof(cmdbuf), 0);
                break;
               
            default:
                cmdbuf[0] = 0xdead;
                cmdbuf[1] = 0xdead;
                cmdbuf[2] = 0xdead;
                zmq_send(responder, cmdbuf, sizeof(cmdbuf), 0);
                break;
                
        }
        
    }
    return 0;
}
