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

# constants
NUM_HEADER_BYTES = 1024L
SAMPLES_PER_RECORD = 1024L
RECORD_SIZE = 8 + 16 + SAMPLES_PER_RECORD*2 + 10 # size of each continuous record in bytes
RECORD_MARKER = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 255])

# constants for pre-allocating matrices:
MAX_NUMBER_OF_SPIKES = 1e6
MAX_NUMBER_OF_RECORDS = 1e6
MAX_NUMBER_OF_CONTINUOUS_SAMPLES = 1e8
MAX_NUMBER_OF_EVENTS = 1e6

def load(filepath):
    
    # redirects to code for individual file types
    if 'continuous' in filepath:
        data = loadContinuous(filepath)
    elif 'spikes' in filepath:
        data = loadSpikes(filepath)
    elif 'events' in filepath:
        data = loadEvents(filepath)
    else:
        raise Exception("Not a recognized file type. Please input a .continuous, .spikes, or .events file")
        
    return data

def loadFolder(folderpath):

    # load all continuous files in a folder  
    
    data = { }    
    
    filelist = os.listdir(folderpath)   

    t0 = time.time()
    numFiles = 0
    
    for i, f in enumerate(filelist):
        if '.continuous' in f:
            #print ''.join(('Loading Channel ',f.replace('.continuous',''),'...'))
            data[f.replace('.continuous','')] = loadContinuous(os.path.join(folderpath, f))
            numFiles += 1

    print ''.join(('Avg. Load Time: ', str((time.time() - t0)/numFiles),' sec'))
    print ''.join(('Total Load Time: ', str((time.time() - t0)),' sec'))        
            
    return data


def loadContinuous(filepath):

    print "Loading continuous data from " + filepath

    ch = { }
    recordNumber = np.intp(-1)
    
#    f = open(filepath,'rb')
#    header = readHeader(f)
    
    ##preallocate the data array
#    while f.tell() < os.fstat(f.fileno()).st_size:
#        newSampleNumber = np.fromfile(f,np.dtype('<i8'),1)
#        N = np.fromfile(f,np.dtype('<u2'),1);        
#        recordingNumber = np.fromfile(f,np.dtype('>u2'),1) 
#        data = np.fromfile(f,np.dtype('>i2'),N)
#        marker = f.read(10)
#        totalN = totalN+1
        
#    f.close
    # pre-allocate samples
    samples = np.zeros(MAX_NUMBER_OF_CONTINUOUS_SAMPLES)
    timestamps = np.zeros(MAX_NUMBER_OF_RECORDS)
    recordingNumbers = np.zeros(MAX_NUMBER_OF_RECORDS)
    indices = np.arange(0,MAX_NUMBER_OF_RECORDS*SAMPLES_PER_RECORD, SAMPLES_PER_RECORD, np.dtype(np.int64))
    
    #read in the data
    f = open(filepath,'rb')
    
    header = readHeader(f)
    
    fileLength = os.fstat(f.fileno()).st_size
    #print fileLength
    #print f.tell()
    
    while f.tell() < fileLength:
        
        recordNumber += 1        
        
        timestamps[recordNumber] = np.fromfile(f,np.dtype('<i8'),1) # little-endian 64-bit signed integer 
        N = np.fromfile(f,np.dtype('<u2'),1) # little-endian 16-bit unsigned integer
        
        #print index

        if N != SAMPLES_PER_RECORD:
            raise Exception('Found corrupted record in block ' + str(recordNumber))
        
        recordingNumbers[recordNumber] = (np.fromfile(f,np.dtype('>u2'),1)) # big-endian 16-bit unsigned integer
        data = np.fromfile(f,np.dtype('>i2'),N) * float(header['bitVolts']) # big-endian 16-bit signed integer, multiplied by bitVolts   
        
        try:
            samples[indices[recordNumber]:indices[recordNumber+1]] = data            
        except ValueError:
            print type(index)
            raise
        
        marker = f.read(10) # dump
        
    #print recordNumber
    #print index
        
    ch['header'] = header 
    ch['timestamps'] = timestamps[0:recordNumber]
    ch['data'] = samples[0:indices[recordNumber]]  # OR use downsample(samples,1), to save space
    ch['recordingNumber'] = recordingNumbers[0:recordNumber]
    f.close()
    return ch
    
