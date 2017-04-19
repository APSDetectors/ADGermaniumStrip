
import epics


'''



epics.caput('GER1:cam1:GeConnZMQ',1)

epics.caput('GER1:cam1:GeDisconnZMQ',1)

epics.caput('GER1:cam1:Acquire', 1)
epics.caput('GER1:cam1:AcquireTime',1 )


epics.caget('GER1:cam1:GeIsConnected_RBV')


dbgrep("*Ge*")
dbgrep("*Num*")

epics.caget('GER1:cam1:NumImagesCounter_RBV')


epics.caget('GER1:image1:ArrayCallbacks_RBV')



epics.caget('GER1:image1:EnableCallbacks_RBV')


epics.caget('GER1:image1:ArrayCounter_RBV')

GER1:image1:ArrayCounter_RBV


epics.caput('GER1:image1:EnableCallbacks',1 )

GER1:cam1:ArrayCallbacks


epics.caget('GER1:cam1:ArrayCallbacks')


dbgrep("*Call*")


dbgrep("*image1*")



dbgrep("*Num*")


'''
