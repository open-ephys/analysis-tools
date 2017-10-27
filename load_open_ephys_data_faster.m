function [data, timestamps, info] = load_open_ephys_data_faster(filename, varargin)
%
% [data, timestamps, info] = load_open_ephys_data(filename, [outputFormat])
%
%   Loads continuous, event, or spike data files into Matlab.
%
%   Inputs:
%
%     filename: path to file
%     outputFormat: (optional) If omitted, continuous data is output in double format and is scaled to reflect microvolts. 
%                   If this argument is 'unscaledInt16' and the file contains continuous data, the output data will be in
%                   int16 format and will not be scaled; this data must be manually converted to a floating-point format
%                   and multiplied by info.header.bitVolts to obtain microvolt values. This feature is intended to save memory
%                   for operations involving large amounts of data.
%
%
%   Outputs:
%
%     data: either an array continuous samples (in microvolts unless outputFormat is specified, see above),
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

bInt16Out = false;
if nargin > 2
    error('Too many input arguments.');
elseif nargin == 2
    if strcmpi(varargin{1}, 'unscaledInt16')
        bInt16Out = true;
    else
        error('Unrecognized output format.');
    end
end

fid = fopen(filename);
fseek(fid,0,'eof');
filesize = ftell(fid);

NUM_HEADER_BYTES = 1024;
fseek(fid,0,'bof');
hdr = fread(fid, NUM_HEADER_BYTES, 'char*1');
info = getHeader(hdr);
if isfield(info.header, 'version')
    version = info.header.version;
else
    version = 0.0;
end

switch filetype
    case '.events'
        bStr = {'timestamps' 'sampleNum' 'eventType' 'nodeId' 'eventId' 'data' 'recNum'};
        bTypes = {'int64' 'uint16' 'uint8' 'uint8' 'uint8' 'uint8' 'uint16'};      
        bRepeat = {1 1 1 1 1 1 1};
        dblock = struct('Repeat',bRepeat,'Types', bTypes,'Str',bStr);
        if version < 0.2, dblock(7) = [];  end
        if version < 0.1, dblock(1).Types = 'uint64'; end
    case '.continuous'
        SAMPLES_PER_RECORD = 1024;
        bStr = {'ts' 'nsamples' 'recNum' 'data' 'recordMarker'};
        bTypes = {'int64' 'uint16' 'uint16' 'int16' 'uint8'};
        bRepeat = {1 1 1 SAMPLES_PER_RECORD 10};
        dblock = struct('Repeat',bRepeat,'Types', bTypes,'Str',bStr);
        if version < 0.2, dblock(3) = []; end
        if version < 0.1, dblock(1).Types = 'uint64'; dblock(2).Types = 'int16'; end
    case '.spikes'
        num_channels = info.header.num_channels;
        num_samples = 40; 
        bStr = {'eventType' 'timestamps' 'timestamps_software' 'source' 'nChannels' 'nSamples' 'sortedId' 'electrodeID' 'channel' 'color' 'pcProj' 'samplingFrequencyHz' 'data' 'gain' 'threshold' 'recordingNumber'};
        bTypes = {'uint8' 'int64' 'int64' 'uint16' 'uint16' 'uint16' 'uint16' 'uint16' 'uint16' 'uint8' 'float32' 'uint16' 'uint16' 'float32' 'uint16' 'uint16'};
        bRepeat = {1 1 1 1 1 1 1 1 1 3 2 1 num_channels*num_samples num_channels num_channels 1};
        dblock = struct('Repeat',bRepeat,'Types', bTypes,'Str',bStr);
        if version < 0.4,  dblock(7:12) = []; dblock(8).Types = 'uint16'; end
        if version == 0.3, dblock = [dblock(1), struct('Repeat',1,'Types','uint32','Str','ts'), dblock(2:end)]; end
        if version < 0.3, dblock(2) = []; end
        if version < 0.2, dblock(9) = []; end
        if version < 0.1, dblock(2).Types = 'uint64'; end
end
blockBytes = str2double(regexp({dblock.Types},'\d{1,2}$','match', 'once')) ./8 .* cell2mat({dblock.Repeat});
numIdx = floor((filesize - NUM_HEADER_BYTES)/sum(blockBytes));

switch filetype
    case '.events'
        timestamps = segRead('timestamps')./info.header.sampleRate;
        info.sampleNum = segRead('sampleNum');
        info.eventType = segRead('eventType');
        info.nodeId = segRead('nodeId');
        info.eventId = segRead('eventId');
        data = segRead('data');
        if version >= 0.2, info.recNum = segRead('recNum'); end
    case '.continuous'
        if nargout>1
            info.ts = segRead('ts');
        end            
        info.nsamples = segRead('nsamples');
        if ~all(info.nsamples == SAMPLES_PER_RECORD)&& version >= 0.1, error('Found corrupted record'); end
        if version >= 0.2, info.recNum = segRead('recNum'); end
        
        % read in continuous data
        if bInt16Out
            data = segRead_int16('data', 'b');
        else
            data = segRead('data', 'b') .* info.header.bitVolts;
        end
        
        if nargout>1 % do not create timestamp arrays unless they are requested
            timestamps = nan(size(data));
            current_sample = 0;
            for record = 1:length(info.ts)
                timestamps(current_sample+1:current_sample+info.nsamples(record)) = info.ts(record):info.ts(record)+info.nsamples(record)-1;
                current_sample = current_sample + info.nsamples(record);
            end
            timestamps = timestamps./info.header.sampleRate;
        end
    case '.spikes'
        timestamps = segRead('timestamps')./info.header.sampleRate;
        info.source = segRead('source');
        info.samplenum = segRead('nSamples');
        info.gain = permute(reshape(segRead('gain'), num_channels, numIdx), [2 1]);
        info.thresh = permute(reshape(segRead('threshold'), num_channels, numIdx), [2 1]);
        if version >= 0.4, info.sortedId = segRead('sortedId'); end
        if version >= 0.2, info.recNum = segRead('recordingNumber'); end
        data = permute(reshape(segRead('data'), num_samples, num_channels, numIdx), [3 1 2]);
        data = (data-32768)./ permute(repmat(info.gain/1000,[1 1 num_samples]), [1 3 2]);
end
fclose(fid);

function seg = segRead_int16(segName, mf)
    %% This function is specifically for reading continuous data. 
    %  It keeps the data in int16 precision, which can drastically decrease
    %  memory consumption
    if nargin == 1, mf = 'l'; end
    segNum = find(strcmp({dblock.Str},segName));
    fseek(fid, sum(blockBytes(1:segNum-1))+NUM_HEADER_BYTES, 'bof'); 
    seg = fread(fid, numIdx*dblock(segNum).Repeat, [sprintf('%d*%s', ...
        dblock(segNum).Repeat,dblock(segNum).Types) '=>int16'], sum(blockBytes) - blockBytes(segNum), mf);
    
end

function seg = segRead(segName, mf)
    if nargin == 1, mf = 'l'; end
    segNum = find(strcmp({dblock.Str},segName));
    fseek(fid, sum(blockBytes(1:segNum-1))+NUM_HEADER_BYTES, 'bof'); 
    seg = fread(fid, numIdx*dblock(segNum).Repeat, sprintf('%d*%s', ...
        dblock(segNum).Repeat,dblock(segNum).Types), sum(blockBytes) - blockBytes(segNum), mf);
    
end

end
function info = getHeader(hdr)
eval(char(hdr'));
info.header = header;
end