def loadSpikes(filepath):
    
    # doesn't quite work...spikes are transposed in a weird way    
    
    data = { }
    
    print 'loading spikes...'
    
    f = open(filepath,'rb')
    header = readHeader(f)
    
    if float(header[' version']) < 0.4:
        raise Exception('Loader is only compatible with .spikes files with version 0.4 or higher')
     
    data['header'] = header 
    numChannels = int(header['num_channels'])
    numSamples = 40 # **NOT CURRENTLY WRITTEN TO HEADER**
    
    spikes = np.zeros((MAX_NUMBER_OF_SPIKES, numSamples, numChannels))
    timestamps = np.zeros(MAX_NUMBER_OF_SPIKES)
    source = np.zeros(MAX_NUMBER_OF_SPIKES)
    gain = np.zeros((MAX_NUMBER_OF_SPIKES, numChannels))
    thresh = np.zeros((MAX_NUMBER_OF_SPIKES, numChannels))
    sortedId = np.zeros((MAX_NUMBER_OF_SPIKES, numChannels))
    recNum = np.zeros(MAX_NUMBER_OF_SPIKES)
    
    currentSpike = 0
    
    while f.tell() < os.fstat(f.fileno()).st_size:
        
        eventType = np.fromfile(f, np.dtype('<u1'),1) #always equal to 4, discard
        timestamps[currentSpike] = np.fromfile(f, np.dtype('<i8'), 1)
        software_timestamp = np.fromfile(f, np.dtype('<i8'), 1)
        source[currentSpike] = np.fromfile(f, np.dtype('<u2'), 1)
        numChannels = np.fromfile(f, np.dtype('<u2'), 1)
        numSamples = np.fromfile(f, np.dtype('<u2'), 1)
        sortedId[currentSpike] = np.fromfile(f, np.dtype('<u2'),1)
        electrodeId = np.fromfile(f, np.dtype('<u2'),1)
        channel = np.fromfile(f, np.dtype('<u2'),1)
        color = np.fromfile(f, np.dtype('<u1'), 3)
        pcProj = np.fromfile(f, np.float32, 2)
        sampleFreq = np.fromfile(f, np.dtype('<u2'),1)
        
        waveforms = np.fromfile(f, np.dtype('<u2'), numChannels*numSamples)
        wv = np.reshape(waveforms, (numSamples, numChannels))
        
        gain[currentSpike,:] = np.fromfile(f, np.float32, numChannels)
        thresh[currentSpike,:] = np.fromfile(f, np.dtype('<u2'), numChannels)
        
        recNum[currentSpike] = np.fromfile(f, np.dtype('<u2'), 1)

        #print wv.shape        
        
        for ch in range(numChannels):
            spikes[currentSpike,:,ch] = (np.float64(wv[:,ch])-32768)/(gain[currentSpike,ch]/1000)
        
        currentSpike += 1
        
    data['spikes'] = spikes[:currentSpike,:,:]
    data['timestamps'] = timestamps[:currentSpike]
    data['source'] = source[:currentSpike]
    data['gain'] = gain[:currentSpike,:]
    data['thresh'] = thresh[:currentSpike,:]
    data['recordingNumber'] = recNum[:currentSpike]
    data['sortedId'] = sortedId[:currentSpike]

    return data
    
def loadEvents(filepath):

    data = { }
    
    print 'loading events...'
    
    f = open(filepath,'rb')
    header = readHeader(f)
    
    if float(header[' version']) < 0.4:
        raise Exception('Loader is only compatible with .events files with version 0.4 or higher')
     
    data['header'] = header 
    
    index = -1

    channel = np.zeros(MAX_NUMBER_OF_EVENTS)
    timestamps = np.zeros(MAX_NUMBER_OF_EVENTS)
    sampleNum = np.zeros(MAX_NUMBER_OF_EVENTS)
    nodeId = np.zeros(MAX_NUMBER_OF_EVENTS)
    eventType = np.zeros(MAX_NUMBER_OF_EVENTS)
    eventId = np.zeros(MAX_NUMBER_OF_EVENTS)
    recordingNumber = np.zeros(MAX_NUMBER_OF_EVENTS)

    while f.tell() < os.fstat(f.fileno()).st_size:
        
        index += 1
        
        timestamps[index] = np.fromfile(f, np.dtype('<i8'), 1)
        sampleNum[index] = np.fromfile(f, np.dtype('<i2'), 1)
        eventType[index] = np.fromfile(f, np.dtype('<u1'), 1)
        nodeId[index] = np.fromfile(f, np.dtype('<ui'), 1)
        eventId[index] = np.fromfile(f, np.dtype('<u1'), 1)
        channel[index] = np.fromfile(f, np.dtype('<u1'), 1)
        recordingNumber[index] = np.fromfile(f, np.dtype('<u2'), 1)
        
    data['channel'] = channel[:index]
    data['timestamps'] = timestamps[:index]
    data['eventType'] = eventType[:index]
    data['nodeId'] = nodeId[:index]
    data['eventId'] = eventId[:index]
    data['recordingNumber'] = recordingNumber[:index]
    data['sampleNum'] = sampleNum[:index]
    
    return data
    
def readHeader(f):
    header = { }
    h = f.read(1024).replace('\n','').replace('header.','')
    for i,item in enumerate(h.split(';')):
        if '=' in item:
            header[item.split(' = ')[0]] = item.split(' = ')[1]
    return header
    
def downsample(trace,down):
    downsampled = scipy.signal.resample(trace,np.shape(trace)[0]/down)
    return downsampled
    

    