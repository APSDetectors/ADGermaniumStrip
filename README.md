# ADGermaniumStrip

This is an EPICS areaDetector driver for the Germanium Strip Detector (GSD) developed by Brookhaven and Argonne National Laboratories. The GSD consists of a monolithic segmented germanium sensor with spectroscopy highly integrated readout (i.e., ASICs) (https://doi.org/10.1109/TNS.2014.2365358). It is designed for powder and energy-dispersive X-ray diffraction techniques at the NSLS-2 and APS synchrotron light sources. 

The detector is controlled and data transmitted via ZeroMQ server running on a picoZed board which is part of a BNL-develop DAQ system to readout the detector. 

## Installation notes
* Check the envPaths file for the required EPICS modules, located at:
** /ADGermaniumStrip-1-0/iocs/germaniumStripIOC/iocBoot/iocGermaniumStrip/envPaths


