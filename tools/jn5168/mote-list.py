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
   
  #res = re.compile('\[\'((COM\d+)\'.?.?).*\]').match(com_str)
  res = re.compile('.*\[(.*)\].*').match(com_str)
  
  ports = []
  if res:
    portsStr=str(res.group(1))
    #print portsStr
    ports=portsStr.replace('\'', '').replace(',','').split()
  return ports

def program_motes(motes, firmwareFile):
  for m in motes:
    cmd=[flashProgram,'-c',m, '-B', '1000000', '-s', '-w', '-f', firmwareFile]
    proc = subprocess.Popen(cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,)
    stdout_value, stderr_value = proc.communicate('through stdin to stdout')    
    print (stdout_value)
    errors = (stderr_value)
    if errors != '':
      print 'Errors:', errors  

def main():
   motes=list_mote()
   print 'Found JN5168 motes at:'
   print motes
   if len(sys.argv) > 1:
    print '\nBatch programming all connected motes...\n'
    firmwareFile=sys.argv[1]   
    program_motes(motes, firmwareFile)
   else:
    print '\nNo firmware specified.\n'
         
main()