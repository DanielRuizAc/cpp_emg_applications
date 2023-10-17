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

%% 
fileName = './jsonemg/2023-10.txt'; % filename in txt extension
% fileName = './bts_xsens_implementation/Batch.txt';
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
            index_bts = index_bts + length(ch_data);
        end
    end
end

figure
for i=1:size(channels,1)
    subplot(size(channels,1),1,i)
    plot(channels_index(i,:)*0.001 ,channels(i,:))
end

fr = 1000;

f = fr*(0:length(channels_index(1,:))/2 )/length(channels_index(1,:));

for i=1:size(channels,1)
    four = fft(channels(i,:));
    four = abs(four/length(four));
    four = four(1, 1:(length(four)/2+1));
    four(1,2:end-1) = 2*four(1,2:end-1);
    chan_four(i,:) = four;
end

figure
for i=1:size(channels,1)
    subplot(size(channels,1),1,i)
    plot(f ,chan_four(i,:))
end

nq_freq = fr/2;
cutoff_low = 10;
cutoff_high = 200;

[b,a] = butter(4,[cutoff_low,cutoff_high]/fr,"bandpass");

for i=1:size(channels,1)
    filtered_data(i,:) = filtfilt(b,a,channels(i,:));
end

figure
for i=1:size(channels,1)
    subplot(size(channels,1),1,i)
    plot(channels_index(i,:)*0.001 ,filtered_data(i,:))
end

WinLen = 100;                                            % Window Length For RMS Calculation
for i=1:size(channels,1)
    rmsv(i,:) = sqrt(movmean(abs(filtered_data(i,:)).^2, WinLen));
end

% rmsv = sqrt(movmean(abs(channels).^2, WinLen));    

figure
for i=1:size(channels,1)
    subplot(size(channels,1),1,i)
    plot(channels_index(i,:)*0.001 ,rmsv(i,:))
end
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