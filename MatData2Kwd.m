function MatData2Kwd(data,Output,Name,Fs,varargin)
%MatData2Kwd Function to convert matlab data to kwd. (Open Ephys GUI 
% compatible file). Variable data must be a ch x samples matrix. For LFP
% signal models, generated in matlab and fluctuating from -1 to 1, we
% recommend a multiplication of 1000 for better visualization on LFP viewer.
%
% 
% MatData2Kwd(data,Output,Name,Fs,[scale]=false)
%   data is a matrix of Channels x Record data; Output is the folder adress
%   which .kwd file will be saved; Name is the string which will name your
%   file; Fs is the frequency sample or sample rate of your matrix. We 
%   recommend to use the same sample rate as the options avaible in the
%   Rhythm FPGA in Open Ephys GUI. If scale is set to true, rescales the
%   data assuming a bitVolts value of 0.195, such that the data is
%   displayed at the same scale in the GUI.
%
%   Example: 
% 
%   Fs = 30000;
%   dt = 1/Fs;
%   for ch = 1:6
%       data(ch,:) = 1000*sin(2*pi*4*(0:dt:1*60));
%   end
% 
%   MatData2Kwd(data,'/home/yourDirectory/GUI/','SampleFile',Fs)
%

    narginchk(4,5)
    
    bit_volts = 0.195;
    scale_val = 1;
    if ~isempty(varargin) && varargin{1}
        scale_val = bit_volts;
    end
        
    kwdfile = [Output Name '.kwd'];
    internal_path = '/recordings/0';
    
    if numel(dir(kwdfile))
        delete(kwdfile);
    end

    h5create(kwdfile, [internal_path '/data'], ...
        [size(data,1) size(data,2)], ...
        'Datatype', 'int16');
                        
    h5writeatt(kwdfile,'/recordings/0','name',Name);
    h5writeatt(kwdfile,'/recordings/0','start_time',0);
    h5writeatt(kwdfile,'/recordings/0','sample_rate',Fs);
    h5writeatt(kwdfile,'/recordings/0','bit_depth',bit_volts);
    h5writeatt(kwdfile, '/', 'kwik_version', 2);
             

    this_block = int16(data/scale_val);
                    
    h5write(kwdfile,'/recordings/0/data', ...
            this_block);
                
end
       
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   $Authors: Eliezyer Fermino de Oliveira and   %
% Cleiton Lopes Aguiar                           % 
%                                                %
%   $Data: June 13, 2016                         %
%   $Version: 1.0.0                              %
%                                                %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
