#!/usr/bin/env python
import os
import subprocess
import xml.etree.ElementTree as ET
from threading import Thread
from time import sleep

randomSeedInit = 123457;
emailAddress = 'beshr@chalmers.se';
emailTitle = 'Cooja exp-';
memorySize = 512;
expPath = '/home/beshr/work/contiki-private/examples/tsch-testbed/imgs/';
doneFolder = expPath + 'done';
coojaCmd = ('java -d64 -mx%dm -Djava.awt.headless=true -jar ../dist/cooja.jar -nogui=') %(memorySize);
simulationFiles = ['app-no-rpl-unicast-dgm-full.csc', 'app-no-rpl-unicast-dgm-fullNoAttenuation.csc', 'app-no-rpl-unicast-dgm-short.csc', 'app-no-rpl-unicast-dgm-shortNoAttenuation.csc', 'app-rpl-collect-only-sb.csc', 'app-rpl-collect-only-rb.csc', 'app-rpl-collect-only-min.csc'];


#tar -czf ${mypath}exp.tar.gz ${mypath}*.txt | uuencode ${mypath}exp.tar.gz | mail -s "Cooja exp" beshr@chalmers.se


def setRandomSeed(simFile, randomSeed):
    simFileTree = ET.parse(simFile)
    simFileRoot = simFileTree.getroot()
    #<randomseed>123456</randomseed>
    seedElement = simFileRoot.find('*randomseed')
    seedElement.text = str(randomSeed)
    simFileTree.write(simFile);
    
def runSimulationThread(simFile):
    global coojaCmd, emailAddress, emailTitle, expPath, randomSeedInit
    randomSeed = randomSeedInit;
    seedIncrement = 1;
    for i in range(0,3):
        setRandomSeed(expPath+simFile, randomSeed)
        runSim = coojaCmd + expPath + simFile;
        simTitle = simFile.split(".csc")[0]
        
        logFiles = expPath + 'log_' + simTitle + '*.txt'
        
        moveResultsCmd = ('mv %s %s') %(logFiles, doneFolder);
        tarFile = expPath + simTitle + '-exp.tar.gz';
        compressResults = ('tar -czvf "%s" --wildcards %s') %(tarFile, logFiles);
        encodeResults = ('uuencode %s %s') %(tarFile, tarFile);
        mailResults = 'mail -s "%s" %s' %((emailTitle +' '+ simTitle +' ' + str(randomSeed)), emailAddress);
        tarEncodeMail = ('%s; %s | %s') %(compressResults, encodeResults, mailResults);
        moveTarFile = ('mv %s %s') %(tarFile, doneFolder);
        #deleteTarFile = 'rm -f %s' %tarFile;

        subprocess.call(runSim, shell=True);
        os.system(tarEncodeMail);
        os.system(moveResultsCmd);
        os.system(moveTarFile);
        randomSeed = randomSeed + seedIncrement

if __name__ == "__main__":
    print "Starting Cooja experiments:"
    threads = []
    for simFile in simulationFiles:
        threads.append(Thread(target = runSimulationThread, args = (simFile, )))
    
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    print "Cooja experiments are done. Exiting."


    
    
    
