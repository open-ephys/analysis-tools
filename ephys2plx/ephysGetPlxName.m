% To generate plexon file name
% V1.0 July 22, 2014 - GL 

function PlxName = ephysGetPlxName(pathName)
    if ispc
        indexFolder = find(pathName=='\');
    elseif ismac
        indexFolder = find(pathName=='/');
    else
        error('Unknown Operating System');
    end
    CSM2Name = pathName(indexFolder(end-1)+1:indexFolder(end-1)+9);
    PlxName = [pathName(1:indexFolder(end-1)) CSM2Name '_P.plx'];
end