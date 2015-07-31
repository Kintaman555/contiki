#include "contiki.h"
#include "net/ip/uip.h"
#include "rich.h"
#include "rest-engine.h"
#include <stdio.h>
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl.h"

static void plexi_dag_event_handler(void);
static void plexi_nbrs_event_handler(void);
static void plexi_get_dag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_get_neighbors_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

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
EVENT_RESOURCE(resource_rpl_dag,								/* name */
               "obs;title=\"RPL DAG Parent and Children\"",	/* attributes */
               plexi_get_dag_handler,							/* GET handler */
               NULL,											/* POST handler */
               NULL,											/* PUT handler */
               NULL,											/* DELETE handler */
               plexi_dag_event_handler);						/* event handler */

static void
plexi_get_dag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		content_len = 0;
		CONTENT_PRINTF("{");
		// TODO: get details per dag id other than the default
		
		rpl_parent_t *parent = rpl_get_any_dag()->preferred_parent;
		if(parent != NULL) {
			uip_ipaddr_t *parent_addr = rpl_get_parent_ipaddr(parent);
			CONTENT_PRINTF("\"parent\":[\"%x:%x:%x:%x\"]",
				UIP_HTONS(parent_addr->u16[4]), UIP_HTONS(parent_addr->u16[5]),
				UIP_HTONS(parent_addr->u16[6]), UIP_HTONS(parent_addr->u16[7])
			);
		} else {
			CONTENT_PRINTF("\"parent\":[]");
		}
		
		CONTENT_PRINTF(",\"child\":[");
		
		uip_ds6_route_t *r;
		int first_item = 1;
		uip_ipaddr_t *last_next_hop = NULL;
		uip_ipaddr_t *curr_next_hop = NULL;
		for (r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
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
		CONTENT_PRINTF("]}");

		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		REST.set_response_payload(response, (uint8_t *)content, content_len);
	}
}

static void
plexi_dag_event_handler()
{
	/* Registered observers are notified and will trigger the GET handler to create the response. */
	REST.notify_subscribers(&resource_rpl_dag);
}

/* Wait for 30s without activity before notifying subscribers */
static struct ctimer route_changed_timer;

static void
route_changed_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr, int num_routes)
{
  /* We have added or removed a routing entry, notify subscribers */
	if(event == UIP_DS6_NOTIFICATION_ROUTE_ADD || event == UIP_DS6_NOTIFICATION_ROUTE_RM) {
		printf("PLEXI: setting route_changed callback with 30s delay\n");
		ctimer_set(&route_changed_timer, 30*CLOCK_SECOND, (void(*)(void *))plexi_dag_event_handler, NULL);
	}
}

EVENT_RESOURCE(resource_6top_nbrs,
		"title=\"6top neighbours\"",
		plexi_get_neighbors_handler,
		NULL,
		NULL,
		NULL,
		plexi_nbrs_event_handler);

static void
plexi_get_neighbors_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		content_len = 0;
		CONTENT_PRINTF("{");
		uip_ds6_nbr_t *nbr;	
		clock_time_t now = clock_time();
		int first_item = 1;
		uip_ipaddr_t *last_next_hop = NULL;
		uip_ipaddr_t *curr_next_hop = NULL;
		for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL; nbr = nbr_table_next(ds6_neighbors, nbr)) {
			curr_next_hop = (uip_ipaddr_t *)uip_ds6_nbr_get_ipaddr(nbr);
			if(curr_next_hop != last_next_hop) {
				if (!first_item) {
					CONTENT_PRINTF(",");
				}
				first_item = 0;
				CONTENT_PRINTF("\"%x:%x:%x:%x\":",
								UIP_HTONS(curr_next_hop->u16[4]), UIP_HTONS(curr_next_hop->u16[5]),
								UIP_HTONS(curr_next_hop->u16[6]), UIP_HTONS(curr_next_hop->u16[7])
				);
				const uip_lladdr_t *nbr_lladdr = uip_ds6_nbr_get_ll((const uip_ds6_nbr_t *)nbr);
				if(nbr_lladdr != NULL) {
					rpl_parent_t *p = rpl_get_parent((uip_lladdr_t *)nbr_lladdr);
					if(p != NULL) {
						CONTENT_PRINTF("%u", (unsigned)(now - nbr->reachable.start)<(unsigned)(now - p->last_tx_time)?(unsigned)(now - nbr->reachable.start):(unsigned)(now - p->last_tx_time));
					} else {
						CONTENT_PRINTF("%u", (unsigned)(now - nbr->reachable.start));
					}
				} else {
					CONTENT_PRINTF("%u", (unsigned)(now - nbr->reachable.start));
				}
				last_next_hop = curr_next_hop;
			}
		}
		CONTENT_PRINTF("}");
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		REST.set_response_payload(response, (uint8_t *)content, content_len);
	}
}

static void
plexi_nbrs_event_handler() {
	REST.notify_subscribers(&resource_6top_nbrs);
}

void
rich_scheduler_interface_init()
{
  static struct uip_ds6_notification n;
  rest_init_engine();
  rest_activate_resource(&resource_rpl_dag, "rpl/dag");
  rest_activate_resource(&resource_6top_nbrs, "6top/nbrs");
  /* A callback for routing table changes */
  uip_ds6_notification_add(&n, route_changed_callback);
  // TODO: add notification when ds6_neighbors change
  //process_start(&ischeduler, NULL);
  printf("PLEXI: initializing scheduler interface\n");
}