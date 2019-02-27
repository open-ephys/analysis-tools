function L=list_open_ephys_binary(jsonFile, type)
%function L=list_open_ephys_binary(oebinFile, type)
%
%Lists the elements present in a Open Ephys binary format recording
%  oebinFile: The path for the structure.oebin json file
%  type: The type of data to load. Can be 'continuous', 'events' or
%  'spikes'
%
%Returns a cell array with the folder_name field of each entry
%
%Requires minimum MATLAB version: R2016b

json=jsondecode(fileread(jsonFile));

switch type
    case 'continuous'
         data=json.continuous;
    case 'spikes'
        data=json.spikes;
    case 'events'
        data=json.events;
    otherwise
        error('Data type not supported');
end

len=length(data);
L=cell(len,1);
for i=1:len
    if (iscell(data))
        L{i}=data{i}.folder_name;
    else
        L{i}=data(i).folder_name;
    end
end

end