
clear all;
clc;

myNetworkPRR=[0, 0.9, 0.9, 0.1, 0.1; 
    0.9, 0, 0.9, 0.8, 0.8;
    0.9, 0.9, 0, 0.8, 0.8;
    0.1, 0.8, 0.75, 0, 0.8;
    0.1, 0.75, 0.8, 0.8, 0];

myNetwork=1./myNetworkPRR;
disp(myNetwork.^2)
%% calculating routes
sink = 1;
target = []; %to all nodes
%note the transpose! We want to calculate the paths FROM every node TO
%sink, however, the function calculates the paths FROM the sink TO
%everynode... that's why we reverse/transpose the weights
[distances, paths] = dijkstra(myNetwork'.^2,sink, target);

%{2, 1, 10, 3, 1, LINK_OPTION_TX | LINK_OPTION_TIME_KEEPING, LINK_TYPE_NORMAL}
% typedef struct {
%   uint16_t node_id;
%   /* MAC address of neighbor */
%   uint16_t nbr_id;
%   /* Slotframe identifier */
%   uint16_t slotframe_handle;
%   /* Timeslot for this link */
%   uint16_t timeslot;
%   /* Channel offset for this link */
%   uint16_t channel_offset;
%   /* A bit string that defines
%    * b0 = Transmit, b1 = Receive, b2 = Shared, b3 = Timekeeping, b4 = reserved */
%   uint8_t link_options;
%   /* Type of link. NORMAL = 0. ADVERTISING = 1, and indicates
%      the link may be used to send an Enhanced beacon. */
%   enum link_type link_type;
% } link_t;
%% Global parameters
ts_cnt = 0;
sfHandle = 7;
ebSfHandle = 100;
chOffset = 0;
ebChOffset = 1;
numNodes = length(myNetwork);
tsDuration = 15.0/1000; 
%% generating links
linkTable = {};
% linkTableStr = '';
for i=2:length(paths)
    str='';
    link = '';
    for j=length(paths{i}):-1:2
       src = paths{i}(j);
       dst = paths{i}(j-1);
       etx = ceil(2*myNetwork(src, dst));
       ss = sprintf('(%d, %d)x%d ', src, dst, etx); 
       for k=1:etx
           ts_cnt = ts_cnt + 1;
           ts = ts_cnt;
           ltyp = 'LINK_TYPE_NORMAL';
           node = src;
           nbr = dst;
           lopt = 'LINK_OPTION_TX | LINK_OPTION_TIME_KEEPING'; 
           % link1 = sprintf('{%d, %d, %d, %d, %d, %s, %s},\n', node, nbr, sfHandle, ts, chOffset, lopt, ltyp);
           linkCell = {node, nbr, sfHandle, ts, chOffset, lopt, ltyp};
           linkTable = [linkTable; linkCell];
           node = dst;
           nbr = src;
           lopt = 'LINK_OPTION_RX';
           % link2 = sprintf('{%d, %d, %d, %d, %d, %s, %s},\n', node, nbr, sfHandle, ts, chOffset, lopt, ltyp);
           linkCell = {node, nbr, sfHandle, ts, chOffset, lopt, ltyp};
           linkTable = [linkTable; linkCell];
           % link = sprintf('%s%s%s', link, link1, link2);
           % str = sprintf('%s%s%s','//', str, ss);
       end
    end
    %linkTableStr = sprintf('%s%s\n%s',linkTable, str, link);
end

%% EB links
ebPeriod = 2;
ebSfLength = floor(ebPeriod/tsDuration);
ebTsCnt = 0;
for i = 1:numNodes
    ebTsCnt = ebTsCnt + 1;
    node = i;
    nbr = 0;
    lopt = 'LINK_OPTION_TX'; 
    ltyp = 'LINK_TYPE_ADVERTISING_ONLY';
    % link1 = sprintf('{%d, %d, %d, %d, %d, %s, %s},\n', node, nbr, sfHandle, ts, chOffset, lopt, ltyp);
    linkCell = {node, nbr, ebSfHandle, ebTsCnt, ebChOffset, lopt, ltyp};
    linkTable = [linkTable; linkCell];
end

%% slotframes
latencyGoal = 0.5;
sfLength = max(ts_cnt, floor(latencyGoal/tsDuration));
slotframeTable = zeros(2*numNodes, 3);
for i = 1:numNodes
    % sf for data
    slotframeTable(i, :) = [i, sfHandle, sfLength];
    % sf for EB
    slotframeTable(i+numNodes, :) = [i, ebSfHandle, ebSfLength];
end
%% printing 
slotframeTableStr = sprintf('#define STATIC_SLOTFRAMES { \\\n');
for i = 1:size(slotframeTable, 1)
    slotframeTableStr = sprintf('%s\t{%d, %d, %d}, \\\n', slotframeTableStr, slotframeTable(i, 1), slotframeTable(i, 2), slotframeTable(i, 3));
end
slotframeTableStr = sprintf('%s}\n\n', slotframeTableStr);
slotframeTableSize = sprintf('#define NUMBER_OF_SLOTFRAMES %d\n', size(slotframeTable, 1));

linksTableStr = sprintf('#define STATIC_LINKS { \\\n');
for i = 1:length(linkTable)
    linksTableStr = sprintf('%s\t{%d, %d, %d, %d, %d, %s, %s}, \\\n', linksTableStr, linkTable{i, 1}, linkTable{i, 2}, linkTable{i, 3}, linkTable{i, 4}, linkTable{i, 5}, linkTable{i, 6}, linkTable{i, 7});
end
linksTableStr = sprintf('%s}\n\n', linksTableStr);
linkTableSize = sprintf('#define NUMBER_OF_LINKS %d\n', length(linkTable));

fileHeading = sprintf('/* %s */\n#ifndef STATIC_SCHEDULES_H_\n#define STATIC_SCHEDULES_H_\n\n', datestr(datetime));
fileFooter = sprintf('#endif /* STATIC_SCHEDULES_H_ */\n');
file = [fileHeading, slotframeTableSize, slotframeTableStr, linkTableSize, linksTableStr, fileFooter];

disp(file);
fileID = fopen('../static-schedules.h','w');
fprintf(fileID,'%s',file);
fclose(fileID);
% disp(slotframeTableSize);
% disp(slotframeTableStr);
% disp(linkTableSize);
% disp(linksTableStr);

% disp(slotframeTable);
% disp(linkTable);
% myNetwork2=[0, 2, 2, 10, 10;
% 2, 0, 2, 3, 4;
% 2, 2, 0, 4, 3;
% 10, 3, 4, 0, 3;
% 10, 4, 3, 3, 0];

% randomNet = random('normal', 60, 20, 200);
% f = @() dijkstra(randomNet,sink, target)
% timeit(f)