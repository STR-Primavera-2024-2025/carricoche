clear all;
clc;
delete(instrfind);

% u=udp('169.254.88.200',8888,'localPort',56200);
u=udp('192.168.1.150',8888,'localPort',8888);
fopen(u);


%this causes a reset
% s=serial('COM14', 'BaudRate', 115200,'Terminator','LF','TimeOut',20);
% fopen(s);
% fclose(s);
dataVector=[];

try
    close all;
    figure;
    hold on;
    grid on;
    box on;
    % ylim([0 360]);
    data1_k_1=0;
    data2_k_1=0;
    data3_k_1=0;
    data4_k_1=0;
    for i=1:1:5
        for i=1:1:100
            data=fscanf(u,'%f,%f,%f,%f')%fread(u);    
            plot([i-1 i],[data1_k_1 data(1)],'-','Color',[0 0.4470 0.7410]);
            plot([i-1 i],[data2_k_1 data(2)],'-','Color',[0.8500 0.3250 0.0980]);
            plot([i-1 i],[data3_k_1 data(3)],'-','Color',[0.9290 0.6940 0.1250]);
            plot([i-1 i],[data4_k_1 data(4)],'-','Color',[0.4940 0.1840 0.5560]);        
            xlim([i-50 i]);
            drawnow;
            data1_k_1=data(1);
            data2_k_1=data(2);
            data3_k_1=data(3);
            data4_k_1=data(4);
            dataVector=[dataVector data];
        end
        myData=dataVector';%[Va Vb Vc];
        TS_ECG = timescope('SampleRate', 10, 'TimeSpanSource', 'Auto', 'ShowGrid', true);
        TS_ECG(myData);
        TS_ECG.YLimits = [min(min(myData))*1.05, max(max(myData))*1.05];
        myData=[];
        figure;
        hold on;
        grid on;
        box on;
    end
    
    
% text_char=char(text)'
% text=fread(u);
% text_char=char(text)'
% text=fread(u);
% text_char=char(text)'

% fwrite(u,'hi!!!');
% pause(1);
% text=fread(u);
% text_char=char(text)'
% pause(1);
% fwrite(u,'bye!!!');
% pause(1);
% text=fread(u);
% text_char=char(text)'
% pause(1);


catch Me
    Me.identifier
    close all;
    disp("Error. Closing udp connection...");
    fclose(u);
    return;
end
% fclose(u);
