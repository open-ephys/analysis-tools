function output_path = get_full_path(input_path)

% 
% If a truncated path is given (e.g., ~/Folder/file), this function
% returns the full path (e.g., /home/username/Folder/file).
%
% It should be cross-platform compatible.
%

if strcmp(input_path(1),'~') % need full path for 'xmlread()' to work properly
    
    if ispc
        userDir = winqueryreg('HKEY_CURRENT_USER',...
            ['Software\Microsoft\Windows\CurrentVersion\' ...
             'Explorer\Shell Folders'],'Personal');
    else
        userDir = char(java.lang.System.getProperty('user.home'));
    end
    
    output_path = [userDir input_path(2:end)];
    
else
    output_path = input_path;
end