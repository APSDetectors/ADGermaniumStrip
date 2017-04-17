TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#======== DETECTOR LIBRARY ==============

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for miscellaneous/asynPortDriver.cpp
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif
USR_INCLUDES += -I../../../../../areaDetector-Master/ZMQ/include

INC += germaniumStrip.h


LIBRARY_IOC = germaniumStrip

LIB_SRCS += stopWatch.cpp
LIB_SRCS += germaniumStrip.cpp
LIB_SRCS += maia_client.cpp

DBD += germaniumStripSupport.dbd

#LIB_LIBS += zmq
#USR_LDFLAGS += -L../../../../../areaDetector-Master/ZMQ/lib -lzmq

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

