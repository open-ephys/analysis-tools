analysis-tools
==============

Code for loading and converting data saved by the Open Ephys GUI

More info on the Open Ephys data format can be found at https://open-ephys.atlassian.net/wiki/display/OEW/Data+format

More info on the Kwik data format (implemented in the GUI as of October 2014) can be found at https://github.com/klusta-team/kwiklib/wiki/Kwik-format

For Matlab:
- use 'load_open_ephys_data.m' for .continuous, .spikes, and .events files

For Python:
- use the 'OpenEphys.py' module for .continuous files, .spikes, and .events files
- use the 'Kwik.py' module for .kwd files

For MClust:
- use the 'LoadTT_openephys.m' loading engine

For Plexon Offline Sorter:
- use the 'ephys2plx' library

Please submit any bug reports and feature requests to https://github.com/open-ephys/analysis-tools/issues

