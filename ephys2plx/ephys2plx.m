% Convert Ephys-data into Plexon-data
% This function will ask the Ephys .xml files. 

% INPUT:


% Version:                                                                  
%         1.0       July 11, 2014 - Guangxing Li 
%         1.1       July 14, 2014 - Guangxing Li (input plexon filename)
%         1.2       July 18, 2014 - Guangxing Li (auto generate plexon
%                                   filename)
 
ConProcessor = 'Sources/Rhythm FPGA';

ephysConv;   %get .xml file, and read information

fullPlexonName = ephysGetPlxName(pathName);

if (exist(['ephys2plx_mex.' mexext],'file') == 3)
    ephys2plx_mex(FileHeaders,FileNames,fullPlexonName);
else
    error('Compile cere2plx_mex.c first !');
end
