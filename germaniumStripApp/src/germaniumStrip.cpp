/* germaniumStrip.cpp
 *
 * This is a driver for a geulated area detector.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  March 20, 2008
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <cantProceed.h>
#include <iocsh.h>

#include "ADDriver.h"
#include <epicsExport.h>
#include "germaniumStrip.h"

static const char *driverName = "germaniumStrip";

/* Some systems don't define M_PI in math.h */
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

  


 //   pRaw_->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);


/** Controls the shutter */
void germaniumStrip::setShutter(int open)
{
    int shutterMode;

    getIntegerParam(ADShutterMode, &shutterMode);
    if (shutterMode == ADShutterModeDetector) {
        /* Geulate a shutter by just changing the status readback */
        setIntegerParam(ADShutterStatus, open);
    } else {
        /* For no shutter or EPICS shutter call the base class method */
        ADDriver::setShutter(open);
    }
}


static void geTaskC(void *drvPvt)
{
    germaniumStrip *pPvt = (germaniumStrip *)drvPvt;

    pPvt->geTask();
}

/** This thread calls computeImage to compute new image data and does the callbacks to send it to higher layers.
  * It implements the logic for single, multiple or continuous acquisition. */
void germaniumStrip::geTask()
{
    int status = asynSuccess;
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    int acquire=0;
    NDArray *pImage;
    double acquireTime, acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;
    const char *functionName = "geTask";

    //this->lock();
    /* Loop forever */
    
    myclient->initDataclient();
    is_running_deamon=true;
    
    while(is_running_deamon)
    {
    
        // frame is a long acq from ge strip det. it makes manuy images. one message from zmq is one image.
        // each image is around 64 k or less. 100s images per frame.     
        // we need to associate frame number w/ eqach message. we alter server on maia to send frame nub
        // every message.
        
        //!! this should not be hard coded...
        int ndims = 1;
        size_t dims[3];
        dims[0] = 65536;
        
        NDArray *image;
        
        printf("to alloc  \n");
        image = this->pNDArrayPool->alloc(ndims,dims, NDUInt32, 0, NULL);
        unsigned int*databuffer = (unsigned int*)image->pData;
        
        
        
        int num_ints_rcvd;
        int is_meta_nis_data;
        int frame_number;
        unsigned int max_ints=65536;
        
        //wait for message.
         printf("to  myclient->getOneMessage \n");

        
        myclient->getOneMessage(
            databuffer, 
            &num_ints_rcvd,//num ints in mesage
            &is_meta_nis_data,// 1 for meta, 0 for data
            &frame_number,
            max_ints//max ints to rcv
            );
        
        int num_events = num_ints_rcvd/2;
    
        //NDAttribute (const char *pName, const char *pDescription, NDAttrSource_t sourceType, const char *pSource, NDAttrDataType_t dataType, void *pValue)
    
         printf("done  myclient->getOneMessage  \n");      
         
         printf("to  image->pAttributeList->add \n");
         
        //add new attr to img, if not already there. if there, it updates values         
        image->pAttributeList->add(
            "maia_frame_number", 
            "ge strip det hw frame num",
             NDAttrInt32, 
             &frame_number);
               
        //add new attr to img, if not already there. if there, it updates values         
        image->pAttributeList->add(
            "maia_type", 
            "DATA =0, META=1, for raw Ge Strip Det data",
             NDAttrInt32, 
             &is_meta_nis_data);
      
     
                   
        //add new attr to img, if not already there. if there, it updates values         
        image->pAttributeList->add(
            "maia_num_events", 
            "num raw events in this image",
             NDAttrInt32, 
             &num_events);
      
         
         
         printf("to  getIntegerParam(NDArrayCounter \n");
         
         getIntegerParam(NDArrayCounter, &imageCounter);
          imageCounter++;
          setIntegerParam(NDArrayCounter, imageCounter);
          
          
          getIntegerParam(ADNumImagesCounter, &numImagesCounter);
          numImagesCounter++;
          setIntegerParam(ADNumImagesCounter, numImagesCounter);
         
         
        
         /* Put the frame number and time stamp into the buffer */
        image->uniqueId = imageCounter;
        image->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
        
         printf("to  updateTimeStamp \n");
        updateTimeStamp(&image->epicsTS);
    
    
     getIntegerParam(NDArrayCallbacks, &arrayCallbacks);

        if (arrayCallbacks) {
          /* Call the NDArray callback */
          /* Must release the lock here, or we can get into a deadlock, because we can
           * block on the plugin lock, and the plugin can be calling us */
          //!!this->unlock();
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: calling imageData callback\n", driverName, functionName);
           printf("to  doCallbacksGenericPointer \n");         
          doCallbacksGenericPointer(image, NDArrayData, 0);
         //!! this->lock();
        }

     

    printf("to   callParamCallbacks\n");
        /* Call the callbacks to update any changes */
        callParamCallbacks();
        image->release();
        


        /* If we are acquiring then sleep for the acquire period minus elapsed time. */
    }//while deamon
    printf("to  myclient->destroyData \n");
    myclient->destroyData();
 //!! this->unlock();
  
}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire, ADColorMode, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus germaniumStrip::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int adstatus;
    int acquiring;
    int imageMode;
    int isconn;
    bool istat;
    const char *functionName = "germaniumStrip";
    
    printf("writeInt32  \n");
    
    asynStatus status = asynSuccess;

    /* Ensure that ADStatus is set correctly before we set ADAcquire.*/
    getIntegerParam(ADStatus, &adstatus);
    getIntegerParam(ADAcquire, &acquiring);
    getIntegerParam(ADImageMode, &imageMode);
   
    getIntegerParam(GeIsConnected, &isconn);
   
 
    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setIntegerParam(function, value);

    if (function==GeConnZMQ && isconn==0)
    {
        printf("GeConnZMQ  \n");
        setIntegerParam(GeIsConnected,1);
        
        myclient->initCommandClient();
        printf("To read \n");
        int aa = myclient->read(0);
        printf("Started client, ret %d\n",aa);
        
        
        
         istat = (epicsThreadCreate("GeDetTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)geTaskC,
                                this) == NULL);
        
     
        
        if (istat) {
            printf("%s:%s epicsThreadCreate failure for image task\n",
                driverName, functionName);

        }
        
    }
    else if (function==GeDisconnZMQ && isconn==1)
    {
    
        printf("GeDisconnZMQ  \n");
        myclient->destroyCtrl();
        is_running_deamon=false;
        myclient->stopDataWaitRcv();
        
        setIntegerParam(GeIsConnected,0);

    }
    else if (function==ADAcquire && value==1 && isconn==1)
    {
        printf(" ADAcquire \n");
        myclient->startFrame();
    }
   
    

    else if (function < FIRST_GE_DETECTOR_PARAM) 
        status = ADDriver::writeInt32(pasynUser, value);
    
    
    
    printf(" callParamCallbacks \n");
    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:writeInt32 error, status=%d function=%d, value=%d\n",
              driverName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:writeInt32: function=%d, value=%d\n",
              driverName, function, value);
    return status;
}


