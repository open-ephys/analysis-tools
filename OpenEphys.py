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
import struct

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

def loadFolder(folderpath,**kwargs):

    # load all continuous files in a folder  
    
    data = { }    
    
    # load all continuous files in a folder  
    if 'channels' in kwargs.keys():
        filelist = ['100_CH'+x+'.continuous' for x in map(str,kwargs['channels'])]
    else:
        filelist = os.listdir(folderpath)   

    t0 = time.time()
    numFiles = 0
    
    for i, f in enumerate(filelist):
        if '.continuous' in f:
            data[f.replace('.continuous','')] = loadContinuous(os.path.join(folderpath, f))
            numFiles += 1

    print ''.join(('Avg. Load Time: ', str((time.time() - t0)/numFiles),' sec'))
    print ''.join(('Total Load Time: ', str((time.time() - t0)),' sec'))        
            
    return data


def loadContinuous(filepath):

    print "Loading continuous data..."

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
    
def pack(folderpath,source='100',**kwargs):  
#convert single channel open ephys channels to a .dat file for compatibility with the KlustaSuite, Neuroscope and Klusters
#should not be necessary for versions of open ephys which write data into HDF5 format.  
#loads .continuous files in the specified folder and saves a .DAT in that folder
#optional arguments:
#   source: string name of the source that openephys uses as the prefix. is usually 100, if the headstage is the first source added, but can specify something different
#
#   data: pre-loaded data to be packed into a .DAT 
#   dref: int specifying a channel # to use as a digital reference. is subtracted from all channels.
#   order: the order in which the .continuos files are packed into the .DAT. should be a list of .continious channel numbers. length must equal total channels.
#   suffix: appended to .DAT filename, which is openephys.DAT if no suffix provided.

    #load the openephys data into memory
    if 'data' not in kwargs.keys():
        if 'channels' not in kwargs.keys():
            data = loadFolder(folderpath)
        else:   
            data = loadFolder(folderpath,channels=kwargs['channels'])
    else:
        data = kwargs['data']
    #if specified, do the digital referencing
    if 'dref' in kwargs.keys():
        ref =load(os.path.join(folderpath,''.join((source,'_CH',str(kwargs['dref']),'.continuous'))))
        for i,channel in enumerate(data.keys()):
            data[channel]['data'] = data[channel]['data'] - ref['data']   
    #specify the order the channels are written in
    if 'order' in kwargs.keys():
        order = kwargs['order']
    else:
        order = data.keys()                         
    #add a suffix, if one was specified        
    if 'suffix' in kwargs.keys():
        suffix=kwargs['suffix']
    else:
        suffix=''

    #make a file to write the data back out into .dat format
    outpath = os.path.join(folderpath,''.join(('openephys',suffix,'.dat')))
    out = open(outpath,'wb')
    
    #go through the data and write it out in the .dat format
    #.dat format specified here: http://neuroscope.sourceforge.net/UserManual/data-files.html
    channelOrder = []
    print ''.join(('...saving .dat to ',outpath,'...'))
    bar = ProgressBar(len(data[data.keys()[0]]['data']))#progressbar.ProgressBar(maxval=1, widgets=[progressbar.Bar('=', '[', ']'), ' ', progressbar.Percentage()])
    for i in range(len(data[data.keys()[0]]['data'])):
        for j in range(len(order)):
            if source in data.keys()[0]:
                ch = data[''.join((source,'_CH',str(order[j]).replace(source+'_CH','')))]['data']
            else:        
                ch = data[''.join(('CH',str(order[j]).replace('CH','')))]['data']
            out.write(struct.pack('h',ch[i]))#signed 16-bit integer
            #figure out which order this thing packed the channels in. only do this once.
            if i == 0:
                channelOrder.append(order[j])
        #update how mucb we have list
        if i%(len(data[data.keys()[0]]['data'])/100)==0:
            bar.animate(i)
            #bar.update(float(i+1)/float(len(data[data.keys()[0]])))
    #bar.finish()
    out.close()      
    print ''.join(('order: ',str(channelOrder)))
    print ''.join(('.dat saved to ',outpath))
    
#**********************************************************
# progress bar class used to show progress of pack()
    #stolen from some post on stack overflow
import sys
try:
    from IPython.display import clear_output
    have_ipython = True
except ImportError:
    have_ipython = False
class ProgressBar:
    def __init__(self, iterations):
        self.iterations = iterations
        self.prog_bar = '[]'
        self.fill_char = '*'
        self.width = 40
        self.__update_amount(0)
        if have_ipython:
            self.animate = self.animate_ipython
        else:
            self.animate = self.animate_noipython

    def animate_ipython(self, iter):
        print '\r', self,
        sys.stdout.flush()
        self.update_iteration(iter + 1)

    def update_iteration(self, elapsed_iter):
        self.__update_amount((elapsed_iter / float(self.iterations)) * 100.0)
        self.prog_bar += '  %d of %s complete' % (elapsed_iter, self.iterations)

    def __update_amount(self, new_amount):
        percent_done = int(round((new_amount / 100.0) * 100.0))
        all_full = self.width - 2
        num_hashes = int(round((percent_done / 100.0) * all_full))
        self.prog_bar = '[' + self.fill_char * num_hashes + ' ' * (all_full - num_hashes) + ']'
        pct_place = (len(self.prog_bar) // 2) - len(str(percent_done))
        pct_string = '%d%%' % percent_done
        self.prog_bar = self.prog_bar[0:pct_place] + \
            (pct_string + self.prog_bar[pct_place + len(pct_string):])

    def __str__(self):
        return str(self.prog_bar)
#*************************************************************
    