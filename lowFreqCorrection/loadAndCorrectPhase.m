function [data, timestamps, info] = loadAndCorrectPhase(filename, cutoff)
% [data, timestamps, info] = loadAndCorrectPhase(filename, cutoff)
%
% Loads continuous data file and corrects the distortion of the low LFP 
% frequencies by the high pass filter of the amplifier (see "Artefactual 
% origin of biphasic cortical spike-LFP correlation", bioRxiv, 
% http://dx.doi.org/10.1101/051029 for details).
%
% Inputs: filename - full path filename of the file to read 
%                    (in OpenEphys .continuous format)
%           cutoff - the cutoff frequency (Hz) used in the recording 
%                    (currently only 0.1 and 1.0 values are supported) 
%
% Outputs:    data - the continuous trace after low frequency correction
%  timestamps,info - as in load_open_ephys_data
%

% The values below come from direct measurement of the equipment, as described in the bioRxiv note
switch cutoff
  case 0.1
    freq  = [0.0615    0.1214    0.1845    0.2247    0.2914    0.3732    0.8186    1.1102    2.0432    3.0051 11.1815   20.7900   30.1811];
    phase = [3.0659    2.7502    2.4215    2.1768    2.0019    1.7454    1.1285    0.8774    0.5578    0.4007 0.1024    0.0389    0.0145];
  case 1
    freq  = [0.1067    0.1775    0.2308    0.2950    0.4092    0.8221    1.1241    2.0472    2.9354   11.2952  20.3804   29.6150];
    phase = [3.6443    3.2698    2.8039    2.5902    2.2015    1.4175    1.1069    0.6644    0.4840   0.1121   0.0500    0.0213];
  otherwise
    error('loadAndCorrectPhase: this cutoff value isn''t supported')
end
freq(end+1) = 50;
phase(end+1) = 0;

[x, timestamps, info] = load_open_ephys_data_faster(filename);
x = x - mean(x);
N = 2^nextpow2(min(numel(x), info.header.sampleRate*500)); % ~500s chunks

freqX  = info.header.sampleRate * (1:N/2-1)/N; % frequency of each FFT component (to be computed below)
freqXI = freqX >= freq(1) & freqX <= freq(end); % frequencies for which phase distortion was measured
phaseDistort = zeros(N/2-1,1);
phaseDistort(freqXI)  = interp1(log(freq), phase, log(freqX(freqXI)), 'pchip');
phaseDistort(~freqXI) = 0; % The rationale is that for frequencies below freq(1) the power is so low it doesn't matter if we don't correct them.

data = zeros(size(x));
for k = 1:ceil(numel(x)/N)
  X = fft(x(N*(k-1)+1:min(N*k, numel(x))), N);    
  X = X(2:N/2); 
  X = abs(X).*exp(1i.*(angle(X) - phaseDistort)); % shift each frequency's phase by -phaseDistort
  y = ifft([0; X; 0; conj(flipud(X))]);
  data(N*(k-1)+1:min(N*k, numel(x))) = y(1:min(N*k, numel(x)) - N*(k-1));
end
