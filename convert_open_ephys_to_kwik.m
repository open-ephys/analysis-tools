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

%%

% 1. create the KWIK file

kwikfile = [get_full_path(output_directory) filesep ...
        'session_info.kwik'];
    
disp(['Writing ' kwikfile '...'])
    
info.kwikfile = kwikfile;
    
if numel(dir(kwikfile))
    delete(kwikfile)
end

H5F.create(kwikfile);
h5writeatt(kwikfile, '/', 'kwik_version', '2')


%%

% 2. create the KWD files

processor_index = 0;

for processor = 1:size(info.processors,1)
    
    recorded_channels = info.processors{processor, 3};
    
    if ~isempty(recorded_channels)

        processor_index = processor_index + 1;

        % initialize this processor's kwd file
        kwdfile = [get_full_path(output_directory) filesep ...
            int2str(info.processors{processor,1}) '_raw.kwd'];
        info.kwdfiles{processor_index} = kwdfile;

        if numel(dir(kwdfile))
            delete(kwdfile);
        end
        
        disp(['Writing ' kwdfile '...']);

        h5create(kwdfile, '/kwik_version', [1 1], 'Datatype', 'int16');
        h5write(kwdfile, '/kwik_version', int16(2));
        h5writeatt(kwdfile, '/', 'kwik_version', 2);

        num_channels = length(recorded_channels);
        for ch = 1:num_channels

            filename_in = [input_directory filesep ...
                int2str(info.processors{processor, 1}) ...
                '_CH' int2str(recorded_channels(ch)) '.continuous'];

            [data, timestamps, info_continuous] = load_open_ephys_data_faster(filename_in, 'unscaledInt16');

            recording_blocks = unique(info_continuous.recNum);
            block_size = info_continuous.header.blockLength;

            for X = 1:length(recording_blocks)

                in_block = find(info_continuous.recNum == recording_blocks(X));
                start_sample = (in_block(1)-1)*block_size+1;
                end_sample = (in_block(end)-1)*block_size+block_size;

                this_block = int16(data(start_sample:end_sample));

                internal_path = ['/recordings/' int2str(X-1)];

                if ch == 1 % only create dataset and write attributes once per recording block

                    if processor_index == 1 % only write to the kwik file for the first processor
                        h5create(kwikfile, [internal_path '/start_sample'], [1 1],...
                            'Datatype', 'int64');
                        h5write(kwikfile, [internal_path '/start_sample'], int64(timestamps(start_sample)));

                        h5create(kwikfile, [internal_path '/sample_rate'], [1 1],...
                            'Datatype', 'int16');
                        h5write(kwikfile, [internal_path '/sample_rate'], int16(info_continuous.header.sampleRate));
                    end
    
                    h5create(kwdfile, [internal_path '/data'], ...
                        [num_channels numel(this_block)], ...
                        'Datatype', 'int16', ...
                        'ChunkSize', [1 numel(this_block)]);
                    
                    h5writeatt(kwdfile, internal_path, 'start_sample', int64(timestamps(start_sample)));
                    h5writeatt(kwdfile, internal_path, 'sample_rate', int16(info_continuous.header.sampleRate));

                    h5writeatt(kwdfile, internal_path, 'bit_depth', info_continuous.header.bitVolts);
                    h5create(kwdfile, [internal_path '/application_data/channel_bit_volts'], [1 num_channels], ...
                        'DataType', 'double');
                end
                
                h5write(kwdfile, [internal_path '/application_data/channel_bit_volts'], ...
                    info_continuous.header.bitVolts, [1 ch], [1 1]);

                h5write(kwdfile,[internal_path '/data'], ...
                    (this_block(1:end))', [ch 1], [1 numel(this_block)]);

            end

        end

    end
end

%%

% 3. create the KWX file

kwxfile = [get_full_path(output_directory) filesep ...
    'spikes.kwx'];

info.kwxfile = kwxfile;

if numel(dir(kwxfile))
    delete(kwxfile)
end

for X = 1:size(info.electrodes,1)
    
    filename_string = info.electrodes{X, 1};
    channels = info.electrodes{X, 2};
    
    internal_path = ['/channel_groups/' int2str(X-1)];
    
    for ch = 1:length(channels)
       
        h5create(kwikfile, [internal_path '/' int2str(ch-1) '/kwd_index'], [1 1], 'Datatype', 'int16');
        h5write(kwikfile, [internal_path '/' int2str(ch-1) '/kwd_index'], int16(channels(ch)));
    end
    
    h5writeatt(kwikfile, internal_path, 'name', filename_string);
    
    filename_string(filename_string == ' ') = [ ];
    
    filename_in = [input_directory filesep ...
        filename_string '.spikes'];
    
    [data, timestamps, info_spikes] = load_open_ephys_data(filename_in);
    
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

