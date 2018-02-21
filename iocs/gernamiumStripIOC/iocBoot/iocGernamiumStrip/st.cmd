# Must have loaded envPaths via st.cmd.linux or st.cmd.win32
<envPaths

errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/germaniumStripApp.dbd")
germaniumStripApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "GER1:")
# The port name for the detector
epicsEnvSet("PORT",   "GER1")
# The queue size for all plugins
epicsEnvSet("QSIZE",  "20")
# The maximim image width; used to set the maximum size for this driver and for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "1024")
# The maximim image height; used to set the maximum size for this driver and for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "1024")
# The maximum number of time series points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

asynSetMinTimerPeriod(0.001)

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "10000000")

# Create a germaniumStrip driver

germaniumStripConfig("$(PORT)", $(XSIZE), $(YSIZE), 1, 0, 0)

dbLoadRecords("$(ADGERMANIUMSTRIP)/db/germaniumStrip.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")



NDPluginGeDebugConfigure("GEDEBUG",65536,10000,0,"$(PORT)",0,50,0)
#dbLoadRecords("NDFile.template","P=$(PREFIX),R=GEDEBUG:,PORT=GEDEBUG,ADDR=0,TIMEOUT=1")
dbLoadRecords("NDPluginGeStripDebug.template","P=$(PREFIX),R=GEDEBUG:,PORT=GEDEBUG,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")


#dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=GEDEBUG:,PORT=GEDEBUG,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Float64,FTVL=DOUBLE,NELEMENTS=12000000")



NDFileGeConfigure("GEFILE",10000000,300,0,"$(PORT)",50,0)
dbLoadRecords("$(GEFILE)/NDFileGeApp/Db/NDFileGe.template","P=$(PREFIX),R=GEFILE:,PORT=GEFILE,NDARRAY_PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin, set it to get data from first germaniumStrip driver.
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)

# This waveform allows transporting 64-bit float images
dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Float64,FTVL=DOUBLE,NELEMENTS=65536")


# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd
#set_requestfile_path("$(ADGERMANIUMSTRIP)/germaniumStripApp/Db")

iocInit()

# save things every thirty seconds
#create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")

dbpf GER1:cam1:GeServerType,1
dbpf GER1:cam1:GeServerString,"tcp://127.0.0.1"
dbpf GER1:cam1:GeDeleteFirstMessage,0
dbpf GER1:cam1:GeFrameMode,0
dbpf GER1:GEFILE:EnableCallbacks,1

dbpf GER1:image1:MinCallbackTime,1.0

