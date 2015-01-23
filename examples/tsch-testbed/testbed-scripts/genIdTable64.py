#!/usr/bin/env python

import re
import fileinput
import math
from pylab import *

idMacMap = {}
file = sys.argv[1]

for line in open(file, 'r').readlines():
    # match time, id, module, log; The common format of any line
    res = re.compile('^(\d+)\\tID:(\d+)\\tNode id: (\d+), Rime address: ([a-f\d:]+)').match(line)
    if res:
        time = int(res.group(1))
        id = int(res.group(2))
        currId = int(res.group(3))
        address = res.group(4)
        idMacMap[id] = {'currId': currId, 'address': address}
        
for id in idMacMap:
    if id != idMacMap[id]['currId']:
        print 'Warning: node %d has current nodeid %d!' %(id, idMacMap[id]['currId'])
        
print "Number of nodes active: %u" %(len(idMacMap))
        
for id in sorted(idMacMap):
    print "{%3d, {{0x%s}}},"%(id, idMacMap[id]['address'].replace(':', ',0x'))
