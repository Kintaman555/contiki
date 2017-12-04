CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI=../../..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_WITH_IPV6 = 1
MAKE_WITH_ORCHESTRA ?= 0 # force Orchestra from command line
MAKE_WITH_SECURITY ?= 0 # force Security from command line

MAKE_WITH_ALMMAC ?= 0 # force ALMMAC from command line

APPS += orchestra
MODULES += core/net/mac/tsch

#APPS += almmac

ifeq ($(MAKE_WITH_ORCHESTRA),1)
CFLAGS += -DWITH_ORCHESTRA=1
endif

ifeq ($(MAKE_WITH_ALMMAC),1)
CFLAGS += -DWITH_ALMMAC=1
endif

ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

include $(CONTIKI)/Makefile.include
