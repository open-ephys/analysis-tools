function info = get_session_info(directory)

% STEP 1: GET FULL PATH FOR DIRECTORY

directory = get_full_path(directory);

%%

% STEP 2: PARSE THE 'SETTINGS.XML' FILE

directory_contents = dir(directory);

DOMnode = xmlread([directory filesep 'settings.xml']);
xRoot =DOMnode.getDocumentElement;

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
                processors{processorIndex,3} = []; % empty cell for channels

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

% STEP 3: FIND THE AVAILABLE FILES

for filenum = 1:length(directory_contents)
    
   if numel(strfind(directory_contents(filenum).name, '.continuous')) > 0
      
       fname = directory_contents(filenum).name;
       
       underscore = strfind(fname, '_');
       chstr = strfind(fname, 'CH');
       dot = strfind(fname, '.');
       
       nodeId = str2num(fname(1:underscore-1));
       chNum = str2num(fname(chstr+2:dot-1));

       for n = 1:size(processors,1)
           if nodeId == processors{n,1}
               processors{n,3} = [processors{n,3} chNum];
           end
       end
   end
end

for n = 1:size(processors,1)
   
    processors{n,3} = sort(processors{n,3});
    
end

info.electrodes = electrodes;
info.processors = processors;
info.events_file = 'all_channels.events';
info.messages_file = 'messages.events';
end
