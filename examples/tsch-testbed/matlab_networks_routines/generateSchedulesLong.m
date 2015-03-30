function [ slotframeTable, linkTable, parents, expectedLatency ] = generateSchedulesLong( myNetwork, paths, sfHandle, chOffset, ebSfHandle, ebChOffset, ebPeriod, tsDuration, latencyGoal )
%GENERATESCHEDULES Summary of this function goes here
%   Detailed explanation goes here



%% generating links
linkTable = {};
numNodes = length(myNetwork);
parents = zeros(numNodes,1);
ts_cnt = 0;
% linkTableStr = '';
for i=2:length(paths)
    %str='';
    %link = '';
    etx = ones(1, length(paths{i}));
    for j=length(paths{i}):-1:2
       src = paths{i}(j);
       dst = paths{i}(j-1);
       parents(src) = dst;
       etx(j) = round(2*myNetwork(src, dst));
    end
    %disp(etx)
    etx = max(etx);
    %disp(etx)
    for k=1:etx
        for j=length(paths{i}):-1:2
           src = paths{i}(j);
           dst = paths{i}(j-1);
           parents(src) = dst;
           %ss = sprintf('(%d, %d)x%d ', src, dst, etx); 
       
           ts_cnt = ts_cnt + 1;
           ltyp = 'LINK_TYPE_NORMAL';
           node = src;
           nbr = dst;
           lopt = 'LINK_OPTION_TX | LINK_OPTION_TIME_KEEPING'; 
           % link1 = sprintf('{%d, %d, %d, %d, %d, %s, %s},\n', node, nbr, sfHandle, ts, chOffset, lopt, ltyp);
           linkCell = {node, nbr, sfHandle, ts_cnt, chOffset, lopt, ltyp};
           linkTable = [linkTable; linkCell];
           node = dst;
           nbr = src;
           lopt = 'LINK_OPTION_RX';
           % link2 = sprintf('{%d, %d, %d, %d, %d, %s, %s},\n', node, nbr, sfHandle, ts, chOffset, lopt, ltyp);
           linkCell = {node, nbr, sfHandle, ts_cnt, chOffset, lopt, ltyp};
           linkTable = [linkTable; linkCell];
           % link = sprintf('%s%s%s', link, link1, link2);
           % str = sprintf('%s%s%s','//', str, ss);
       end
    end
    %linkTableStr = sprintf('%s%s\n%s',linkTable, str, link);
end
disp(parents)

%% EB links
ebSfLength = floor(ebPeriod/tsDuration);
for i = 1:numNodes
    node = i;
    nbr = 0;
    lopt = 'LINK_OPTION_TX'; 
    ltyp = 'LINK_TYPE_ADVERTISING_ONLY';
    % link1 = sprintf('{%d, %d, %d, %d, %d, %s, %s},\n', node, nbr, sfHandle, ts, chOffset, lopt, ltyp);
    linkCell = {node, nbr, ebSfHandle, i - 1, ebChOffset, lopt, ltyp};
    linkTable = [linkTable; linkCell];
    if(parents(i) ~= 0)
        nbr = parents(i);
        lopt = 'LINK_OPTION_RX'; 
        ltyp = 'LINK_TYPE_ADVERTISING_ONLY';
        % link1 = sprintf('{%d, %d, %d, %d, %d, %s, %s},\n', node, nbr, sfHandle, ts, chOffset, lopt, ltyp);
        linkCell = {node, nbr, ebSfHandle, nbr - 1, ebChOffset, lopt, ltyp};
        linkTable = [linkTable; linkCell];
    end
end

%% slotframes
sfLength = max(ts_cnt, floor(latencyGoal/tsDuration));
expectedLatency = sfLength * tsDuration;
n = 1; %n=numNodes
slotframeTable = zeros(2*n, 3);
for i = 1:n
    % sf for data
    slotframeTable(i, :) = [i, sfHandle, sfLength];
    % sf for EB
    slotframeTable(i+n, :) = [i, ebSfHandle, ebSfLength];
end

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
end