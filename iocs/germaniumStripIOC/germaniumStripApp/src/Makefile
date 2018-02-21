TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for miscellaneous/asynPortDriver.cpp
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif


#zmq_DIR = $(ZMQ_DIR)
PROD_SYS_LIBS += zmq
#PROD_LIBS     += zmq
USR_LDFLAGS += -L/home/beams/TMADDEN/swWork/ZMQ/lib

#USR_LDFLAGS += -L/home/beams0/TMADDEN/EPICS/ADEpics/synApps_5_5/support/areaDetector-Master/ZMQ/lib -lzmq

PROD_NAME = germaniumStripApp
PROD_IOC = $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd


$(PROD_NAME)_DBD += GeDebugpluginSupport.dbd
$(PROD_NAME)_DBD += GepluginSupport.dbd
$(PROD_NAME)_DBD += germaniumStripSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
PROD_SRCS_DEFAULT += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp
PROD_SRCS_vxWorks += $(PROD_NAME)_registerRecordDeviceDriver.cpp

PROD_LIBS += NDPluginGEDEBUG
PROD_LIBS += NDPluginGe
PROD_LIBS += germaniumStrip

include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

