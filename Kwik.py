# -*- coding: utf-8 -*-
"""
Created on Wed Oct  8 12:05:54 2014

@author: Josh Siegle

Loads .kwd files

"""

import h5py
import numpy as np

def load(filename, dataset=0):
        
    f = h5py.File(filename, 'r')
    
    data = {}
    
    data['info'] = f['recordings'][str(dataset)].attrs
    data['data'] = f['recordings'][str(dataset)]['data']
    data['timestamps'] = ((np.arange(0,data['data'].shape[0])
                         + data['info']['start_time'])       
                         / data['info']['sample_rate'])
                         
    return data
    
def write(filename, dataset=0, bit_depth=1.0, sample_rate=25000.0):
    
    f = h5py.File(filename, 'w-')
    f.attrs['kwik_version'] = 2
    
    grp = f.create_group("/recordings/0")
    
    dset = grp.create_dataset("data", dataset.shape, dtype='i16')
    dset[:,:] = dataset
    
    grp.attrs['start_time'] = 0.0
    grp.attrs['start_sample'] = 0
    grp.attrs['sample_rate'] = sample_rate
    grp.attrs['bit_depth'] = bit_depth
    
    f.close()
    
    
     
    
                        
                         
                        