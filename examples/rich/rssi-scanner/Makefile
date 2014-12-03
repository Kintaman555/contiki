CONTIKI_PROJECT = rssi-scanner
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

TARGET ?= jn5168

#APPS = json

# Needed to compile with reduceded packetbuf attributes
WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

#PROJECTDIRS += .. ../tools
#PROJECT_SOURCEFILES += rich-scheduler-interface.c rich.c
#
#CFLAGS += -DWITH_COAP=13
#CFLAGS += -DREST=coap_rest_implementation
#CFLAGS += -DUIP_CONF_TCP=0
#APPS += er-coap-13
#APPS += erbium

all: $(CONTIKI_PROJECT)

CONTIKI=../../..

%.class: %.java
	javac $(basename $<).java

viewrssi: ViewRSSI.class
	make login $(filter-out $@,$(MAKECMDGOALS)) | java ViewRSSI
	
viewrssi3d: ViewRSSI3D.class
	make login $(filter-out $@,$(MAKECMDGOALS)) | java ViewRSSI3D
include $(CONTIKI)/Makefile.include
