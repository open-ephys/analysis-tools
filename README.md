analysis-tools
==============

Code for loading and converting data saved by the Open Ephys GUI

More info on the data formats used by Open Ephys can be found at https://open-ephys.atlassian.net/wiki/display/OEW/Data+format

For Matlab:
- use 'load_open_ephys_data.m' for .continuous, .spikes, and .events files

For Python:
- use the 'OpenEphys.py' module for .continuous files, .spikes, and .events files
- use the 'Kwik.py' module for .kwd files

For Julia:
- check out this repository: https://github.com/galenlynch/OpenEphysLoader.jl

For MClust:
- use the 'LoadTT_openephys.m' loading engine

For Plexon Offline Sorter:
- use the 'ephys2plx' library

Please submit any bug reports and feature requests to https://github.com/open-ephys/analysis-tools/issues

