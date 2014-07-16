function varargout = LoadTT_simpleclust(fn,varargin )

%MClust Loading engine for reading in tetrode data collected with 
%Open Ephys GUI software (http://open-ephys.org/#/gui/). Requires
%pre-processing with Simpleclust (Jacob Voigts,
%http://jvoigts.scripts.mit.edu/blog/simpleclust-manual-spike-sorting-in-matlab/)
%This function is meant to be called by MClust
%(A. David Redish, http://redishlab.neuroscience.umn.edu/MClust/MClust.html).
%
%input:
%     fn = file name string. This should be 'chN_simpleclust.mat'
%     records_to_get = an array that is either a list or a range of values
%     record_units =
%         1: timestamp list (a vector of timestamps to load (uses binsearch to find them in file))
%         2: record number list (a vector of records to load)
%         3: range of timestamps (a vector with 2 elements: a start and an end timestamp)
%         4: range of records (a vector with 2 elements: a start and an end record number)
%         5: return the count of spikes (records_to_get should be [] in this case)
% if only fn is passed in then the entire file is opened.
% if only fn is passed AND only t is provided as the output, then all
%    of the timestamps for the entire file are returned.
%
% output:
%    [t, wv]
%    t = n x 1: timestamps of each spike in file
%    wv = n x 4 x 32 waveforms
%
%
%mike wehr 06.06.2014
%wehr@uoregon.edu

load(fn);

switch length(varargin)
    
    case 0      % No additional arguments
        record_units=-1;      % All records
        
    case 1      % Supplied range, but no units
        error('LoadTT_simpleclust: For range of records you must specify record_units');
        
    case 2      % User specified range of values to get
        records_to_get=varargin{1};
        record_units=varargin{2};
        
    otherwise
        error('LoadTT_simpleclust: Too many input arguments');
end

timestamps=mua.ts;
numsamples=size(mua.waveforms, 2)/4;
waveforms(:,1,:)=mua.waveforms(:,1:numsamples);
waveforms(:,2,:)=mua.waveforms(:,numsamples+1:2*numsamples);
waveforms(:,3,:)=mua.waveforms(:,2*numsamples+1:3*numsamples);
waveforms(:,4,:)=mua.waveforms(:,3*numsamples+1:4*numsamples);


switch record_units
    
    case -1         % All records
        index=1:length(timestamps);
        t=timestamps(index);
        
    case 1          % Timestamp list
        index=find(intersect(timestamps,records_to_get));
        t=timestamps(index);
        
    case 2          % Record number list
        index=records_to_get;
        t=timestamps(records_to_get);
        
    case 3          % Timestamp range
        index=find(timestamps >= records_to_get(1) & timestamps <= records_to_get(2));
        t=timestamps(index);
        
    case 4          % Record number range
        index=records_to_get(1):1:records_to_get(2);
        t=timestamps(index)';
        
    case 5         % return spike count
        t=length(timestamps);      % value returned is not timestamp, but rather spike count
        
    otherwise
        error('OpenEphysLoadingEngine:Invalid argument for record_units');
        
end
varargout{1}=t;
if nargout == 2
    varargout{2}=waveforms(index,:,:); 
end