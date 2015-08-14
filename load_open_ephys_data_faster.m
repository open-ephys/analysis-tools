function [data, timestamps, info] = load_open_ephys_data_faster(filename)
%
% [data, timestamps, info] = load_open_ephys_data(filename)
%
%   Loads continuous, event, or spike data files into Matlab.
%
%   Inputs:
%
%     filename: path to file
%
%
%   Outputs:
%
%     data: either an array continuous samples (in microvolts),
%           a matrix of spike waveforms (in microvolts),
%           or an array of event channels (integers)
%
%     timestamps: in seconds
%
%     info: structure with header and other information
%
%
%
%   DISCLAIMER:
%
%   Both the Open Ephys data format and this m-file are works in progress.
%   There's no guarantee that they will preserve the integrity of your
%   data. They will both be updated rather frequently, so try to use the
%   most recent version of this file, if possible.
%
%

%
%     ------------------------------------------------------------------
%
%     Copyright (C) 2014 Open Ephys
%
%     ------------------------------------------------------------------
%
%     This program is free software: you can redistribute it and/or modify
%     it under the terms of the GNU General Public License as published by
%     the Free Software Foundation, either version 3 of the License, or
%     (at your option) any later version.
%
%     This program is distributed in the hope that it will be useful,
%     but WITHOUT ANY WARRANTY; without even the implied warranty of
%     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%     GNU General Public License for more details.
%
%     <http://www.gnu.org/licenses/>.
%

[~,~,filetype] = fileparts(filename);
if ~any(strcmp(filetype,{'.events','.continuous','.spikes'}))
    error('File extension not recognized. Please use a ''.continuous'', ''.spikes'', or ''.events'' file.');
end
fid = fopen(filename);
fseek(fid,0,'eof');
filesize = ftell(fid);

NUM_HEADER_BYTES = 1024;
fseek(fid,0,'bof');
hdr = fread(fid, NUM_HEADER_BYTES, 'char*1');
eval(char(hdr'));
info.header = header;
if (isfield(info.header, 'version'))
    if info.header.version <= 0.3
        error('Only version 0.4 is supported');
    end
else
    error('Only version 0.4 is supported');
end

switch filetype
    case '.events'
        blockBytes = [NUM_HEADER_BYTES 8 2 1 1 1 1 2];
        blockSize = sum(blockBytes(2:end));
        numIdx = floor((filesize - NUM_HEADER_BYTES)/blockSize);
        timestamps = freadoff(sum(blockBytes(1:1)), fid, numIdx, 'int64', blockSize-8, 'l')./info.header.sampleRate;
        info.sampleNum = freadoff(sum(blockBytes(1:2)), fid, numIdx, 'uint16', blockSize-2, 'n');
        info.eventType = freadoff(sum(blockBytes(1:3)), fid, numIdx, 'uint8', blockSize-1, 'n');
        info.nodeId = freadoff(sum(blockBytes(1:4)), fid, numIdx, 'uint8', blockSize-1, 'n');
        info.eventId = freadoff(sum(blockBytes(1:5)), fid, numIdx, 'uint8', blockSize-1, 'n');
        data = freadoff(sum(blockBytes(1:6)), fid, numIdx, 'uint8', blockSize-1, 'n');
        info.recordingNumber = freadoff(sum(blockBytes(1:7)), fid, numIdx, 'uint16', blockSize-2, 'n');
    case '.continuous'
        SAMPLES_PER_RECORD = 1024;
        blockBytes = [NUM_HEADER_BYTES 8 2 2 2*SAMPLES_PER_RECORD 10];
        blockSize = sum(blockBytes(2:end));
        numIdx = floor((filesize - NUM_HEADER_BYTES)/blockSize);
        info.ts = freadoff(sum(blockBytes(1:1)), fid, numIdx, 'int64', blockSize-8, 'l');
        info.nsamples = freadoff(sum(blockBytes(1:2)), fid, numIdx, 'uint16', blockSize-2, 'l');
        info.recNum = freadoff(sum(blockBytes(1:3)), fid, numIdx, 'uint16', blockSize-2,'n');
        data = freadoff(sum(blockBytes(1:4)), fid, numIdx*SAMPLES_PER_RECORD, '1024*int16', blockSize-2*SAMPLES_PER_RECORD, 'b').*info.header.bitVolts; % read in data
        timestamps = nan(size(data));
        current_sample = 0;
        for record = 1:length(info.ts)
            timestamps(current_sample+1:current_sample+info.nsamples(record)) = info.ts(record):info.ts(record)+info.nsamples(record)-1;
            current_sample = current_sample + info.nsamples(record);
        end
    case '.spikes'
        num_channels = info.header.num_channels;
        num_samples = 40; 
        blockBytes = [NUM_HEADER_BYTES 1 8 8 2 2 2 2 2 2 3 8 2 num_channels*num_samples*2 num_channels*4 num_channels*2 2];
        blockSize = sum(blockBytes(2:end));
        numIdx = floor((filesize - NUM_HEADER_BYTES)/blockSize);
        timestamps = freadoff(sum(blockBytes(1:2)), fid, numIdx, 'int64', blockSize-8, 'l')./info.header.sampleRate;
        info.source = freadoff(sum(blockBytes(1:4)), fid, numIdx, 'uint16', blockSize-2, 'l');
        info.gain = freadoff(sum(blockBytes(1:14)), fid, numIdx, 'single', blockSize-4,'n');
        info.thresh = freadoff(sum(blockBytes(1:15)), fid, numIdx, 'uint16', blockSize-2, 'l');
        info.sortedId = freadoff(sum(blockBytes(1:7)), fid, numIdx, 'uint16', blockSize-2, 'l');
        info.recNum = freadoff(sum(blockBytes(1:16)), fid, numIdx, 'uint16', blockSize-2, 'l');
        data = freadoff(sum(blockBytes(1:13)), fid, numIdx*num_samples*num_channels, '160*uint16', blockSize-2*num_samples*num_channels, 'l');
        data = permute(reshape(data, num_samples, num_channels, numIdx), [3 1 2]);
        data = (data-32768)./(info.gain(1)/1000);
end
fclose(fid); % close the file
end
function data = freadoff(offset, fid, size, precision, skip, mf)
fseek(fid, offset, 'bof'); 
data = fread(fid, size, precision, skip, mf);
end
