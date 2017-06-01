#include <epicsEvent.h>
#include "ADDriver.h"

#include "maia_client.h"

#define DRIVER_VERSION      0
#define DRIVER_REVISION     0
#define DRIVER_MODIFICATION 0

/** Geulation detector driver; demonstrates most of the features that areaDetector drivers can support. */
class epicsShareClass germaniumStrip : public ADDriver {
public:
    germaniumStrip(const char *portName, int maxSizeX, int maxSizeY, NDDataType_t dataType,
                int maxBuffers, size_t maxMemory,
                int priority, int stackSize);

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    asynStatus writeOctet(asynUser *pasynUser, const char *value,
                                       size_t nChars, size_t *nActual);



    virtual void setShutter(int open);
    virtual void report(FILE *fp, int details);
    void geTask(); /**< Should be private, but gets called from C, so must be public */

protected:
    int GeFirst;
    #define FIRST_GE_DETECTOR_PARAM GeFirst
    
    int GeConnZMQ;
    int GeDisconnZMQ;
    int GeConnString;
    int GeIsConnected;
    
    
    
    int GeLast;  
    #define LAST_GE_DETECTOR_PARAM GeLast

    int current_frame_number;

    int current_state;

    enum  {
	st_start,//got start, alloc image, add attr new frame, goto datafirst
	st_datafirst, //wait for 1st data of frame, fill dat on frame to st_fnum
	st_data,//alloc new array, keep current array, imagewait for da 
	st_fnum//wait for fnum, add fnum attr to array, to st_data
	};


private:
    /* These are the methods that are new to this class */
  
    /* Our data */
    epicsEventId startEventId_;
    epicsEventId stopEventId_;
    NDArray *pRaw_;
    
    maia_client *myclient;
  
    #define DEFAULT_ZMQ_CONN_STR "tcp://127.0.0.1"
    bool is_running_deamon;
    
};

typedef enum {
    GeModeLinearRamp,
    GeModePeaks,
    GeModeSine
} GeModes_t;

typedef enum {
    GeSineOperationAdd,
    GeSineOperationMultiply
} GeSineOperation_t;

#define GeGainXString                "GE_GAIN_X"
#define GeGainYString                "GE_GAIN_Y"

#define NUM_GE_DETECTOR_PARAMS ((int)(&LAST_GE_DETECTOR_PARAM - &FIRST_GE_DETECTOR_PARAM + 1))

