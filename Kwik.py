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
                        
                         
                        