#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/mman.h>
#include <math.h>
#include <fcntl.h>


#include "maia_client.h"

maia_except::maia_except(char *msg) : std::exception() 
{
    strcpy(errormsg,msg);
}

const char* maia_except::what() const throw() 
{
    return(errormsg);
}


maia_client::maia_client() {

   mytimer = new stopWatch();
   ctrl_context=0;
   ctrl_socket=0;
   data_context=0;
   data_socket=0;

   is_rcv_waiting=false;
   is_writing_file = false;
    dbg_timeout_counter=0;
}



maia_client::~maia_client() {
	
    delete(mytimer);
    //!! what of the contextz from zmq. need to destrpy on proper threads or else we crash
}
    

void maia_client::stopDataWaitRcv(void)
{
   is_rcv_waiting=false; 
   is_writing_file=false;
}

void maia_client::setConnectString(char *connect_str) {

    strcpy(this->connect_str, connect_str);
}

int maia_client::initCommandClient(void)
{        
    char ctstr[256];

    printf("initCommandClient  \n");
    ctrl_context = zmq_ctx_new();
    ctrl_socket = zmq_socket(ctrl_context,ZMQ_REQ);
    
    
    
      
    int sock_timeout_ms = CMD_SOCK_RCV_TIMEOUT_MS;   
     
    zmq_setsockopt(
       ctrl_socket,
       ZMQ_RCVTIMEO,
       &sock_timeout_ms,
       4);
       
       
    
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

int maia_client::initDataclient(void)
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
        TOPIC_STRT,
        4);




    zmq_setsockopt(
        data_socket,
        ZMQ_SUBSCRIBE,
        TOPIC_FNUM,
        4);
 
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
    
    int sock_timeout_ms = DATA_SOCK_RCV_TIMEOUT_MS;   
     
    zmq_setsockopt(
       data_socket,
       ZMQ_RCVTIMEO,
       &sock_timeout_ms,
       4);
       
             
    topic = (zmq_msg_t*)malloc(sizeof(zmq_msg_t)); 
    msg = (zmq_msg_t*)malloc(sizeof(zmq_msg_t)); 
             
    zmq_msg_init_size (topic,4);
    zmq_msg_init_size(msg,MAX_MSG_WORDS*4);

        
     printf("initDataclient done  %s\n",ctstr);

}


int maia_client::cntrlRecv(int maxnints, unsigned int *cmd_buf)
{

        int nbytes = zmq_recv(ctrl_socket,cmd_buf,sizeof(int)*maxnints, 0);
        
        return(nbytes);
        

}

int maia_client::cntrlSend(int nints, unsigned int *payload)
{
    zmq_send(ctrl_socket, payload, sizeof(int)*nints, 0);
}

void maia_client::destroyCtrl(void)
{
    zmq_close(ctrl_socket);
    zmq_ctx_destroy(ctrl_context);
    
    ctrl_context=0;
    ctrl_socket=0;
}


void maia_client::destroyData(void)
{   
        
    zmq_msg_close(topic);
    zmq_msg_close(msg);
        
  
    free(topic);
    free(msg);
    zmq_close(data_socket);
    zmq_ctx_destroy(data_context);
    data_context=0;
    data_socket=0;
}
int maia_client::write(unsigned int addr, unsigned int value)
{
    unsigned int payload[3];
    unsigned int ret_buf[3];
    
    
    payload[0] = 0x1;
    payload[1] = addr;
    payload[2] = value;
    

    cntrlSend(3,payload);
    int nbytes_rcv = cntrlRecv(3,ret_buf);
   
    return(nbytes_rcv);  
}

int maia_client::read(unsigned int addr, int *nbytes_rcv)
{

    unsigned int payload[3];
    unsigned int ret_buf[3];
    ret_buf[2] = 0;
    
    payload[0] = 0x0;
    payload[1] = addr;
    payload[2] = 0x0;
    

    cntrlSend(3,payload);   
    *nbytes_rcv  = cntrlRecv(3,ret_buf);
  
    return(ret_buf[2]);
}

int maia_client::read(unsigned int addr)
{

    unsigned int payload[3];
    unsigned int ret_buf[3];
    ret_buf[2] = 0;
    
    payload[0] = 0x0;
    payload[1] = addr;
    payload[2] = 0x0;
    

    cntrlSend(3,payload);   
    int nbytes_rcv  = cntrlRecv(3,ret_buf);
    
    if (nbytes_rcv==-1)
    {
        
        throw maia_except("ZMQ receive timed out\n");
    } 
  
    return(ret_buf[2]);
}



