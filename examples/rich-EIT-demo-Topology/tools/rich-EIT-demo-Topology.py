import sys
import time
import socket
import thread
import numpy as np
import Gnuplot

# Required buffer length for each node buffer
SIZE_NODE_DATA = 600

# Repeat time for connect watchdog and ping
TICK = 5
# Connect watchdog. Unit: [TICK]
CONNECT_WATCHDOG = 30/TICK

TX_PORT = 8085
RX_PORT = 8086

last_sample_value = 0
x_range = np.arange(SIZE_NODE_DATA)
dummy_data = np.zeros(SIZE_NODE_DATA, dtype=np.int)
plot = Gnuplot.Data(x_range,dummy_data)
g = Gnuplot.Gnuplot()

# Attention: Skip leading zero's in IPv6 address list, otherwise address comparison with received UDP packet address goes wrong
NODE_ALIAS = 0            # Pretty node name
NODE_DATA = 1             # Time line with data to plot
NODE_CONNECTED = 2        # If 0, not connected, else connect_watchdog value
NODE_LAST_DATA = 3        # Storage for received sample
NODE_PLOT = 4             # Plot instance
node_data = {"bbbb::215:8d00:36:180"  : ["RICH001",     np.zeros(SIZE_NODE_DATA, dtype=np.int), 0, last_sample_value, plot],
             "bbbb::215:8d00:36:892"  : ["RICH196",     np.zeros(SIZE_NODE_DATA, dtype=np.int), 0, last_sample_value, plot],
             "bbbb::215:8d00:57:5b5d" : ["RICH197",     np.zeros(SIZE_NODE_DATA, dtype=np.int), 0, last_sample_value, plot],
             "bbbb::215:8d00:36:8b1"  : ["RICH193",     np.zeros(SIZE_NODE_DATA, dtype=np.int), 0, last_sample_value, plot],
             "bbbb::215:8d00:36:8b3" :  ["RICH198",     np.zeros(SIZE_NODE_DATA, dtype=np.int), 0, last_sample_value, plot]}

# List of all nodes derived from node_data list
node_list = node_data.keys()

def initPlots():
    for node in range(len(node_list)):
        node_data_key = node_data[node_list[node]]
        g.title('Rich Demonstrator: Topology Agility')
        g.ylabel('Value')
        g.xlabel('Time[s]')
        g("set yrange [-150:150]")
        g("set xrange [0:"+str(SIZE_NODE_DATA)+"]")
        node_data_key[NODE_PLOT] = Gnuplot.Data(x_range,node_data_key[NODE_DATA], title=node_data_key[NODE_ALIAS] , with_='lines')
    #g.plot(node_data[node_list[0]][NODE_PLOT],node_data[node_list[1]][NODE_PLOT],node_data[node_list[2]][NODE_PLOT],node_data[node_list[3]][NODE_PLOT],node_data[node_list[4]][NODE_PLOT])
    nodes = [ node_data[node_list[i]][NODE_PLOT] for i in xrange(len(node_list)) ]
    g.plot(*nodes)



def udpReceive():
    """RUNS ON SEPARATE THREAD """
    while True:
        data, addr = s_rx.recvfrom(128)
        node_data_key = node_data[addr[0]]
        # Indicate node is connected
        node_data_key[NODE_CONNECTED] = CONNECT_WATCHDOG
        print addr[0] + ' (' + node_data_key[NODE_ALIAS] + ') : ' + data
        # Write new data at index in data buffer. Data buffer is view of plot
        data_lock.acquire()
        node_data_key[NODE_LAST_DATA] = int(data)
        data_lock.release()

def plotGraphs():
    while True:
        data_lock.acquire()
        for node in range(len(node_list)):
            node_data_key = node_data[node_list[node]]
            for k in range(1,SIZE_NODE_DATA,1):
                node_data_key[NODE_DATA][k-1] = node_data_key[NODE_DATA][k]
            node_data_key[NODE_DATA][SIZE_NODE_DATA-1] = node_data_key[NODE_LAST_DATA]
            if node_data_key[NODE_CONNECTED] == 0:
                node_data_key[NODE_PLOT] = Gnuplot.Data(x_range,node_data_key[NODE_DATA], title=node_data_key[NODE_ALIAS], with_='dots')
            else:
                node_data_key[NODE_PLOT] = Gnuplot.Data(x_range,node_data_key[NODE_DATA], title=node_data_key[NODE_ALIAS], with_='lines')
        #g.plot(node_data[node_list[0]][NODE_PLOT],node_data[node_list[1]][NODE_PLOT],node_data[node_list[2]][NODE_PLOT],node_data[node_list[3]][NODE_PLOT],node_data[node_list[4]][NODE_PLOT])
        nodes = [ node_data[node_list[i]][NODE_PLOT] for i in xrange(len(node_list)) ]
        g.plot(*nodes)
        data_lock.release()
        time.sleep(1)


##### MAIN #####
s_tx = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
s_rx = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
s_rx.bind(('', RX_PORT))
initPlots()
data_lock = thread.allocate_lock()
thread.start_new_thread(udpReceive, ())
thread.start_new_thread(plotGraphs, ())
ping_node_index = 0
ping_msg = "ping"
while True:
    # Every 5 secs, one node of the list in pinged
    if (ping_node_index >=len(node_list)):
        ping_node_index = 0;
    try:
        print "ping " + node_data[node_list[ping_node_index]][NODE_ALIAS]
        s_tx.sendto(ping_msg, (node_list[ping_node_index], TX_PORT))
    except:
        print 'Failed to send to ' + node_list[ping_node_index]
    ping_node_index += 1
    # Update connect watchdog
    for node in range(len(node_list)):
        node_data_key = node_data[node_list[node]]
        if (node_data_key[NODE_CONNECTED] > 0):
            node_data_key[NODE_CONNECTED] -= 1
    time.sleep(TICK)





