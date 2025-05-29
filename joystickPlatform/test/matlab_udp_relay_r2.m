u=udp('192.168.1.101',8888,'localPort',8888,'timeout',2);
fopen(u)

for i=1:1:2
    % fread(u)
    data=fscanf(u,'%c')%
end

% return;

% "{\"vBatt\":0.00,\"ref\":00.00,\"data\":[48.756080,2.302038]}"

fwrite(u,'H1');
pause(1);
data=fscanf(u,'%c')
fwrite(u,'H2');
pause(1);
data=fscanf(u,'%c')
fwrite(u,'H3');
pause(1);
data=fscanf(u,'%c')
fwrite(u,'H4');
pause(1);
data=fscanf(u,'%c')

fwrite(u,'L1');
pause(1);
data=fscanf(u,'%c')
fwrite(u,'L2');
pause(1);
data=fscanf(u,'%c')
fwrite(u,'L3');
pause(1);
data=fscanf(u,'%c')
fwrite(u,'L4');
pause(1);
data=fscanf(u,'%c')

fwrite(u,'T1');
fwrite(u,'T2');
fwrite(u,'T3');
fwrite(u,'T4');
pause(1);
data=fscanf(u,'%c')

fwrite(u,'T1');
fwrite(u,'T2');
fwrite(u,'T3');
fwrite(u,'T4');
pause(1);
data=fscanf(u,'%c')

fwrite(u,'S1');
fwrite(u,'S2');
fwrite(u,'S3');
fwrite(u,'S4');
pause(1);
data=fscanf(u,'%c')

fwrite(u,'Adios UDP...');
pause(1);
data=fscanf(u,'%c')

fclose(u);



