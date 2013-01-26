function [data, timestamps, info] = load_open_ephys_data(filename, use_updated_format)

%
% [data, timestamps, info] = load_open_ephys_data(filename, use_updated_format)
%
%   Loads continuous or event data files into Matlab.
%
%   Inputs:
%     
%     filename: path to file
%
%     use_updated_format: 0 or 1, defaults to 1 if not specified
%          - 0 indicates no sample numbers in event files and no record
%              markers in continuous files
%          - 1 indicates sample numbers in events files and record markers
%              in continuous files
%
%
%   Outputs:
%
%     data: either continuous samples or event channels
%
%     timestamps: sample time in seconds
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
%     Copyright (C) 2013 Open Ephys
%
%     Last updated by jsiegle on January 26, 2013
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
%     You should have received a copy of the GNU General Public License
%     along with this program.  If not, see <http://www.gnu.org/licenses/>.
% 


if nargin < 2
    use_updated_format = 1; % include sample number in event data and record marker in continuous data
end


filetype = filename(max(strfind(filename,'.'))+1:end); % parse filetype

fid = fopen(filename);
filesize = getfilesize(fid);

%-----------------------------------------------------------------------
%------------------------- EVENT DATA ----------------------------------
%-----------------------------------------------------------------------

if strcmp(filetype, 'events')
    
    disp(['Loading events file...']);
    
    index = 0;
    
    hdr = fread(fid, 1024, 'char*1');
    eval(char(hdr'));
    info.header = header;
    
    while ftell(fid) + 22 < filesize % at least one record remains
        
        index = index + 1;
        
        timestamps(index) = fread(fid, 1, 'int64');
        
        if (use_updated_format)
            info.sampleNum(index) = fread(fid, 1, 'int16'); % implemented after 11/16/12
        end
        
        info.eventType(index) = fread(fid, 1, 'uint8');
        info.nodeId(index) = fread(fid, 1, 'uint8');
        info.eventId(index) = fread(fid, 1, 'uint8');
        data(index) = fread(fid, 1, 'uint8'); % save event channel as 'data' (maybe not the best thing to do)
        
    end
    
    timestamps = timestamps./1e6; % convert from microseconds to seconds
    
%-----------------------------------------------------------------------
%---------------------- CONTINUOUS DATA --------------------------------
%-----------------------------------------------------------------------
    
elseif strcmp(filetype, 'continuous')
    
    disp(['Loading continuous file...']);
    
    index = 0;
    
    max_samples = 10e6; % pre-allocate 10 million samples for data
    
    hdr = fread(fid, 1024, 'char*1');
    eval(char(hdr'));
    info.header = header;
    
    data = zeros(max_samples,1);
    
    current_sample = 0;
      
    while ftell(fid) + 4096 < filesize % at least one record remains
     
        index = index+1;
        
        info.ts(index) = fread(fid, 1, 'int64');
        nsamples = fread(fid, 1, 'int16');
        
        block = fread(fid, nsamples, 'int16', 0, 'b');
        
        if (use_updated_format)
            fread(fid, 10, 'uint8');
        end
        
        data(current_sample+1:current_sample+nsamples) = block;
        
        current_sample = current_sample + nsamples;
        
        info.nsamples(index) = nsamples;
        
    end
    
    data = data(1:current_sample); % get rid of extra zeros
    
    timestamps = nan(size(data));
    
    current_sample = 0;
    
    for record = 1:length(info.ts)-1
       
        ts_interp = linspace(info.ts(record),info.ts(record+1),info.nsamples(record)+1);
        
        timestamps(current_sample+1:current_sample+info.nsamples(record)) = ts_interp(1:end-1);
        
        current_sample = current_sample + info.nsamples(record);
    end
    
    % NOTE: the timestamps for the last record will not be interpolated
    
    timestamps = timestamps./1e6; % convert from microseconds to seconds
    
else
    
    error('Filetype not recognized.');
    
end

fclose(fid);

end

function filesize = getfilesize(fid)

fseek(fid,0,'eof');
filesize = ftell(fid);
fseek(fid,0,'bof');

end
