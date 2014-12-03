CONTIKI_PROJECT=rpl-bridge
all: $(CONTIKI_PROJECT)
 
TARGET ?= jn5168

APPS = json

CONTIKI=../../..

UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

#linker optimizations
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += slip-bridge.c slip.c

PROJECTDIRS += .. ../tools
PROJECT_SOURCEFILES += rich-scheduler-interface.c rich.c

CFLAGS += -DWITH_COAP=13
CFLAGS += -DREST=coap_rest_implementation
CFLAGS += -DUIP_CONF_TCP=0
APPS += er-coap-13
APPS += erbium

PORT ?= 60002

include $(CONTIKI)/Makefile.include

$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

#using XON/XOFF flow control
connect-bridge-sw:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -v1 -X -B 1000000 $(PREFIX)
	
#using hw flow control	
connect-bridge-hw:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -v1 -H -B 1000000 $(PREFIX)
	
#using no flow control	
connect-bridge-no:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -v1 -B 1000000 $(PREFIX)


connect-bridge-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 -p $(PORT) -t tun1
