# -*- coding: utf-8 -*-
"""
Created on Sun Aug  3 15:18:38 2014

@author: Dan Denman and Josh Siegle

Loads .continuous, .events, and .spikes files saved from the Open Ephys GUI

Usage:
    import OpenEphys
    data = OpenEphys.load(pathToFile) # returns a dict with data, timestamps, etc.

"""

import os
import numpy as np
import scipy.signal
import scipy.io
import time

def load(filepath):
    
    # redirects to code for individual file types
    if 'continuous' in filepath:
        loadContinuous(filepath)
    elif 'spikes' in filepath:
        loadSpikes(filepath)
    elif 'events' in filepath:
        loadEvents(filepath)
    else:
        print "Not a recognized file type. Please input a .continuous, .spikes, or .events file"

def loadFolder(folderpath):

    # load all continuous files in a folder  
    
    data={}    
    
    filelist = os.listdir(folderpath)   

    t0 = time.time()
    numFiles = 0
    
    for i, f in enumerate(filelist):
        if '.continuous' in f:
            print ''.join(('Loading Channel ',f.replace('.continuous',''),'...'))
            data[f.replace('.continuous','')] = loadChannel(os.path.join(folderpath, f))
            numFiles += 1

    print ''.join(('Avg. Load Time: ', str((time.time() - t0)/numFiles),' sec'))
    print ''.join(('Total Load Time: ', str((time.time() - t0)),' sec'))        
            
    return data


def loadContinuous(filepath):

    sampleNumbers = []; samples = []; times = []; Ns = []; recordingNumbers = [];
    ch = {};
    index = 0
    totalN = 0
    
    ##preallocate the data array
    f = open(filepath,'rb')
    header = readHeader(f)
    
    while f.tell() < os.fstat(f.fileno()).st_size:
        newSampleNumber = np.fromfile(f,np.dtype('<i8'),1)
        N = np.fromfile(f,np.dtype('<u2'),1);        
        recordingNumber = np.fromfile(f,np.dtype('>u2'),1) 
        data = np.fromfile(f,np.dtype('>i2'),N)
        marker = f.read(10)
        totalN = totalN+1
        
    f.close
    samples = np.zeros(totalN*float(header['blockLength']))# <-- actual pre-allocation 
    
    #read in the data
    f = open(filepath,'rb')
    
    header = readHeader(f)
    
    while f.tell() < os.fstat(f.fileno()).st_size:
        
        sampleNumber = np.fromfile(f,np.dtype('<i8'),1);sampleNumbers.extend(sampleNumber) # little-endian 64-bit signed integer
        N = np.fromfile(f,np.dtype('<u2'),1); # little-endian 8-bit unsigned integer
        recordingNumbers.extend(np.fromfile(f,np.dtype('>u2'),1)) # big-endian 8-bit unsigned integer
        data = np.fromfile(f,np.dtype('>i2'),N)*float(header['bitVolts']) # big-endian 8-bit signed integer, multiplied by bitVolts
        
        if N > 0:
            N = 1024
            samples[index:index+N] = data
            times[index:index+N] = np.linspace(sampleNumber,sampleNumber+N,N,False)
            index += N
            
        marker = f.read(10) # dump
        
    ch['header'] = header  
    ch['timestamps'] = np.array(times)/float(header['sampleRate'])
    ch['data'] = samples  # downsample(samples,1)
    ch['recordingNumber'] = recordingNumbers
    f.close()
    return ch
    
def loadSpikes(filepath):
    
    print 'loading spikes...'
    
def loadEvents(filepath):
    
    print 'loading events...'
    
def readHeader(f):
    header = {}
    h = f.read(1024).replace('\n','').replace('header.','')
    for i,item in enumerate(h.split(';')):
        if '=' in item:
            header[item.split(' = ')[0]] = item.split(' = ')[1]
    return header
    
def downsample(trace,down):
    downsampled = scipy.signal.resample(trace,np.shape(trace)[0]/down)
    return downsampled
    

    