void maia_client::setBurstlen(unsigned int value)
{
    write(REG_BURSTLEN,value);
    printf("Burst length set to %d transfers\n", value);
}

void maia_client::setBufSize(unsigned int value)
{
    write(REG_BUFFSIZE,value);
    printf("Buffer size set to %d bytes\n",  value);
}

void maia_client::setRate(float rate)
{
    unsigned int n = (unsigned int)( (400e6 / (rate*1e6)) - 1);
    write(REG_RATE,n);
    printf("Rate set to %.2f MHz\n",(400e6/((float)n+1) / 1e6));
}

void maia_client::setFrameLen(float value)
{
    write(REG_FRAMELEN,(unsigned int)(value*25000000.0));
    printf("Frame length set to %d secs\n",value);
}


void maia_client::startFrame(void)
{
    write(REG_STARTFRAME,1);
    printf("New Frame Initiated...\n" );
}


unsigned int maia_client::getFrameNum(void)
{   
    unsigned int val = read(REG_FRAMENUM);
    printf("Frame Num=%d\n", val);
    return(val);
}


void maia_client::fifoReset(void)
{
    printf("Resetting FIFO..\n");
    write(REG_FIFORST,4);
    write(REG_FIFORST,1);
}


void maia_client::triggerData(void)
{
    unsigned int payload[3];
    unsigned int ret_buf[3];
    
    
    payload[0] = 0x2;
    payload[1] = 0x0;
    payload[2] = 0x0;
    

    cntrlSend(3,payload);
    cntrlRecv(3,ret_buf);
    
}

void maia_client::getOneFrameToFile(char *filename, unsigned int maxevents)
{
    unsigned int nbr = 0;
    unsigned int totallen=0;
  
    int num_ints_rcvd;
    int message_rcv_type;
    unsigned int max_ints;
    int frame_number;
    
    char fullname[256];
    
        printf(" getOneFrameToFile\n");

//        start = datetime.datetime.now()
    sprintf(fullname, "%s.dat",filename);
    FILE *fd = fopen(
        fullname,
        "wb");
        
        
    printf("getOneFrameToFile\n");
    
    void *msg_content;
 
    int numwords;
    is_writing_file=true;
    printf(" to zmq_msg_recv\n");

    while(is_writing_file)
    {
        
      
        max_ints=data_buffer_size;
   
    
        getOneMessage(
            data_buffer, 
            &num_ints_rcvd,//num ints in mesage, -1 on err or no data
            &message_rcv_type,// 1 for meta, 0 for data
            &frame_number,
            max_ints//max ints to rcv
            );

        
        if (num_ints_rcvd>0)
        {        
            printf(" done getOneMessage\n");

          
            
            if (message_rcv_type==1)
            {
                printf(" meta2\n");

                
                char meta[16];
                
                sprintf(fullname, "%s.txt",filename);
                FILE *fd2 = fopen(
                    fullname,
                    "w");

                 
                sprintf(meta,"%x",data_buffer[0]);
                fputs(
                    meta,
                    fd2);
                fclose(fd2);

                break;
            }        
            else if (message_rcv_type==0)
            {                
                printf("Event data received2\n");

              
                
                fwrite(data_buffer,sizeof(int),num_ints_rcvd,fd);

                totallen = totallen + num_ints_rcvd;
                printf("Msg Num: %d, Msg len: %d, Tot len: %d\n",
                    nbr,num_ints_rcvd,totallen);

                if (maxevents!=-1 && totallen > maxevents)
                {
                    break;
                }

                nbr++;

            }  //   if (strcmp(msg_address,TOPIC_DATA)
            else
            {
                printf("Unexpected topic \n");
            }
        }//if rcv_stat
    }//while
           
  
        
    fclose(fd);
           
    printf("Total Image Size: %d (%d bytes), %d parts\n",
        totallen,
        totallen*sizeof(int),
        nbr
        );
    
      
}
        



