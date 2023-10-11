fileName = './Library_proveXme/20231005 083144.917.txt'; % filename in txt extension
str = fileread(fileName); % dedicated for reading files as text
json_msgs = splitlines(str); % Splitting the complete text in lines

forearm_x_positions = [];
forearm_y_positions = [];
forearm_z_positions = [];
index=1;
for i=1:(length(json_msgs)-1)
    json_packet= jsondecode(json_msgs{i});
    frames = json_packet.data;
    for j=1:(length(frames))
        forearm_x_positions(index) = frames(j).segments.LeftForeArm.position.x;
        forearm_y_positions(index) = frames(j).segments.LeftForeArm.position.y;
        forearm_z_positions(index) = frames(j).segments.LeftForeArm.position.z;
        index=index+1;
    end
end

plot(0.01:0.01:length(forearm_z_positions)*0.01,forearm_x_positions);
hold on
plot(0.01:0.01:length(forearm_z_positions)*0.01,forearm_y_positions);
hold on
plot(0.01:0.01:length(forearm_z_positions)*0.01,forearm_z_positions);
legend


% fileName = './jsonemg/Batch.txt'; % filename in txt extension
fileName = './bts_xsens_implementation/Batch.txt';
str_emg = fileread(fileName); % dedicated for reading files as text
json_msgs_emg = splitlines(str_emg); % Splitting the complete text in lines

channels = [];
index_bts = 0;
channels_index = [];
for i=1:(length(json_msgs_emg)-1)
    json_packet= jsondecode(json_msgs_emg{i});
    data_pkt = json_packet.data;
    if (isempty(data_pkt)) 
        continue
    end
    for j=1:length(data_pkt)
        ch_data = data_pkt(j).data;

        for k=1:length(ch_data)
            channels(j,index_bts+k) = ch_data(k).value;
            channels_index(j,index_bts+k) = ch_data(k).index;
        end
        if (j==length(data_pkt))
            disp(1)
            index_bts = index_bts + length(ch_data);
        end
    end
end

WinLen = 10;                                            % Window Length For RMS Calculation
rmsv = sqrt(movmean(channels.^2, 100));    

figure
plot(channels_index(1,:)*0.001 ,channels(1,:))

figure
plot((1:length(rmsv))*0.001,rmsv)
%% 
figure
plot(channels_index(2,:)*0.001 ,channels(2,:))

figure
plot(channels_index(3,:)*0.001 ,channels(3,:))

figure
plot(channels_index(4,:)*0.001 ,channels(4,:))

%%

clear
fileName = './bts_xsens_implementation/Batch.txt';
str_emg = fileread(fileName); % dedicated for reading files as text
json_x_sens_msgs_emg = splitlines(str_emg); % Splitting the complete text in lines

index_bts = 0;
index_xsens = 1;

for i=1:(length(json_x_sens_msgs_emg)-1)
    json_packet= jsondecode(json_x_sens_msgs_emg{i});
    bts_data_pkt = json_packet.bts.data;
    frames = json_packet.xsens.data;
    if (isempty(bts_data_pkt)) 
        continue
    end
    for j=1:length(bts_data_pkt)
        ch_data = bts_data_pkt(j).data;

        for k=1:length(ch_data)
            channels(j,index_bts+k) = ch_data(k).value;
            channels_index(j,index_bts+k) = ch_data(k).index;
        end
        if (j==length(bts_data_pkt))
            index_bts = index_bts + length(ch_data);
        end
    end
    
    for j=1:(length(frames))
        forearm_x_positions(index_xsens) = frames(j).segments.RightForeArm.position.x;
        forearm_y_positions(index_xsens) = frames(j).segments.RightForeArm.position.y;
        forearm_z_positions(index_xsens) = frames(j).segments.RightForeArm.position.z;

        forearm_ang_positions_x(index_xsens) = frames(j).joints.RightUpperArm_RightForeArm.x;
        forearm_ang_positions_y(index_xsens) = frames(j).joints.RightUpperArm_RightForeArm.y;
        forearm_ang_positions_z(index_xsens) = frames(j).joints.RightUpperArm_RightForeArm.z;
        index_xsens=index_xsens+1;
    end

end