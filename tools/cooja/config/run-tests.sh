#! /bin/sh
echo "Starting tests..."

java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/Users/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-no-rpl-unicast-dgm-full.csc
java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/Users/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-no-rpl-unicast-dgm-short.csc
java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/Users/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-rpl-collect-only-sb.csc
java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/Users/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-rpl-collect-only-rb.csc
java -d64 -mx512m -jar ../dist/cooja.jar -nogui=/Users/beshr/work/contiki-private/examples/tsch-testbed/imgs/app-rpl-collect-only-min.csc

echo "Done!"
