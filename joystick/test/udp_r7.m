clear all;
clc;
delete(instrfind);

u = udpport("IPV4",'LocalPort',9999);
TS_ECG = timescope('SampleRate', 100, 'TimeSpanSource', 'Auto', 'ShowGrid', true);
TS_Voltage = timescope('SampleRate', 10000, 'TimeSpanSource', 'Auto', 'ShowGrid', true,'Position',[240 593 800 330]);
TS_Current = timescope('SampleRate', 10000, 'TimeSpanSource', 'Auto', 'ShowGrid', true,'Position',[240 262 800 330]);

% initTime=tic;


for (i=1:1:20)
    data = readline(u);
    if (data~='OSC')
        data = char(readline(u));
        if (data(1)=='A')
            Va=str2num(data(3:end))';
        else
            Va=0;
        end
        data = char(readline(u));
        if (data(1)=='B')
            Vb=str2num(data(3:end))';
        else
            Vb=0;
        end
        data = char(readline(u));
        if (data(1)=='C')
            Vc=str2num(data(3:end))';
        else 
            Vc=0;
        end        
        data = char(readline(u));
        if (data(1)=='D')
            Ia=str2num(data(3:end))';
        else
            Ia=0;
        end
        data = char(readline(u));
        if (data(1)=='E')
            Ib=str2num(data(3:end))';
        else 
            Ib=0;
        end
        data = char(readline(u));
        if (data(1)=='F')
            Ic=str2num(data(3:end))';
        else
            Ic=0;
        end  
        
%         [length(Va) length(Vb) length(Vc)];
%         [length(Ia) length(Ib) length(Ic)];
        if ( (length(Va)==length(Vb)) && (length(Vb)==length(Vc)) && (length(Va)>1))
            myDataVoltage=[Va Vb Vc];            
            TS_Voltage(myDataVoltage);
            TS_Voltage.YLimits = [min(min(myDataVoltage))*1.05, max(max(myDataVoltage))*1.05];
            drawnow;
        else
            disp('ErrorV...');  
        end
        if ( (length(Ia)==length(Ib)) && (length(Ib)==length(Ic)) && (length(Ia)>1))
            myDataCurrent=[Ia Ib Ic];
            TS_Current(myDataCurrent);     
            TS_Current.YLimits = [min(min(myDataCurrent))*1.05, max(max(myDataCurrent))*1.05];
            drawnow;
        else
            disp('ErrorI...');
        end
    end
    
end
