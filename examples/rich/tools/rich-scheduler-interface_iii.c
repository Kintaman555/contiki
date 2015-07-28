#include "contiki.h"
#include "net/ip/uip.h"
#include "rich.h"
#include "rest-engine.h"
#include <stdio.h>
#include <LightingBoard.h>
#include <pca9634.h>
#include "net/ipv6/uip-ds6-route.h"

static void event_rpl_dodag_handler(void);
static void get_rpl_dodag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

static char content[REST_MAX_CHUNK_SIZE];
static int content_len = 0;

#define CONTENT_PRINTF(...) { if(content_len < sizeof(content)) content_len += snprintf(content+content_len, sizeof(content)-content_len, __VA_ARGS__); }

#define CLIP(value, level) if (value > level) { \
                              value = level;    \
                           }                
/*********** RICH scheduler resources ****************************************/

/***************************************************************************/
/* Observable dodag resource and event handler to obtain all neighbor data */ 
/***************************************************************************/
EVENT_RESOURCE(resource_rpl_dodag,								/* name */
               "obs;title=\"RPL DoDAG Parent and Children\"",	/* attributes */
               get_rpl_dodag_handler,							/* GET handler */
               NULL,											/* POST handler */
               NULL,											/* PUT handler */
               NULL,											/* DELETE handler */
               event_rpl_dodag_handler);						/* event handler */

static void
get_rpl_dodag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		content_len = 0;
		CONTENT_PRINTF("\"parents\",\"children\"");
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		REST.set_response_payload(response, (uint8_t *)content, content_len);
	}
}

static void
event_rpl_dodag_handler()
{
	/* Registered observers are notified and will trigger the GET handler to create the response. */
	REST.notify_subscribers(&resource_rpl_dodag);
}

/* Wait for 30s without activity before notifying subscribers */
static struct ctimer route_changed_timer;

static void
route_changed_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr, int num_routes)
{
  /* We have added or removed a routing entry, notify subscribers */
  if(event == UIP_DS6_NOTIFICATION_ROUTE_ADD
      || event == UIP_DS6_NOTIFICATION_ROUTE_RM) {
    printf("RICH: setting route_changed callback with 30s delay\n");
    ctimer_set(&route_changed_timer, 30*CLOCK_SECOND,
        event_rpl_dodag_handler, NULL);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ischeduler, ev, data)
{
	PROCESS_BEGIN();
	
	static struct uip_ds6_notification n; 										// Initialize a notification to trigger a callback when a RPL route changes
	static int is_coordinator = 0;
	//is_coordinator = node_id == 1;
  
	/* Start RICH stack */
	if(is_coordinator) {
		uip_ipaddr_t prefix;
		uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
		rich_init(&prefix);
	} else {
		rich_init(NULL);
	}

	printf("Starting RPL node\n");

	rest_init_engine();
	rest_activate_resource(&resource_rpl_dodag,  "rpl/dodag");
  
	PROCESS_END();
}

/*********** Init *************************************************/

void
rich_scheduler_interface_init()
{
  static struct uip_ds6_notification n;
  rest_init_engine();
  rest_activate_resource(&resource_rpl_dodag, "rpl/dodag");
  /* A callback for routing table changes */
  uip_ds6_notification_add(&n, route_changed_callback);
  //process_start(&ischeduler, NULL);
  printf("RICH: initializing scheduler interface\n");
}

/*
//create the response itself
void
dodaglist_create_response(coap_packet_t* response)
{
	content_len = 0;

	/*
	* first the parent and than the children list
	*
	

	int size;
	//array that holds pointers to the addresses in the list
	rimeaddr_t* neighboraddrs[32];
	//variables used later
	uip_ds6_route_t *r;
	int first_item = 1;
	uip_ipaddr_t *last_next_hop = NULL;
	uip_ipaddr_t *curr_next_hop = NULL;

	
	CONTENT_PRINTF("[");//dodag  begin

	//parent begin

	//get the list and size of list
	size = tsch_get_neighboraddrs(&neighboraddrs);
	/* 
	*  Parent is always in the third element of the list
	*  First two elements are always 0 and 255
	*  If there are only two elements than this node is the rpl-border-router as it has no parents
	*  otherwise return the third element
	*  Sometimes there is an random fourth element, this is probably due to being in the process of switching
	*  Or queue list (Dimitris is looking into this)
	*
	
	//because coap apparently fucks up the sending of single string, this is done in a one item list
	if (size == 2)
	{
		CONTENT_PRINTF("[\"Border-Router\"]");
	}
	else
	{
		//because the printf prints 00 as 0 add a zero here
		//this printing is in RiSCH ip format
		CONTENT_PRINTF("[\"aaaa::2%x:%x%x0:%x:%x%x\"]",	neighboraddrs[2]->u8[1], 
											neighboraddrs[2]->u8[2],
											neighboraddrs[2]->u8[4],
											neighboraddrs[2]->u8[5],
											neighboraddrs[2]->u8[6],
											neighboraddrs[2]->u8[7]);
	}

	CONTENT_PRINTF(",");//parent end

	CONTENT_PRINTF("[");//child begin
	for (r = uip_ds6_route_head();
		r != NULL;
		r = uip_ds6_route_next(r)) {

		/* We rely on the fact that routes are ordered by next hop.
		* Loop over all loops and print every next hop exactly once. *
		curr_next_hop = uip_ds6_route_nexthop(r);
		if (curr_next_hop != last_next_hop) {
			if (!first_item) {
				CONTENT_PRINTF(",");
			}
			first_item = 0;
			CONTENT_PRINTF("\"%x:%x:%x:%x\"",
				UIP_HTONS(curr_next_hop->u16[4]), UIP_HTONS(curr_next_hop->u16[5]),
				UIP_HTONS(curr_next_hop->u16[6]), UIP_HTONS(curr_next_hop->u16[7])
				);
			last_next_hop = curr_next_hop;
		}
	}
	CONTENT_PRINTF("]");//child end

	CONTENT_PRINTF("]");//dodag end

	coap_set_header_content_type(response, REST.type.APPLICATION_JSON);
	coap_set_payload(response, content, content_len);
}
*/