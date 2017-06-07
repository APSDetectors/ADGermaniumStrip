#include <epicsEvent.h>
#include "ADDriver.h"

#include "maia_client.h"
#include "stopWatch.h"

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

        void geTask2();

        //0 for geTask, 1 for geTash2
        int getWhichTask();
        void geTimeTask();
    protected:
        int which_task;

        int GeFirst;
#define FIRST_GE_DETECTOR_PARAM GeFirst

        int GeConnZMQ;
        int GeDisconnZMQ;
        int GeConnString;
        int GeIsConnected;
        int GeServerType; //0 for new, 1 for old. new has fnum and strt messages. old is from joe mead, only data and meta    
        int GeDeleteFirstMessage; //1 to throw 1st imag of frame. it is prob old foga fifo data.
        int GeFrameMode; //reg 220dec, 0 for normal, 1 for debug, or run forever.
        int GeNumEvents;
        int GeFrameNumber;
        int GeMessageType;
        int GeEventRate;
        int GeTotalEvents;
        int GeMessNumber;

        int GeLast;  
#define LAST_GE_DETECTOR_PARAM GeLast

        int current_frame_number;

        int current_state;
        volatile double elapsedtime;

        enum  {
            st_start,//got start, alloc image, add attr new frame, goto datafirst
            st_datafirst, //wait for 1st data of frame, fill dat on frame to st_fnum
            st_data,//alloc new array, keep current array, imagewait for da 
            st_fnum//wait for fnum, add fnum attr to array, to st_data
        };

        stopWatch *myclock;


    private:
        /* These are the methods that are new to this class */

        /* Our data */
        epicsEventId startEventId_;
        epicsEventId stopEventId_;
        NDArray *pRaw_;

        maia_client *myclient;

        bool is_delete_message;

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

