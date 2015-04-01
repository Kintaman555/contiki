#! /bin/sh
echo "Starting tests..."

mypath="/home/beshr/work/contiki-private/examples/tsch-testbed/imgs/"

java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/home/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-no-rpl-unicast-dgm-full.csc
tar -czf $(mypath)exp.tar.gz $(mypath)*.txt  | uuencode $(mypath)exp.tar.gz exp.tar.gz | mail -s "Cooja exp" beshr@chalmers.se
mv $(mypath)exp.tar.gz $(mypath)done
mv $(mypath)*.txt$(mypath)done

java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/home/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-no-rpl-unicast-dgm-short.csc
tar -czf $(mypath)exp.tar.gz $(mypath)*.txt  | uuencode $(mypath)exp.tar.gz exp.tar.gz | mail -s "Cooja exp" beshr@chalmers.se
mv $(mypath)exp.tar.gz $(mypath)done
mv $(mypath)*.txt$(mypath)done

java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/home/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-rpl-collect-only-sb.csc
tar -czf $(mypath)exp.tar.gz $(mypath)*.txt  | uuencode $(mypath)exp.tar.gz exp.tar.gz | mail -s "Cooja exp" beshr@chalmers.se
mv $(mypath)exp.tar.gz $(mypath)done
mv $(mypath)*.txt$(mypath)done

java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/home/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-rpl-collect-only-rb.csc
tar -czf $(mypath)exp.tar.gz $(mypath)*.txt  | uuencode $(mypath)exp.tar.gz exp.tar.gz | mail -s "Cooja exp" beshr@chalmers.se
mv $(mypath)exp.tar.gz $(mypath)done
mv $(mypath)*.txt$(mypath)done

java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/home/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-rpl-collect-only-min.csc
tar -czf $(mypath)exp.tar.gz $(mypath)*.txt  | uuencode $(mypath)exp.tar.gz exp.tar.gz | mail -s "Cooja exp" beshr@chalmers.se
mv $(mypath)exp.tar.gz $(mypath)done
mv $(mypath)*.txt$(mypath)done

echo "Done!"
