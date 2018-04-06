function info = convert_open_ephys_to_kwik(varargin)
%
% Converts Open Ephys data to KWIK format
%
%   by Josh Siegle, November 2013
%
%  info = convert_open_ephys_to_kwik(input_directory, output_directory)
%
%  input_directory: folder with Open Ephys data
%  output_directory (optional): folder to save the KWIK files
%    - defaults to using the input_directory
%
% KWIK file contains:
% - metadata
% - spikes times
% - clusters
% - recording for each spike time
% - probe-related information
% - information about channels
% - information about cluster groups
% - events, event types
% - aesthetic information, user data, application data
%
% KWX file contains:
% - spike features, masks, waveforms
%
% KWD file contains:
% - raw/filtered recordings
%
% all files contain a kwik_version attribute (currently equal to 2)
%
% PRM = processing parameters
% PRB = probe parameters

input_directory = varargin{1};

if (nargin == 1)
    output_directory = input_directory;
else
    output_directory = varargin{2};
end

info = get_session_info(input_directory);

% loop over experiments

for kE = 1:length(info)
    
    experiment_prefix = ['experiment' num2str(kE)];
    
    %%
    
    % 1. create the KWIK file
    
    kwikfile = [get_full_path(output_directory) filesep ...
        experiment_prefix '_session_info.kwik'];
    
    disp(['Writing ' kwikfile '...'])
    
    info(kE).kwikfile = kwikfile;
    
    if numel(dir(kwikfile))
        delete(kwikfile)
    end
    
    H5F.create(kwikfile);
    h5writeatt(kwikfile, '/', 'kwik_version', '2')
    
    
    %%
    
    % 2. create the KWD files
    
    processor_index = 0;
    
    kwik_blocks_written = [];
    
    for processor = 1:size(info(kE).processors,1)
        
        recorded_channels = info(kE).processors{processor, 3};
        
        if isempty(recorded_channels)
            continue;
        end
        
        processor_index = processor_index + 1;
        
        % initialize this processor's kwd file
        kwdfile = [get_full_path(output_directory) filesep ...
            experiment_prefix '_' ...
            int2str(info(kE).processors{processor,1}) '_raw.kwd'];
        info(kE).kwdfiles{processor_index} = kwdfile;
        
        if numel(dir(kwdfile))
            delete(kwdfile);
        end
        
        disp(['Writing ' kwdfile '...']);
        
        H5F.create(kwdfile);
        h5writeatt(kwdfile, '/', 'kwik_version', uint16(2));
        
        num_channels = length(recorded_channels);
        
        blocks_created = [];
        
        for ch = 1:num_channels
            
            filename_in = recorded_channels{ch};
            [data, timestamps, info_continuous] = load_open_ephys_data_faster(filename_in, 'unscaledInt16');
            
            recording_blocks = unique(info_continuous.recNum);
            block_size = info_continuous.header.blockLength;
            
            for block_num = recording_blocks' + 1 % make sure it's one-based so they can be used for indexing
                
                in_block = find(info_continuous.recNum == block_num - 1);
                start_sample = (in_block(1)-1)*block_size+1;
                end_sample = (in_block(end)-1)*block_size+block_size;
                
                this_block = int16(data(start_sample:end_sample));
                this_block_ts = int64(info_continuous.ts(in_block));
                
                internal_path = ['/recordings/' int2str(block_num - 1)];
                
                if block_num > length(blocks_created) || ~blocks_created(block_num)
                    % only create dataset and write attributes once per recording block                    
                    blocks_created(block_num) = true;
                    
                    if block_num > length(kwik_blocks_written) || ~kwik_blocks_written(block_num)
                        % only write to the kwik file once per block, over all processors
                        kwik_blocks_written(block_num) = true;
                        
                        h5create(kwikfile, [internal_path '/start_sample'], [1 1],...
                            'Datatype', 'int64');
                        h5write(kwikfile, [internal_path '/start_sample'], int64(timestamps(start_sample)));
                        
                        h5create(kwikfile, [internal_path '/sample_rate'], [1 1],...
                            'Datatype', 'int16');
                        h5write(kwikfile, [internal_path '/sample_rate'], int16(info_continuous.header.sampleRate));
                    end
                    
                    % keep track of whether this is multi-sample data
                    firstSampleRate(block_num) = info_continuous.header.sampleRate;
                    isMultiSample(block_num) = false;
                    
                    % datasets
                    h5create(kwdfile, [internal_path '/application_data/channel_bit_volts'], Inf, ...
                        'DataType', 'single', 'ChunkSize', 1);
                    
                    h5create(kwdfile, [internal_path '/application_data/channel_sample_rates'], Inf, ...
                        'DataType', 'single', 'ChunkSize', 1);
                    
                    h5create(kwdfile, [internal_path '/application_data/timestamps'], ...
                        [Inf length(this_block_ts)], ...
                        'Datatype', 'int64', 'ChunkSize', [1 16]);
                    
                    h5create(kwdfile, [internal_path '/data'], ...
                        [Inf numel(this_block)], ...
                        'Datatype', 'int16', 'ChunkSize', [1 2048]);
                    
                    % attributes
                    h5writeatt(kwdfile, internal_path, 'start_sample', uint32(0));
                    h5writeatt(kwdfile, internal_path, 'sample_rate', single(firstSampleRate(block_num)));
                    h5writeatt(kwdfile, internal_path, 'bit_depth', uint32(16));
                    h5writeatt(kwdfile, internal_path, 'name', sprintf('Open Ephys Recording #%d', block_num - 1));
                    h5writeatt(kwdfile, internal_path, 'start_time', uint64(this_block_ts(1)));
                end
                
                % get channel index relative to current block
                bitVoltsInfo = h5info(kwdfile, [internal_path '/application_data/channel_bit_volts']);
                relative_chan_ind = bitVoltsInfo.Dataspace.Size + 1;
                
                h5write(kwdfile, [internal_path '/application_data/channel_bit_volts'], ...
                    single(info_continuous.header.bitVolts), relative_chan_ind, 1);
                
                chanSampleRate = info_continuous.header.sampleRate;
                isMultiSample(block_num) = isMultiSample(block_num) | chanSampleRate ~= firstSampleRate(block_num);
                h5write(kwdfile, [internal_path '/application_data/channel_sample_rates'], ...
                    single(chanSampleRate), relative_chan_ind, 1);
                
                h5write(kwdfile, [internal_path '/application_data/timestamps'], ...
                    this_block_ts', [relative_chan_ind 1], [1 length(this_block_ts)]);
                
                h5write(kwdfile, [internal_path '/data'], ...
                    (this_block(1:end))', [relative_chan_ind 1], [1 numel(this_block)]);
                
            end
            
        end
        
        for block_num = find(blocks_created)
            internal_path = ['/recordings/' int2str(block_num - 1)];
            h5writeatt(kwdfile, [internal_path '/application_data'], 'is_multiSampleRate_data', uint8(isMultiSample(block_num)));
        end
    end
    
    %%
    
    % 3. create the KWX file
    
    kwxfile = [get_full_path(output_directory) filesep ...
        experiment_prefix '.kwx'];
    
    info(kE).kwxfile = kwxfile;
    
    if numel(dir(kwxfile))
        delete(kwxfile)
    end
    
    for block_num = 1:size(info(kE).electrodes,1)
        
        filename_string = info(kE).electrodes{block_num, 1};
        channels = info(kE).electrodes{block_num, 2};
        
        internal_path = ['/channel_groups/' int2str(block_num-1)];
        
        for ch = 1:length(channels)
            
            h5create(kwikfile, [internal_path '/' int2str(ch-1) '/kwd_index'], [1 1], 'Datatype', 'int16');
            h5write(kwikfile, [internal_path '/' int2str(ch-1) '/kwd_index'], int16(channels(ch)));
        end
        
        h5writeatt(kwikfile, internal_path, 'name', filename_string);
        
        filename_string(filename_string == ' ') = [ ];
        
        filename_in = [input_directory filesep ...
            filename_string '.spikes'];
        
        [data, ~, info_spikes] = load_open_ephys_data(filename_in);
        
        h5create(kwxfile, [internal_path '/waveforms_filtered'], ...
            size(data), ...
            'Datatype', 'int16', ...
            'ChunkSize',[1 size(data,2) size(data,3)]);
        
        rescaled_waveforms = data.*repmat(reshape(info_spikes.gain, ...
            [size(info_spikes.gain,1) 1 size(info_spikes.gain,2)]), ...
            [1 size(data,2) 1])./1000;
        
        h5write(kwxfile,[internal_path '/waveforms_filtered'], ...
            int16(rescaled_waveforms));
        
    end

end

