% Ver 1.0 Guangxing Li  Jan 24 2014

function Header=getFileHeader(FileName)
    fid = fopen(FileName);
    hdr = fread(fid, 1024, 'char*1');
    eval(char(hdr'));
    Header=header;
end