void maia_client::getOneMessage(
    unsigned int *databuffer, 
    int *num_ints_rcvd,//num ints in mesage
    int *message_rcv_type,// 1 for meta, 0 for data
    int *frame_number,
    unsigned int max_ints//max ints to rcv
    )
{
    printf(" getOneMessage\n");
   
 
    void *msg_content;


    int numwords,numevents;
    
    is_rcv_waiting=true;
    printf(" to zmq_msg_recv\n");
    
    *num_ints_rcvd=-1;
    *message_rcv_type=0;
    
    while(is_rcv_waiting)
    {
        

        int rcv_stat = zmq_msg_recv(
            topic,
            data_socket, 
            0);
        
        if (rcv_stat!=-1)
        {        
            printf(" done zmq_msg_recv\n");

            const char *msg_address  = (const char*)zmq_msg_data(topic);
            // we expect 4 chars. make 5th one a 0 to term a c string
            char topicstr[5];
            topicstr[4] =0;
            memcpy(topicstr,msg_address,4);


            printf("topic %s\n",topicstr);
            
            if (strcmp(topicstr,TOPIC_META)==0)
            {
                printf(" meta\n");

                numwords = zmq_msg_recv(msg,data_socket,0); 
                char meta[16];
                unsigned int *metaint;
               

                metaint =  (unsigned int*)zmq_msg_data(msg);  
                memcpy(databuffer,metaint,sizeof(int));
                *num_ints_rcvd=1;
                *message_rcv_type=message_meta;
                *frame_number=*metaint;
                break;
            }        
            else if (strcmp(topicstr,TOPIC_DATA)==0)
            {   
                //
                // we received topic "data"
                // now receive chubnk of ints. copy into output buffer.
                //             
                printf("Event data received\n");
              
                
                numwords = zmq_msg_recv(msg,data_socket,0) / sizeof(int);
                
                void *dataptr = zmq_msg_data(msg);
                if (numwords>max_ints)
                    numwords=max_ints;
                    
               
                memcpy(databuffer,dataptr,numwords*sizeof(int));
                *num_ints_rcvd=numwords;
                *message_rcv_type=message_data;
                *frame_number = -1;
              
            
                break;
              
                

            }  //   if (strcmp(msg_address,TOPIC_DATA)            
            else if (strcmp(topicstr,TOPIC_FNUM)==0)
            {   
                //
                // latest server should send "fnum" topuc after "data" topuic. so listen for it.
                // 
               numwords = zmq_msg_recv(msg,data_socket,0)/sizeof(int);

               
                  
               int *frnum;
	       frnum=    (int*)zmq_msg_data(msg);
                *num_ints_rcvd=numwords;
                *message_rcv_type=message_fnum;
                *frame_number = *frnum;
    		break;

            }
            else if (strcmp(topicstr,TOPIC_STRT)==0)
            {   
                //
                // latest server should send "fnum" topuc after "data" topuic. so listen for it.
                // 
               numwords = zmq_msg_recv(msg,data_socket,0)/sizeof(int);

               
                  
               int *frnum=    (int*)zmq_msg_data(topic);
                *num_ints_rcvd=numwords;
                *message_rcv_type=message_strt;
                *frame_number = *frnum;
               break; 
            }
            else
            {
                printf("Unexpected topic %s\n",topicstr);
                *num_ints_rcvd=-1;
                *message_rcv_type=message_undef;
                
            }
        }//if rcv_stat
        else
        {
               // printf("timeout \n");
               dbg_timeout_counter++;

        }
    }//while
   
   printf(" leaing get One message \n");
    
      
}
        



void *maia_dateReceiveDaemon(void *args)
{
    printf(" dateReceiveDaemon\n");

    maia_client *myclient=(maia_client*)args;
    
    myclient->initDataclient();
    printf("done initDataclient\n");
    
    int numfiles = 10;
    int filenum = 0;
    
    char fname[256];
    
    for (int k =0; k<numfiles;k++)
    {
        sprintf(fname,"test_%03d",k);
        myclient->getOneFrameToFile(fname, 0xffffffff);
    }
    printf("end dateReceiveDaemon\n");

}

#ifdef MAIA_TEST_MAIN

/********************************************************
*  Main
*
********************************************************/
int main (void)
{
    char gg[200];
    //printf("hit return\n");
    //gets(gg);
    

    maia_client *myclient = new maia_client();
    myclient->setConnectString("tcp://127.0.0.1");

        
    pthread_t evt_thread_pid; 
    int threadargs=0;

    pthread_create(
        &evt_thread_pid, 
        NULL, 
        maia_dateReceiveDaemon, 
        (void *)myclient);

    myclient->initCommandClient();
    
    
    printf("To read \n");
    int aa = myclient->read(0);
    printf("Started client, ret %d\n",aa);
    
    
    myclient->setFrameLen(0.2);

    
    for (int k =0; k<10;k++)
    {
        printf("hit to start frame\n");
        gets(gg);
        myclient->startFrame();
    }

}

#endif
