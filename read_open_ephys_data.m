function [data, timestamps] = read_open_ephys_data(filename)

fid = fopen(filename);

%while ~eof(fid)
for n = 1:100
   
    timestamps(n) = fread(fid, 1, 'int64');
    
    nsamples = fread(fid, 1, 'int32');
    
    data = fread(fid, nsamples, 'int16');
    
    
end

fclose(fid);