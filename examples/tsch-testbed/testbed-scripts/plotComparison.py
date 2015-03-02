#!/usr/bin/env python

import os
import re
import fileinput
import math
from pylab import *
import parseLogs
import pygraphviz as pgv
from plottingTools import *

barcolor = '#0a51a7'
linecolor = "none"
ecolor = '#00A876'
#colors = ['#FF9900', '#00A876', '#0a51a7', '#FF5900', '#8FD9F2', 'black']

N_NODES = 99
N_NEIGHBORS = 15.7

markers = ['s', 'o', '^', 'p', 'd']
colors = ['#FF9900', '#00A876', '#0a51a7', '#FF5900', '#8FD9F2', 'black']
linestyles = ['-', '--']
fillColors = ['#0a51a7', '#FF9900', '#00A876', '#FF5900', '#8FD9F2', 'black']

def getMarker(index):
    return markers[index % len(markers)]

def getLineStyle(index):
    return linestyles[index % len(linestyles)]
 
def getLineColor(index):
    return colors[index % len(colors)]

def getFillColor(index):
    return fillColors[index % len(fillColors)]

def stdev(data):
  avg = average(data)
  return math.sqrt(average(map(lambda x: (x - avg)**2, data)))

def plotStat(all_res, metric, filename, ylabel, legendPos="lower right", legendBbox=None, legend=True):
  fig = plt.figure(figsize=(4.5, 3.5))
  ax = fig.add_subplot(111)

  xshift = 0.4

  a = 0
  b = a+4
  c = b+2
  d = c+2
  configs = [{"mac":"nm", 'l': "Csma", 'i': a, 'color': colors[1]},
             {"mac":"cm64", "l": "ContikiMAC-64", 'i': a+1, 'color': colors[1]},
             {"mac":"cm8", "l": "ContikiMAC-8", 'i': a+2, 'color': colors[1]},
             {"mac":"tmin3", "l": "TSCH-min-3", 'i': b, 'color': colors[2]},
             {"mac":"tmin5", "l": "TSCH-min-5", 'i': b+1, 'color': colors[2]},
             #{"mac":"tmin7", "l": "TSCH-min-7", 'i': b+2, 'color': colors[2]},
#             {"mac":"trb397x31x3", "l": "TSCH-RB-3", 'i': c, 'color': colors[2]},
             {"mac":"trb397x31x7", "l": "TSCH-RB-7", 'i': c+1, 'color': colors[2]},
#             {"mac":"trb397x31x17", "l": "TSCH-RB-17", 'i': c+2, 'color': colors[2]},
#             {"mac":"trb397x31x29", "l": "TSCH-RB-29", 'i': c+3, 'color': colors[2]},
             {"mac":"tsb397x31x7", "l": "TSCH-SB-7", 'i': d, 'color': colors[2]},
#             {"mac":"tsb397x31x17", "l": "TSCH-SB-17", 'i': d+1, 'color': colors[2]},
#             {"mac":"tsb397x31x29", "l": "TSCH-SB-29", 'i': d+2, 'color': colors[2]},
             {"mac":"tsb397x31x47", "l": "TSCH-SB-47", 'i': d+1, 'color': colors[2]},
#             {"mac":"tsb397x31x47x53", "l": "TSCH-SB-47-53", 'i': d+1, 'color': colors[2]},
             ]
  
  w = 0.6
  
  for conf in configs:
      x = conf['i'] + xshift
      y = all_res[conf['mac']][metric]['stats']["avg"]
      e = all_res[conf['mac']][metric]['stats']["stdev"]
      ax.bar(x, y, w, yerr=e, color=conf['color'], ecolor=conf['color'], edgecolor=linecolor)
  
  macs = map(lambda x: x["mac"], configs)
  xlabels = map(lambda x: x["l"], configs)
  x = array(map(lambda x: x["i"], configs))
  x = x + xshift
  y = map(lambda x: all_res[x][metric]['stats']["avg"] if x in all_res else 0, macs)
  e = map(lambda x: all_res[x][metric]['stats']["stdev"] if x in all_res else 0, macs)
  #ax.bar(x, y, w, yerr=e, color=barcolor, ecolor=ecolor, edgecolor=linecolor)
    
  if metric == "Duty Cycle":
    ax.axis(ymax=6)
    ax.annotate('100', xy=(0.2, 6),
                horizontalalignment='left', verticalalignment='bottom',
                fontsize=10)
  if metric == "End-to-end Delivery Ratio":
    ax.axis(ymin=98.5, ymax=100)
    #ax.axis(ymax=1,ymin=0.001)
    #ax.set_yscale('log')
  if metric == "MAC Unicast Success":
    ax.axis(ymin=75)
  ax.axis(xmax=x[-1]+1)
  
  ax.yaxis.grid(True)
  ax.set_axisbelow(True) 
  ax.set_xticks(x + xshift)
  ax.set_xticklabels(xlabels, rotation=45, fontsize=12, horizontalalignment="right")
  ax.set_ylabel(ylabel, fontsize=16)

  fig.savefig('plots/cmp%s.pdf'%(filename), format='pdf', bbox_inches='tight', pad_inches=0)

def getFile(base, name):
    dir = os.path.join(base, 'data')
    if os.path.isdir(dir):
        for file in os.listdir(dir):
            if name in file:
                return os.path.join(dir, file)
    return None

def extractStats(metric, dir):
    dataFile = getFile(dir, metric + '_pernode.txt')
    if dataFile != None:
        gloablData = extractGlobal(dataFile)
        perNode = extractPerNode(dataFile)
        return {'global': gloablData, 'perNode': perNode}
    return None

def main():
  all_res = {}
  xp_dir = "experiments"
  metrics = ["End-to-end Delivery Ratio", "Latency", "Duty Cycle", "MAC Unicast Success"]
  for f in os.listdir(xp_dir):
    res = re.compile('Indriya_([^_]+)_[\\d]+_[\\d]+').match(f)
    if res != None:
      mac = res.group(1)
      dir = os.path.join(xp_dir, f)
      for m in metrics:
          stats = extractStats(m, dir)
          if stats == None:
            continue
          if not mac in all_res:
            all_res[mac] = {}
          if not m in all_res[mac]:
            all_res[mac][m] = {'data': []}
          all_res[mac][m]['data'].append({'dir': dir, 'stats': stats})
      
  for key in all_res:
    all_res[key]['stats'] = {}
    for m in metrics:
      data = map(lambda x: x['stats']['global']['avg'], all_res[key][m]['data'])
      all_res[key][m]['stats'] = {"avg": average(data),
                                      "stdev": stdev(data)}
      #if m == "End-to-end Delivery Ratio":
          #all_res[key][m]['stats']['avg'] = 100-all_res[key][m]['stats']['avg']
      #if m == "MAC Unicast Success":
       #   all_res[key][m]['stats']['avg'] = 100-all_res[key][m]['stats']['avg']
      print key, len(data), m, all_res[key][m]['stats']["avg"], all_res[key][m]['stats']["stdev"]
      
  #plotStat(all_res, "End-to-end Delivery Ratio", "pdr", "End-to-end Loss Rate (%)")
  plotStat(all_res, "End-to-end Delivery Ratio", "pdr", "End-to-end PDR (%)")
  plotStat(all_res, "Latency", "latency", "Latency (s)")
  plotStat(all_res, "Duty Cycle", "dc", "Duty Cycle (%)")
  #plotStat(all_res, "MAC Unicast Success", "ucprr", "MAC Loss Rate (%)")
  plotStat(all_res, "MAC Unicast Success", "ucprr", "MAC Success Rate (%)")
  
main()
