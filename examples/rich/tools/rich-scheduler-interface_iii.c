#include "contiki.h"
#include "net/ip/uip.h"
#include "rich.h"
#include "rest-engine.h"
#include <stdio.h>
#include "net/ipv6/uip-ds6-route.h"
#include "net/rpl/rpl.h"

static void plexi_dag_event_handler(void);
static void plexi_get_dag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

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
		// TODO: get details per dag id
		rpl_instance_t *default_instance = rpl_get_instance(RPL_DEFAULT_INSTANCE);
		CONTENT_PRINTF("[[");
		if(default_instance != NULL && default_instance->current_dag != NULL && default_instance->of != NULL && default_instance->of->calculate_rank != NULL) {
			rpl_parent_t *p = nbr_table_head(rpl_parents);
			int nbr_lst_size = uip_ds6_nbr_num();
			int j = 0;
			if(p == NULL) {
				CONTENT_PRINTF("]");
			} else {
				while(p != NULL) {
					uip_ds6_nbr_t *nbr = rpl_get_nbr(p);
					CONTENT_PRINTF("\"2%x:%x%x%x:%x:%x%x\"",
						UIP_HTONS(nbr_table_get_lladdr(rpl_parents, p)->u8[1]), UIP_HTONS(nbr_table_get_lladdr(rpl_parents, p)->u8[2]),
						UIP_HTONS(nbr_table_get_lladdr(rpl_parents, p)->u8[3]), UIP_HTONS(nbr_table_get_lladdr(rpl_parents, p)->u8[4]),
						UIP_HTONS(nbr_table_get_lladdr(rpl_parents, p)->u8[5]), UIP_HTONS(nbr_table_get_lladdr(rpl_parents, p)->u8[6]),
						UIP_HTONS(nbr_table_get_lladdr(rpl_parents, p)->u8[7])
					);
					p = nbr_table_next(rpl_parents, p);
					if(p != NULL) {
						CONTENT_PRINTF(",");
					}
				}
				CONTENT_PRINTF("]");
			}
		}
		CONTENT_PRINTF(",[");
		
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
		CONTENT_PRINTF("]]");

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
	rest_activate_resource(&resource_rpl_dag,  "rpl/dag");
  
	PROCESS_END();
}

/*********** Init *************************************************/

void
rich_scheduler_interface_init()
{
  static struct uip_ds6_notification n;
  rest_init_engine();
  rest_activate_resource(&resource_rpl_dag, "rpl/dag");
  /* A callback for routing table changes */
  uip_ds6_notification_add(&n, route_changed_callback);
  //process_start(&ischeduler, NULL);
  printf("PLEXI: initializing scheduler interface\n");
}