# analysis-tools

## IMPORTANT NOTE

We now maintain separate libraries for loading data saved by the Open Ephys GUI:
- [`open-ephys-python-tools`](https://github.com/open-ephys/open-ephys-python-tools) can be installed into your Python environment via `pip`
- [`open-ephys-matlab-tools`](https://github.com/open-ephys/open-ephys-matlab-tools) is available via the [Matlab File Exchange](https://www.mathworks.com/matlabcentral/fileexchange/122372-open-ephys-matlab-tools)

Both of these packages can be used to read any of the three currently supported data formats (Binary, Open Ephys Format, and NWB 2.0). More info on the data formats used by the Open Ephys GUI can be found [here](https://open-ephys.github.io/gui-docs/User-Manual/Recording-data/index.html).

The `analysis-tools` repository will be kept around for archival purposes, but we do not recommend using it for new projects.

### Usage instructions (Deprecated)

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