/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters, including ADAcquireTime, ADGain, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus germaniumStrip::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int isconn;
    printf(" writeFloat64 \n");
    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(function, value);


    getIntegerParam(GeIsConnected, &isconn);
   
    if (function == ADAcquireTime && isconn==1)
    {
    
            printf("ADAcquireTime  \n");

        myclient->setFrameLen((float)value);
    }
    
  
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_GE_DETECTOR_PARAM) 
            status = ADDriver::writeFloat64(pasynUser, value);
   
    printf("callParamCallbacks  \n");
    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:writeFloat64 error, status=%d function=%d, value=%f\n",
              driverName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:writeFloat64: function=%d, value=%f\n",
              driverName, function, value);
    return status;
}


/** Report status of the driver.
  * Prints details about the driver if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >0 then driver details are printed.
  */
void germaniumStrip::report(FILE *fp, int details)
{

    fprintf(fp, "Geulation detector %s\n", this->portName);
    if (details > 0) {
        int nx, ny, dataType;
        getIntegerParam(ADSizeX, &nx);
        getIntegerParam(ADSizeY, &ny);
        getIntegerParam(NDDataType, &dataType);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    /* Invoke the base class method */
    ADDriver::report(fp, details);
}

/** Constructor for germaniumStrip; most parameters are geply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to compute the geulated detector data,
  * and sets reasonable default values for parameters defined in this class, asynNDArrayDriver and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxSizeX The maximum X dimension of the images that this driver can create.
  * \param[in] maxSizeY The maximum Y dimension of the images that this driver can create.
  * \param[in] dataType The initial data type (NDDataType_t) of the images that this driver will create.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
germaniumStrip::germaniumStrip(const char *portName, int maxSizeX, int maxSizeY, NDDataType_t dataType,
                         int maxBuffers, size_t maxMemory, int priority, int stackSize)

    : ADDriver(portName, 1, NUM_GE_DETECTOR_PARAMS, maxBuffers, maxMemory,
               0, 0, /* No interfaces beyond those set in ADDriver.cpp */
               0, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize),
      pRaw_(NULL)

