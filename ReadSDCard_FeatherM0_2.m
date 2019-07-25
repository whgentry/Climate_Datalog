%% Reads Files of SD card from Feather M0
% -Pulls all the files from SD card and saves 
%  them to current computer directory

%----------- Parameters to change ------------
% COM port to connect to feather, to find:
% On Windows:
% -open 'Device Manager'
% -expand 'PORTS'
% -look for the options, most likely something greater than COM1
PortNum = 'COM19';
%---------------------------------------------

%% Setup the Object
% Find a serial port object.
obj1 = instrfind('Type', 'serial', 'Port', PortNum, 'Tag', '');

% Create the serial port object if it does not exist
% otherwise use the object that was found.
if isempty(obj1)
    obj1 = serial(PortNum);
else
    fclose(obj1);
    obj1 = obj1(1);
end

% Set parameters of serial
obj1.Baudrate = 115200;
obj1.Terminator = 'CR/LF';

% Connect to instrument object, obj1.
fopen(obj1);

%% Get Read the SD card data
T_START = "TRANSFER START" + char(13) + newline;
T_END = "TRANSFER END" + char(13) + newline;
F_START = "FILE START" + char(13) + newline;
F_END = "FILE END" + char(13) + newline;
CMD_GET_DATA = 'D';


flushinput(obj1);
fprintf(obj1,CMD_GET_DATA);
data1 = fscanf(obj1);
if data1 == T_START
    fprintf("Starting Transfer:\n");
    % Receive All Files
    data1 = fscanf(obj1);
    while data1 ~= T_END
        fileName = data1(1:end-2);
        fid = fopen(fileName,'w');
        fprintf('  %s...',fileName);
        % Read File
        data1 = fscanf(obj1);
        while data1 ~= F_END
            fprintf(fid,"%s",data1);
            data1 = fscanf(obj1);
        end
        fclose(fid);
        fprintf('Complete\n');
        
        data1 = fscanf(obj1);
    end
    fprintf("Transfer Complete\n");
end

fclose(obj1);


