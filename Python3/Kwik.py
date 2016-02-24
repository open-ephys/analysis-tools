# -*- coding: utf-8 -*-
"""
Created on Wed Oct  8 12:05:54 2014

@author: Josh Siegle

Loads .kwd, .kwik and .kwx files

example:
    RawData = Kwik.load('experiment1_100.raw.kwd')
    Events = Kwik.load('experiment1.kwik')
"""

import h5py
import numpy as np

def load(filename, dataset=0):
    f = h5py.File(filename, 'r')
    
    if filename[-4:] == '.kwd':
        data = {}    
        data['info'] = f['recordings'][str(dataset)].attrs
        data['data'] = f['recordings'][str(dataset)]['data']
        data['timestamps'] = ((np.arange(0,data['data'].shape[0])
                             + data['info']['start_time'])       
                             / data['info']['sample_rate'])
        
    elif filename[-4:] == 'kwik':
        data = {}    
        data['Messages'] = f['event_types']['Messages']['events']
        data['TTLs'] = f['event_types']['TTL']['events']
        
    elif filename[-4:] == '.kwx':
        data = f['channel_groups']
    
    return(data)


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


def get_sample_rate(f):
    return f['recordings']['0'].attrs['sample_rate'] 


def get_edge_times(f, TTLchan, rising=True):
    
    events_for_chan = np.where(np.squeeze(f['event_types']['TTL']['events']['user_data']['event_channels']) == TTLchan)
    
    edges = np.where(np.squeeze(f['event_types']['TTL']['events']['user_data']['eventID']) == 1*rising) 
    
    edges_for_chan = np.intersect1d(events_for_chan, edges)
    
    edge_samples = np.squeeze(f['event_types']['TTL']['events']['time_samples'][:])[edges_for_chan]
    edge_times = edge_samples / get_sample_rate(f)
    
    return edge_times


def get_rising_edge_times(filename, TTLchan):
    
    f = h5py.File(filename, 'r')
    
    return get_edge_times(f, TTLchan, True)


def get_falling_edge_times(filename, TTLchan):
    
    f = h5py.File(filename, 'r')
    
    return get_edge_times(f, TTLchan, False)  


def get_experiment_start_time(filename):
    
    f = h5py.File(filename, 'r')
    
    return f['event_types']['Messages']['events']['time_samples'][1]/ get_sample_rate(f)