{
    int status = asynSuccess;
    char versionString[20];
    const char *functionName = "germaniumStrip";

    /* Create the epicsEvents for signaling to the geulate task when acquisition starts and stops */
    startEventId_ = epicsEventCreate(epicsEventEmpty);
    if (!startEventId_) {
        printf("%s:%s epicsEventCreate failure for start event\n",
            driverName, functionName);
        return;
    }
    stopEventId_ = epicsEventCreate(epicsEventEmpty);
    if (!stopEventId_) {
        printf("%s:%s epicsEventCreate failure for stop event\n",
            driverName, functionName);
        return;
    }

   
    /* Set some default values for parameters */
    status =  setStringParam (ADManufacturer, "ANL-BNL");
    status |= setStringParam (ADModel, "Germanium Strip Detector");
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", 
                  DRIVER_VERSION, DRIVER_REVISION, DRIVER_MODIFICATION);
   //!! setStringParam(NDDriverVersion, versionString);
   //!! setStringParam(ADSDKVersion, versionString);
   //!! setStringParam(ADSerialNumber, "No serial number");
   //!! setStringParam(ADFirmwareVersion, "No firmware");

    status |= setIntegerParam(ADMaxSizeX, maxSizeX);
    status |= setIntegerParam(ADMaxSizeY, maxSizeY);
    status |= setIntegerParam(ADMinX, 0);
    status |= setIntegerParam(ADMinY, 0);
    status |= setIntegerParam(ADBinX, 1);
    status |= setIntegerParam(ADBinY, 1);
    status |= setIntegerParam(ADReverseX, 0);
    status |= setIntegerParam(ADReverseY, 0);
    status |= setIntegerParam(ADSizeX, maxSizeX);
    status |= setIntegerParam(ADSizeY, maxSizeY);
    status |= setIntegerParam(NDArraySizeX, maxSizeX);
    status |= setIntegerParam(NDArraySizeY, maxSizeY);
    status |= setIntegerParam(NDArraySize, 0);
    status |= setIntegerParam(NDDataType, dataType);
    status |= setIntegerParam(ADImageMode, ADImageContinuous);
    status |= setDoubleParam (ADAcquireTime, .001);
    status |= setDoubleParam (ADAcquirePeriod, .005);
    status |= setIntegerParam(ADNumImages, 100);

    createParam("GeConnString",       asynParamOctet, &GeConnString);
    status |= setStringParam (GeConnString, DEFAULT_ZMQ_CONN_STR);
    
    createParam("GeConnZMQ",    asynParamInt32, &GeConnZMQ);
    setIntegerParam(GeConnZMQ, 0);

    createParam("GeDisconnZMQ",    asynParamInt32, &GeDisconnZMQ);
    setIntegerParam(GeDisconnZMQ, 0);

    createParam("GeIsConnected",    asynParamInt32, &GeIsConnected);
    setIntegerParam(GeIsConnected, 0);




    myclient= new maia_client();
    
    myclient->setConnectString(DEFAULT_ZMQ_CONN_STR);
    
    if (status) {
        printf("%s: unable to set camera parameters\n", functionName);
        return;
    }

   
}

/** Configuration command, called directly or from iocsh */
extern "C" int germaniumStripConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType,
                                 int maxBuffers, int maxMemory, int priority, int stackSize)
{
    new germaniumStrip(portName, maxSizeX, maxSizeY, (NDDataType_t)dataType,
                    (maxBuffers < 0) ? 0 : maxBuffers,
                    (maxMemory < 0) ? 0 : maxMemory, 
                    priority, stackSize);
    return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg germaniumStripConfigArg0 = {"Port name", iocshArgString};
static const iocshArg germaniumStripConfigArg1 = {"Max X size", iocshArgInt};
static const iocshArg germaniumStripConfigArg2 = {"Max Y size", iocshArgInt};
static const iocshArg germaniumStripConfigArg3 = {"Data type", iocshArgInt};
static const iocshArg germaniumStripConfigArg4 = {"maxBuffers", iocshArgInt};
static const iocshArg germaniumStripConfigArg5 = {"maxMemory", iocshArgInt};
static const iocshArg germaniumStripConfigArg6 = {"priority", iocshArgInt};
static const iocshArg germaniumStripConfigArg7 = {"stackSize", iocshArgInt};
static const iocshArg * const germaniumStripConfigArgs[] =  {&germaniumStripConfigArg0,
                                                          &germaniumStripConfigArg1,
                                                          &germaniumStripConfigArg2,
                                                          &germaniumStripConfigArg3,
                                                          &germaniumStripConfigArg4,
                                                          &germaniumStripConfigArg5,
                                                          &germaniumStripConfigArg6,
                                                          &germaniumStripConfigArg7};
static const iocshFuncDef configgermaniumStrip = {"germaniumStripConfig", 8, germaniumStripConfigArgs};
static void configgermaniumStripCallFunc(const iocshArgBuf *args)
{
    germaniumStripConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival,
                      args[4].ival, args[5].ival, args[6].ival, args[7].ival);
}


static void germaniumStripRegister(void)
{

    iocshRegister(&configgermaniumStrip, configgermaniumStripCallFunc);
}

extern "C" {
epicsExportRegistrar(germaniumStripRegister);
}
