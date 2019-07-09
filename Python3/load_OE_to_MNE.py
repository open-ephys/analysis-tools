# -*- coding: utf-8 -*-
"""
Copyright (C) 2019 Translational NeuroEngineering Laboratory

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@author: Mark Schatza <schat107@umn.edu>
"""

import numpy as np
import mne
import OpenEphys
import glob
import re

'''
Human Sorting from : https://nedbatchelder.com/blog/200712/human_sorting.html
'''
def tryint(s):
    try:
        return int(s)
    except ValueError:
        return s
def alphanum_key(s):
    """ Turn a string into a list of string and number chunks.
        "z23a" -> ["z", 23, "a"]
    """
    return [ tryint(c) for c in re.split('([0-9]+)', s) ] 


def loadContinuous(folder, pluginId):
    '''
    Loads continuous files and returns a MNE data array.
    Currently looses all information stored with the data besides electrode number and sampling rate.
    Anything else needs to be handled separately.

    Inputs:
        folder - The folder where the data is held (needs ending \)
        pluginId - from 100_CH1.continuous. Normally raw data is from 100. This can be found from config.xml file.
    '''
    
    # Get file names and preform human sorting
    files = glob.glob(folder + str(pluginId) + '_CH*.continuous')
    files.sort(key=alphanum_key)
    
    # Load in Open Ephys data and save channel names
    datas = []
    chanNames = []
    for dataPath in files:
        chanNames.append(re.search('100_CH(.*).continuous', dataPath).group(1))
        datas.append(OpenEphys.load(dataPath))
        
    print('loading done! Now putting into MNE format')
    
    # Use eeg because that's the closest mne offers
    chanTypes = ['eeg'] * len(files)
    
    info = mne.create_info(ch_names=chanNames, sfreq=datas[0]['header']['sampleRate'], ch_types=chanTypes)
    
    # Make a list of the data from the channels
    rawData = []
    for data in datas:
        rawData.append(data['data'])
    
    # Put into mne format
    matrix = np.array(rawData)
    raw = mne.io.RawArray(matrix, info)
    
    print('MNE conversion complete')
    return raw

# Example usage
if __name__== "__main__":
    raw = loadContinuous(r'C:\Users\Ephys\Documents\EphysData\OP7\\', 100)
    scalings = {'eeg':2000}
    raw.plot(n_channels = 4, title='Data', 
             scalings=scalings, show=True, block=False)