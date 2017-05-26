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

/*****************************************
* fifo_enable 
*  FIFO Control Register
*     bit 0 = enable 
*     bit 1 = testmode
*     bit 2 = rst 
******************************************/
void fifo_enable()
{
    is_fifo_enabled=1;

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
   is_fifo_enabled=0;

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
  
  

    int i;
  
    pthread_mutex_lock( &fpga_mutex );
  
    for (i=0;i<FIFO_LEN;i++)
    {
      fpgafifo[i] = 0;  
    }
    is_fifo_enabled=0;
    fifo_wr_counter=0;
    fifo_rd_counter=0;
    fpgabase[FIFORDCNTREG]=0;

    pthread_mutex_unlock( &fpga_mutex );

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

   // return(sim_framenum++);
    
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
void write_reg(int addr, unsigned int val)
{

   fpgabase[addr] = val;
   printf("Write Reg: Addr=0x%x  Val=0x%x\n",addr,val); 
  

    if (addr==REG_FIFORST && val==4)
    {
        printf("Resetting FIFO\n"); 

        fifo_disable();
        fifo_reset();
        fifo_enable();
    }
    
    if (addr == REG_SWDEBUG)
    {
        if (val==1)
        {
            printf("FIFORDCNTREG %d fifo_rd_counter %d fifo_wr_counter %d\n",
                fpgabase[FIFORDCNTREG],fifo_rd_counter,fifo_wr_counter);

        }
    }
   

}

/*****************************************
*   Read Register 
*     
******************************************/
unsigned int read_reg(int addr)
{
   int rdback;

   rdback = fpgabase[addr];
   printf("Read Reg: Addr=0x%x  Val=0x%x\n",addr,rdback); 
   return rdback;
}



/*****************************************
* fifo_numwords 
*  Reads back number of words in fifo 
******************************************/
int fifo_numwords()
{

    int nwords = 0;
    
    pthread_mutex_lock( &fpga_mutex );

    nwords=fpgabase[FIFORDCNTREG];
    pthread_mutex_unlock( &fpga_mutex );

   return nwords;

}


/*****************************************
* fifo_getdata  
*  Reads back number of words in fifo 
******************************************/
int fifo_getdata(int len, unsigned int *data)
{

   int i;

    //printf("fifo_getdata %d\n",len);

    pthread_mutex_lock( &fpga_mutex );

   for (i=0;i<len;i++) {
      //data[i] = fpgabase[FIFODATAREG];
      data[i] = fpgafifo[fifo_rd_counter%FIFO_LEN];
      
      fifo_rd_counter++;
      fpgabase[FIFORDCNTREG]--;
      //printf("i %d data 0x%x\n",i,data[i]);
      //printf("i=%d\t0x%x\t0x%x\t0x%x\t0x%x\n",i,data[0],data[1],data[2],data[3]);
   }
    pthread_mutex_unlock( &fpga_mutex );


   return 0;

}


/*******************************************
* mmap fpga register space 
* returns pointer fpgabase
*   
********************************************/
void mmap_fpga()
{
#if 0
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
#endif
   fpgabase = (unsigned int*)malloc(1024*sizeof(int));

    for (int i = 0;i<1024;i++)
        fpgabase[i] = 0;
}


/*******************************************
* Event Publish 
*  
*   Thread which handles checking if there
*   is data in the fifo, and if so sends it out
*   to the client
*******************************************/



int sim_timestamp =0;


float energy0=2600;
float energy1=3400;
float energy2=2200;
float energy3=500.0;

void makeEvent(unsigned int *word0, unsigned int *word1)
{
    unsigned int addr, pd, td, ts,chipnum,chan,quad;
    
   
    
    quad = 0;
    chipnum=rand()%2;
    chan =rand()%32;
    
    
    addr = chan + (chipnum<<5) + (quad<<7);
    
    //pd = rand() % 0xFFF;
    //td = rand() %0x3FF;
    
    int peak = rand()%10;
    
    if (peak<5)
    {
        pd =(unsigned int)(mymath->randg(150.0, energy0));
    }
    else if (peak<8)
    {
        pd =(unsigned int)(mymath->randg(150.0, energy1));
    }
    else
    {
        pd =(unsigned int)(mymath->randg(150.0, energy2));
    }
    
    
    td = (unsigned int)(mymath->randg(20.0, energy3));
    td = td& 0x3FF;
    
    
    
    ts = 0x1FFFFFFF & sim_timestamp;
    
    *word0 =addr << 22;
    *word0 = *word0 | pd;
    *word0 = *word0 | (td<<12);
    *word1 = ts;
    
    //printf("makeEvent 0x%x 0x%x 0x%x 0x%x\n",addr,pd,td,ts);
    sim_timestamp+=rand()%0xFFFF;
    
}

int sim_framelencounter = 0;

volatile int is_running=0;
volatile int detector_state = 0;

#define ST_IDLE 0
#define ST_FRAME 1

void *fake_detector(void *args)
{
    unsigned int word0, word1;
    int k;
    
    is_running = 1;
    detector_state=0;
    
    printf("Starting fake Ge detector, Idle state\n");
    
    double endtime = 0.0;
    
    while(is_running==1)
    {
        usleep(1);
    
        switch(detector_state)
        {
            case ST_IDLE:
                fpgabase[FRAMEACTIVEREG]=0;
            
            
                if (fpgabase[REG_STARTFRAME]==1)
                {
                
                    fpgabase[REG_FRAMENUM]++;
                    fpgabase[FRAMEACTIVEREG]=1;
                    detector_state=ST_FRAME;
                    sim_framelencounter=0;
                    printf("Fake detector to FRAME state\n");
                    fpgabase[REG_STARTFRAME]=0;
                    mytimer->tic();
                    
                    energy0=mymath->randf()*2000.0 + 1500.0;
                    energy1=mymath->randf()*2000.0 + 1500.0;
                    energy2=mymath->randf()*2000.0 + 1500.0;
                    energy3=mymath->randf()*300.0 + 500.0;
                    
                    
                }
                break;
            
            case ST_FRAME:
                fpgabase[FRAMEACTIVEREG]=1;
                pthread_mutex_lock( &fpga_mutex );
                for (k=0;k<100;k++)
                {
                    if (fpgabase[FIFORDCNTREG] < FIFO_LEN-2)
                    {
                        makeEvent(&word0, &word1);

                       

                        fpgafifo[fifo_wr_counter%FIFO_LEN]=word0;
                        fifo_wr_counter++;
                        fpgabase[FIFORDCNTREG]++;

                        fpgafifo[fifo_wr_counter%FIFO_LEN]=word1;
                        fifo_wr_counter++;
                        fpgabase[FIFORDCNTREG]++;
                        


                        //sim_framelencounter+=25000000;

                    }
                }
                pthread_mutex_unlock( &fpga_mutex );
                endtime = (double)(fpgabase[REG_FRAMELEN])/25000000.0;
                
                if (mytimer->isElapsed(endtime))
                {
                    detector_state=ST_IDLE;
                    fpgabase[FRAMEACTIVEREG]=0;
                    printf("Fake detector to IDLE state\n");
                    
                    printf("FIFO len %d\n",fpgabase[FIFORDCNTREG]);

                
                }
                
                
                break;
            
            default:
                detector_state = ST_IDLE;
            
            
            
        
        }//switch
    }//while

    printf("Fake detector Stopping\n");
}



/*******************************************
* Event Publish 
*  
*   Thread which handles checking if there
*   is data in the fifo, and if so sends it out
*   to the client
*******************************************/

#define EVST_IDLE 0
#define EVST_SEND 1
#define EVST_END 2


void *event_publish(void *args)
{
    int ev_state = EVST_IDLE;
    int i, quad, chipnum, chan, pd, td, ts, addr;
    void *context = zmq_ctx_new();
    void *publisher = zmq_socket(context, ZMQ_PUB);
  int size;
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
    
        switch(ev_state)
        {
            case EVST_IDLE:
                framestat = check_framestatus();
                if (framestat==1)
                {
                   ev_state = EVST_SEND;
                   printf("Frame Started...\n"); 
                   
                   
                   
                  

                //
                // Send "strt" so subscibers to fnum will get this message. 
                // send frame number after start.
                //
                  zmq_msg_init_size(&topic,4);
                  memcpy(zmq_msg_data(&topic), "strt", 4);
                  zmq_msg_send(&topic,publisher,ZMQ_SNDMORE);

                  int fnum_ = get_framenum();
                  memcpy(zmq_msg_data(&topic), &fnum_, 4);
                  zmq_msg_send(&topic,publisher,0);
                  zmq_msg_close(&topic);

		 printf("Send strt\n");
                }
            break;
            
            case EVST_SEND:
              
              numwords = fifo_numwords();      
              if (numwords > 0) {
              
                //
                // Get num ints, and events(ints/2) in fpga hw fifo
                //
                  printf("Numwords in FIFO: %d\n",numwords);
                  fifo_getdata(numwords,databuf); 
                  evttot += numwords/2; 
                //
                // Printing dbg only
                //  
                  
                  if (0)
                      for (i=0;i<numwords;i=i+2) { 
                          printf("%x\t%x\n",databuf[i+0],databuf[i+1]); 
                          quad = (databuf[i] & 0x60000000) >> 29;
                          chipnum = (databuf[i] & 0x18000000) >> 27;
                          chan = (databuf[i] & 0x07C00000) >> 22;
                          printf("Address: Quad=%d\tChipNum=%d\tChan=%d\n",quad,chipnum,chan);
                          addr = (databuf[i] & 0x7FC00000) >> 22;
                          printf("Address: %d\n",addr); 
                          pd = (databuf[i] & 0xFFF);
                          td = (databuf[i] & 0x3FF000) >> 12; 
                          ts = (databuf[i+1] & 0x1FFFFFFF);
                          printf("PD: %d\n",pd);
                          printf("TD: %d\n",td);
                          printf("Timestamp: %u\n",ts);
                      }
                      
                //
                // Send "data" message. This is for subscripbers looking for "data"
                //  init topic message
                //      
                  zmq_msg_init_size(&topic,4);
                  memcpy(zmq_msg_data(&topic), "data", 4);
                  zmq_msg_send(&topic,publisher,ZMQ_SNDMORE);

                //
                // init chunk of data message
                // Send chunk of fpga data events. "data" preceeds this chunk so data subscribers will get it
                // then close msg
                //
                  zmq_msg_init_size(&msg,numwords*4);
                  memcpy(zmq_msg_data(&msg), databuf, numwords*4);
                  int size = zmq_msg_send(&msg,publisher,0);
                  zmq_msg_close(&msg);
                  printf("Bytes Sent: %d\n",size);
                  printf("\n\n"); 

                //
                // Send "fnum" so subscibers to fnum will get this message. 
                //
                  memcpy(zmq_msg_data(&topic), "fnum", 4);
                  zmq_msg_send(&topic,publisher,ZMQ_SNDMORE);

                  int fnum_ = get_framenum();
                  memcpy(zmq_msg_data(&topic), &fnum_, 4);
                  zmq_msg_send(&topic,publisher,0);
                  zmq_msg_close(&topic);
                 printf("Send fnum\n");


                  
              }
              numwords = fifo_numwords();
              framestat = check_framestatus();
              
              if (framestat==0 && numwords==0)
              {
                 ev_state = EVST_END;
            
              }
              
            break;
            
            case EVST_END:
              printf("Frame Complete...\n");
                framelen = get_framelen(); 
                evtrate = evttot / framelen;
                
                printf("Total Events in Frame=%d\t Rate=%.1f\n",evttot,evtrate); 
                evttot = 0; 
                zmq_msg_init_size(&topic,4);
                memcpy(zmq_msg_data(&topic), "meta", 4);
                zmq_msg_send(&topic,publisher,ZMQ_SNDMORE);
                  zmq_msg_close(&topic);

                zmq_msg_init_size(&msg,4);
                curframenum = get_framenum();
                memcpy(zmq_msg_data(&msg), &curframenum, 4);
                 size = zmq_msg_send(&msg,publisher,0);
                //printf("Bytes Sent: %d\n",size);
                //printf("\n\n"); 
           
                zmq_msg_close(&msg);
                ev_state= EVST_IDLE;
            break;
            
            default:
               ev_state= EVST_IDLE;
         }//switch
    
      
        usleep(2000);

    }   //while

    pthread_exit(NULL);
}


/********************************************************
*  Main
*
********************************************************/
int main (void)
{
    pthread_t evt_thread_pid; 
      pthread_t fpga_thread_pid; 
   
    int cmdbuf[3];
    int nbytes;
    
    mymath = new erfmath();
    mytimer= new stopWatch();
    
    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    int rc = zmq_bind(responder, "tcp://*:5555");
    assert (rc == 0);

    int threadargs=0;
    int cmd, addr, value;
 
    mmap_fpga(); 

    printf("Maia Simulated Zserver...\n"); 

    fifo_disable();
    fifo_reset();
    fifo_enable();

    //start the event data publisher thread    
    pthread_create(&evt_thread_pid, NULL, event_publish, (void *)threadargs);
    pthread_create(&fpga_thread_pid, NULL,fake_detector , (void *)threadargs);

 
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
