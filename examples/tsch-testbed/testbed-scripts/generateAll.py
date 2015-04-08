#!/usr/bin/env python

import os
import sys
import re
import fileinput
import math
import parseLogs
import plottingTools
import extractFromTrace
import generateSummaryPlots
import plotComparison
        
def extract(dir):
    
    print "Looping over all experiments"
    for file in os.listdir(dir):
        path = (os.path.join(dir, file))
        print path,
        sys.stdout.flush()
        if not os.path.exists(os.path.join(path, 'log.txt')):
        	print " not an experiment directory."
        	continue
        if os.path.exists(os.path.join(path, 'ongoing')):
        	print " is ongoing"
        	continue
        if os.path.exists(os.path.join(path, 'plots/allplots.pdf')):
            print " already done."
            continue
        #if not file.startswith("Indriya_tsb") and not file.startswith("Indriya_trb"):
            #print "not TSCH"
           # continue
        #if os.path.exists(os.path.join(path, 'contentionlog.txt')):
         #   print " already done."
          #  continue
        #if os.path.exists(os.path.join(path, 'probing.txt')):
            #print " already done."
            #continue
        #if not file.startswith("Indriya_prb"):
         #   print "not a probing"
          #  continue
        
        print " extracting data...",
        sys.stdout.flush() 
        os.system("python extractFromTrace.py %s > /dev/null" %path)
        
        print " generating plots...",
        sys.stdout.flush()
        os.system("python generateSummaryPlots.py %s > /dev/null" %path)
        
        print " done."

def extractAll(baseDir):
    plottingTools.structureFolders(baseDir)
    print "Looping over all experiments"
    for dir in os.listdir(baseDir):
        print dir
        subdir = os.path.join(baseDir, dir)
        if not os.path.isdir(subdir):
            continue
        for file in os.listdir(subdir):
            path = (os.path.join(subdir, file))
            print path,
            sys.stdout.flush()
            
            if not os.path.exists(os.path.join(path, 'log.txt')):
                print " not an experiment directory."
                continue
            
            if os.path.exists(os.path.join(path, 'ongoing')):
                print " is ongoing"
                continue
            if os.path.exists(os.path.join(path, 'plots/allplots.pdf')):
                print " already done."
                continue
            
            if os.path.exists(os.path.join(path, 'log.txt')):
                print path
#                 print " extracting data...",
#                 sys.stdout.flush() 
#                 extractFromTrace.mainFunction(path)
#                 
#                 print " generating plots...",
#                 sys.stdout.flush()
#                 generateSummaryPlots.mainFunction(path)     
                print " extracting data...",
                sys.stdout.flush() 
                os.system("python extractFromTrace.py %s > /dev/null" %path)
                
                print " generating plots...",
                sys.stdout.flush()
                os.system("python generateSummaryPlots.py %s > /dev/null" %path)       
                
            print " done."
    plotComparison.plotJammed(baseDir)
    
if __name__=='__main__':
   extractAll(sys.argv[1])    
   #extract('experiments/')
