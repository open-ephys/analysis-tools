[settingsXmlName,pathName] = uigetfile({'*.xml','Settings XML File'},'select a settings file'); 
ephysInfo = getEphysInfo(pathName);
FileNames=cell(3,1);
FileHeaders=cell(3,1);
FileNames{1,1}=[pathName ephysInfo.events_file];   %Events File
FileHeaders{1,1}=getFileHeader([pathName ephysInfo.events_file]);

%Spike Channels
FileNames{2,1}=cell(size(ephysInfo.electrodes,1),2);
FileHeaders{2,1}=cell(size(ephysInfo.electrodes,1),1);
for i=1:size(ephysInfo.electrodes,1)
    electrodeName = ephysInfo.electrodes{i,1};
    electrodeName(electrodeName==' ') = '';
    if ~isempty(strfind(electrodeName,'SingleElectrode')) 
        electrodeName=strrep(electrodeName,'SingleElectrode','SE');
    end
    if ~isempty(strfind(electrodeName,'Stereotrode'))
        electrodeName=strrep(electrodeName,'Stereotrode','ST');
    end
    if ~isempty(strfind(electrodeName,'Tetrode'))
        electrodeName=strrep(electrodeName,'Tetrode','TT');
    end
    SpikeFileName = [pathName electrodeName '.spikes'];
    FileNames{2,1}{i,1} = SpikeFileName;
    FileNames{2,1}{i,2} = ephysInfo.electrodes{i,2};
    FileHeaders{2,1}{i,1}=getFileHeader(SpikeFileName);
end
clear electrodeName SpikeFileName;

%Continuous Channels
for i=1:size(ephysInfo.processors,1)
    if strcmp(ephysInfo.processors{i,2},ConProcessor)
        FileNames{3,1}=cell(size(ephysInfo.processors{i,3},2),2);
        FileHeaders{3,1}=cell(size(ephysInfo.processors{i,3},2),1);
        for j=1:size(ephysInfo.processors{i,3},2)
            ContinuousFileName = [pathName num2str(ephysInfo.processors{i,1}) '_CH' num2str(ephysInfo.processors{i,3}(1,j)) '.continuous'];
            FileNames{3,1}{j,1}=ContinuousFileName;
            FileNames{3,1}{j,2}=ephysInfo.processors{i,3}(1,j);
            FileHeaders{3,1}{j,1}=getFileHeader(ContinuousFileName);
        end
    end
end
clear ContinuousFileName;
