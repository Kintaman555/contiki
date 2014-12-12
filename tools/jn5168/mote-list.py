import sys
import re
import subprocess

flashProgram = 'c:\\Jennic\\Tools\\flashprogrammer\\FlashCLI.exe'
def list_mote():
  #There is no COM0 in windows. We use this to trigger an error message that lists all valid COM ports
  cmd=[flashProgram,'-c','COM0']
  proc = subprocess.Popen(cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,)
  stdout_value, stderr_value = proc.communicate('through stdin to stdout')
  com_str = repr(stderr_value)
  #print '\tpass through:', repr(stdout_value)
  #print '\tstderr      :', com_str
  
  ## Extract COM ports from output 
  #res = re.compile('\[\'((COM\d+)\'.?.?).*\]').match(com_str)
  res = re.compile('.*\[(.*)\].*').match(com_str)
  
  ports = []
  if res:
    portsStr=str(res.group(1))
    #print portsStr
    ports=portsStr.replace('\'', '').replace(',','').split()
  return ports

def extractInformation(inputStr):
    stdout_value=(inputStr)
    portStr=''
    macStr=''
    info=''
    
    print 'output: ', inputStr
    
    res = re.compile('(Connecting to device on COM\d+)').match(stdout_value) 
    if res:
      portStr=str(res.group(1))
      print portStr
      
    ### extracting the following information    
    '''
    Devicelabel:           JN516x, BL 0x00080006
    FlashLabel:            Internal Flash (256K)
    Memory:                0x00008000 bytes RAM, 0x00040000 bytes Flash
    ChipPartNo:            8
    ChipRevNo:             1
    ROM Version:           0x00080006
    MAC Address:           00:15:8D:00:00:35:DD:FB
    ZB License:            0x00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00
    User Data:             00:00:00:00:00:00:00:00
    FlashMID:              0xCC
    FlashDID:              0xEE
    MacLocation:           0x00000010
    Sector Length:         0x08000
    Bootloader Version:    0x00080006
    '''
    
    #res = re.compile('(Devicelabel\:.*Bootloader\sVersion\:\s+0x\w{8})').match(stdout_value) 
    #if res:
    #  info=str(res.group(1))
      
    res = re.compile('(MAC Address\:\s+(?:\w{2}\:?){8})').match(stdout_value)
    if res:
      macStr=str(res.group(1))
      print macStr
    
    res = re.compile('(Program\ssuccessfully\swritten\sto\sflash)').match(stdout_value)
    if res:
      info=info + str(res.group(1))
      
    return [str(portStr + '\n' + info), macStr] 
        
def program_motes(motes, firmwareFile):
  for m in motes:
    cmd=[flashProgram,'-c',m, '-B', '1000000', '-s', '-w', '-f', firmwareFile]
    proc = subprocess.Popen(cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,)
    stdout_value, stderr_value = proc.communicate('through stdin to stdout')   
    info=extractInformation(stdout_value)               
    print info, info[0], info[1]
      
    errors = (stderr_value)
    if errors != '':
    	print 'Errors:', errors  

def main():
   if len(sys.argv) > 2:
   	flashProgram=sys.argv[1]

   motes=list_mote()
   print 'Found JN5168 motes at:'
   print motes
   firmwareFile='#'	

   if len(sys.argv) > 2:   		
   	firmwareFile=sys.argv[2]
   elif len(sys.argv) > 1:
   	firmwareFile=sys.argv[1]		   
   
   if firmwareFile !='#' and firmwareFile !='!':    	
   	print '\nBatch programming all connected motes...\n'
   	program_motes(motes, firmwareFile)
   else:
    	print '\nNo firmware file specified.\n'
         
main()
