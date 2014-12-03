CONTIKI_PROJECT = node
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

TARGET ?= jn5168

APPS = json

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

PROJECTDIRS += .. ../tools
PROJECT_SOURCEFILES += rich-scheduler-interface.c rich.c

CFLAGS += -DWITH_COAP=13
CFLAGS += -DREST=coap_rest_implementation
CFLAGS += -DUIP_CONF_TCP=0
APPS += er-coap-13
APPS += erbium

all: $(CONTIKI_PROJECT)

CONTIKI=../../..
include $(CONTIKI)/Makefile.include
