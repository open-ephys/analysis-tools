function info = get_session_info(directory)

% STEP 1: GET FULL PATH FOR DIRECTORY

directory = get_full_path(directory);
directory_contents = dir(directory);


%%

% STEP 2: FIND NUMBER OF RECORDING SESSIONS

settingsPattern = '^settings(_\d+)?\.xml$';
isSettingsFile = regexp({directory_contents.name}, settingsPattern, 'once');
numSessions = length(cell2mat(isSettingsFile));

for s = numSessions:-1:1 % iterate backwards to preallocate
    
    if s == 1
        sessStr = '';
    else
        sessStr = ['_', num2str(s)];
    end
    
    %%
    
    % STEP 3: PARSE THE 'SETTINGS.XML' FILE   
    
    settingsFilename = ['settings' sessStr '.xml'];
    DOMnode = xmlread([directory filesep settingsFilename]);
    xRoot = DOMnode.getDocumentElement;
    
    processors = cell(0);
    electrodes = cell(0);
    
    % LOOP THROUGH TOP-LEVEL NODES
    for i = 1:xRoot.getChildNodes.getLength-1
        
        if strcmp(xRoot.item(i).getNodeName, 'SIGNALCHAIN')
            
            xSignalChain = xRoot.item(i);
            
            processorIndex = 0;
            
            % LOOP THROUGH PROCESSOR NODES
            for j = 1:xSignalChain.getChildNodes.getLength-1
                
                if strcmp(xSignalChain.item(j).getNodeName, 'PROCESSOR')
                    
                    processorIndex = processorIndex + 1;
                    
                    xProcessor = xSignalChain.item(j);
                    
                    nodeId = str2double(xProcessor.getAttributes.getNamedItem('NodeId').getValue);
                    processorName = char(xProcessor.getAttributes.getNamedItem('name').getValue);
                    
                    processors{processorIndex,1} = nodeId;
                    processors{processorIndex,2} = processorName;
                    processors{processorIndex,3} = {}; % to contain recording filenames
                    
                    if strcmp(processorName, 'Filters/Spike Detector')
                        
                        electrodeIndex = 0;
                        
                        % LOOP THROUGH ELECTRODE NODES IN SPIKE DETECTOR
                        for k = 1:xProcessor.getChildNodes.getLength-1
                            
                            if strcmp(xProcessor.item(k).getNodeName, 'ELECTRODE')
                                
                                electrodeIndex = electrodeIndex + 1;
                                
                                xElectrode = xProcessor.item(k);
                                
                                electrodeName = char(xElectrode.getAttributes.getNamedItem('name').getValue);
                                numChannels = str2double(xElectrode.getAttributes.getNamedItem('numChannels').getValue);
                                channels = zeros(1, numChannels);
                                
                                channelIndex = 0;
                                
                                % LOOP THROUGH CHANNEL NODES FOR ELECTRODE
                                for m = 1:xElectrode.getChildNodes.getLength-1
                                    
                                    if strcmp(xElectrode.item(m).getNodeName, 'SUBCHANNEL')
                                        
                                        channelIndex = channelIndex + 1;
                                        
                                        xChannel = xElectrode.item(m);
                                        
                                        ch = str2double(xChannel.getAttributes.getNamedItem('ch').getValue);
                                        
                                        channels(channelIndex) = ch;
                                        
                                    end
                                    
                                end
                                
                                electrodes{electrodeIndex,1} = electrodeName;
                                electrodes{electrodeIndex,2} = channels;
                                
                            end
                        end
                    end
                end
            end
        end
    end

    %%
    
    % STEP 4: PARSE CONTINUOUS_DATA.OPENEPHYS TO GET CONTINUOUS FILENAMES
    
    DOMnode = xmlread([directory filesep 'Continuous_Data' sessStr '.openephys']);
    xRoot = DOMnode.getDocumentElement;
    
    % loop over recordings
    for kR = 1:xRoot.getChildNodes.getLength - 1
        
        if ~strcmp(xRoot.item(kR).getNodeName, 'RECORDING')
            continue;
        end
        
        xRecording = xRoot.item(kR);        
        
        % loop through processors
        for kP = 1:xRecording.getChildNodes.getLength - 1
            if ~strcmp(xRecording.item(kP).getNodeName, 'PROCESSOR')
                continue;
            end

            xProcessor = xRecording.item(kP);
            nodeId = str2double(xProcessor.getAttributes.getNamedItem('id').getValue);
            procInd = find([processors{:, 1}] == nodeId, 1);
            if isempty(procInd)
                warning('Recorded processor number %d not found in %s', nodeId, settingsFilename);
                procInd = size(processors, 1) + 1;
                processors{procInd, 1} = nodeId;
                processors{procInd, 2} = 'Unknown Processor';
                processors{procInd, 3} = {};
            end
            
            % loop through channels
            for kC = 1:xProcessor.getChildNodes.getLength - 1
                if ~strcmp(xProcessor.item(kC).getNodeName, 'CHANNEL')
                    continue;
                end
                
                xChannel = xProcessor.item(kC);
                filename = char(xChannel.getAttributes.getNamedItem('filename').getValue);
                if ~any(strcmp(filename, processors{procInd, 3}))                
                    processors{procInd, 3}{end+1} = filename;
                end
            end
        end
    end
    
    info(s).electrodes = electrodes;
    info(s).processors = processors;
    info(s).events_file = ['all_channels' sessStr '.events'];
    info(s).messages_file = ['messages' sessStr '.events'];
end
end
