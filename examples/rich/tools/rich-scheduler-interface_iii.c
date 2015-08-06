/**
 *  \file RICH CoAP Scheduler Interface for Contiki 3.x
 *  
 *  \author Simon Duquennoy <simonduq@sics.se>
 *  		George Exarchakos <g.exarchakos@tue.nl>
 *  		Ilker Oztelcan <i.oztelcan@tue.nl>
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "contiki.h"
#include "net/rpl/rpl.h"
#include "rich.h"

#include "rest-engine.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ip/uip-debug.h"

#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-private.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/mac/tsch/tsch-queue.h"

#include "jsontree.h"
#include "jsonparse.h"

#include "er-coap-engine.h"


static void plexi_dag_event_handler(void);
static void plexi_nbrs_event_handler(void);
static void plexi_get_dag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_get_neighbors_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_get_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_post_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_delete_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_get_cells_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_delete_cells_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_put_post_cells_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

int jsonparse_find_field(struct jsonparse_state *js, char *field_buf, int field_buf_len);
static int na_to_linkaddr(const char *na_inbuf, int bufsize, linkaddr_t *linkaddress);

static char content[REST_MAX_CHUNK_SIZE];
static int content_len = 0;

#define DEBUG DEBUG_PRINT
#define SM_UPDATE_INTERVAL (60 * CLOCK_SECOND)
#define CONTENT_PRINTF(...) { if(content_len < sizeof(content)) content_len += snprintf(content+content_len, sizeof(content)-content_len, __VA_ARGS__); }
#define CLIP(value, level) if (value > level) { value = level; }

/*********** RICH scheduler resources ********************************************/

/*********************************************************************************/
/* Observable dodag resource and event handler to obtain RPL parent and children */ 
/*********************************************************************************/
EVENT_RESOURCE(resource_rpl_dag,								/* name */
               "obs;title=\"RPL DAG Parent and Children\"",		/* attributes */
               plexi_get_dag_handler,							/* GET handler */
               NULL,											/* POST handler */
               NULL,											/* PUT handler */
               NULL,											/* DELETE handler */
               plexi_dag_event_handler);						/* event handler */


/* Builds the JSON response to requests for the rpl/dag resource details.
 *	The response is a JSON object of two elements:
 *	{
 *		"parent": [IP addresses of preferred parent according to RPL],
 *		"child": [IP addresses of RPL children]
 *	}
*/
static void plexi_get_dag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) { /* make sure the request accepts JSON reply or does not specify the reply type */
		content_len = 0;
		CONTENT_PRINTF("{");
		// TODO: get details per dag id other than the default
		/* Ask RPL to provide the preferred parent or if not known (e.g. LBR) leave it empty */
		rpl_parent_t *parent = rpl_get_any_dag()->preferred_parent;
		if(parent != NULL) {
			uip_ipaddr_t *parent_addr = rpl_get_parent_ipaddr(parent); /* retrieve the IP address of the parent */
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
		/* Iterate over routing table and record all children */
		for (r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
			curr_next_hop = uip_ds6_route_nexthop(r); /* the first entry in the routing table is the parent, so it is skipped */
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

		REST.set_header_content_type(response, REST.type.APPLICATION_JSON); /* Build the header of the reply */
		REST.set_response_payload(response, (uint8_t *)content, content_len); /* Build the payload of the reply */
	}
}

/* Notifies all clients who observe changes to the rpl/dag resource i.e. to the RPL dodag connections */
static void plexi_dag_event_handler() {
	/* Registered observers are notified and will trigger the GET handler to create the response. */
	REST.notify_subscribers(&resource_rpl_dag);
}

/* Wait for 30s without activity before notifying subscribers */
static struct ctimer route_changed_timer;

/* Callback function to be called when a change to the rpl/dag resource has occurred.
 * Any change is delayed 30seconds before it is propagated to the observers.
*/
static void route_changed_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr, int num_routes) {
  /* We have added or removed a routing entry, notify subscribers */
	if(event == UIP_DS6_NOTIFICATION_ROUTE_ADD || event == UIP_DS6_NOTIFICATION_ROUTE_RM) {
		printf("PLEXI: setting route_changed callback with 30s delay\n");
		ctimer_set(&route_changed_timer, 30*CLOCK_SECOND, (void(*)(void *))plexi_dag_event_handler, NULL);
	}
}

/***********************************************************************************/
/* Observable neighbor list resource and event handler to obtain all neighbor data */ 
/***********************************************************************************/
EVENT_RESOURCE(resource_6top_nbrs,								/* name */
		"obs;title=\"6top neighbours\"",						/* attributes */
		plexi_get_neighbors_handler,							/* GET handler */
		NULL,													/* POST handler */
		NULL,													/* PUT handler */
		NULL,													/* DELETE handler */
		plexi_nbrs_event_handler);								/* event handler */

/* Builds the JSON response to requests for 6top/nbrs resource.
 * The response consists of an object with the neighbors and the time elapsed since the last confirmed interaction with that neighbor.
 * That is:
 * 	{
		"IP address of neigbor 1":time in miliseconds,
		...
		"IP address of neigbor 2":time in miliseconds
	}
*/
static void plexi_get_neighbors_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	/* make sure the request accepts JSON reply or does not specify the response type */
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		content_len = 0;
		CONTENT_PRINTF("{");
		uip_ds6_nbr_t *nbr;	
		clock_time_t now = clock_time();
		int first_item = 1;
		uip_ipaddr_t *last_next_hop = NULL;
		uip_ipaddr_t *curr_next_hop = NULL;
		/* iterate over all link-layer neighbours and calculate the time elapsed since last interaction */
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
				/* For every neighbor record the time since it was added to routing table or the time since a packet was sent to */
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

/* Notifies all clients who observe changes to the 6top/nbrs resource */
static void plexi_nbrs_event_handler() {
	REST.notify_subscribers(&resource_6top_nbrs);
}

/***********************************************************************************/
/*              Slotframe Resource to GET, POST or DELETE slotframes               */ 
/***********************************************************************************/

PARENT_RESOURCE(resource_6top_slotframe,		/* name */
		"obs;title=\"6top Slotframe\";",     	/* attributes */
		plexi_get_slotframe_handler,			/*GET handler*/
		plexi_post_slotframe_handler,			/*POST handler*/
		NULL,									/*PUT handler*/
		plexi_delete_slotframe_handler);		/*DELETE handler*/ 
 
static void plexi_get_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	content_len = 0;

	char uri_subresource[32];
	const char *uri_path = NULL;
	int uri_len = REST.get_url(request, &uri_path);
	int base_len = strlen(resource_6top_slotframe.url);
	
	int c = 0;
	int d = 0;  
  
	if (uri_len == base_len){
		/*An array is created for the slotframe handles, it is passed to the function retrieving them*/
		int handles_list[32];
		/*The function returns the size of the array as an integer.*/
		int size = tsch_schedule_get_slotframe_handles(handles_list);	
	
		CONTENT_PRINTF("[");
		for (c = 0; c<size; c++) {
			if (c == size - 1){
				CONTENT_PRINTF("%u", handles_list[c]);
			} else{
				CONTENT_PRINTF("%u,", handles_list[c]);
			}
		}
		CONTENT_PRINTF("]");
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		REST.set_response_payload(response, (uint8_t *)content, content_len);
	} else if (uri_len > base_len + 1 && uri_len - base_len - 1 < sizeof(uri_subresource)) { /* Check that there is a subresource, and that we have enough space to store it */
		char *end;
		/* Extract the subresource path, have it null-terminated */
		strlcpy(uri_subresource, uri_path + base_len + 1, uri_len - base_len);
		/* Convert subresource to unsigned */
		uint16_t slotframe_handle = (uint16_t)strtoul(uri_subresource, &end, 10);
		/* Check conversion */
		if (end != uri_subresource && *end == '\0' && errno != ERANGE) {
			struct tsch_slotframe *slotframe = tsch_schedule_get_slotframe_from_handle(slotframe_handle);
			if(slotframe != NULL) {
				CONTENT_PRINTF("[");
				struct tsch_link *l = list_head(slotframe->links_list);
				int i = 0;
				if(l != NULL) {
					if(i > 0){
						CONTENT_PRINTF(",");
					}
					CONTENT_PRINTF("%u", l->handle);
					i = 1;
					l=list_item_next(l);
				}
				CONTENT_PRINTF("]");
				REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
				REST.set_response_payload(response, (uint8_t *)content, content_len);
			}
		}
	}
}

static void plexi_post_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)  {
	content_len = 0;
	int state;
	int request_content_len;
	int first_item = 1;
	const uint8_t *request_content;
  
	char *content_ptr;
	char field_buf[32] = "";
	int new_sf_count = 0; /* The number of newly added slotframes */
	int ns = 0; /* number of slots */
	int fd = 0;
	/* Add new slotframe */
  
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
	
		request_content_len = REST.get_request_payload(request, &request_content);
	
		struct jsonparse_state js;
		jsonparse_setup(&js, (const char *)request_content, request_content_len);
	
		/* Start creating response */
		CONTENT_PRINTF("[");
	
		/* Parse json input */
		while((state=jsonparse_find_field(&js, field_buf, sizeof(field_buf)))) {
			switch(state) {
				case '{': /* New element */
					ns = 0;
					fd = 0;
					break;
				case '}': { /* End of current element */
					struct tsch_slotframe *slotframe = tsch_schedule_get_slotframe_from_handle(fd);
					if(!first_item) {
						CONTENT_PRINTF(",");
					}
					first_item = 0;
					if(slotframe || fd<0) {
						printf("PLEXI:! could not add slotframe %u with length %u\n", fd, ns);
						CONTENT_PRINTF("0");
					} else {
						if(tsch_schedule_add_slotframe(fd, ns)) {
							new_sf_count++;
							printf("PLEXI: added slotframe %u with length %u\n", fd, ns);
							CONTENT_PRINTF("1", fd);
						} else {
							CONTENT_PRINTF("0");
						}
					}
					break;
				}
				case JSON_TYPE_NUMBER: //Try to remove the if statement and change { to [ on line 601.
					if(!strncmp(field_buf, "fd", sizeof(field_buf))) {
						fd = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, "ns", sizeof(field_buf))) {
						ns = jsonparse_get_value_as_int(&js);
					} 
					break;
			}
		}
	
		/* Check if json parsing succeeded */
		if(js.error == JSON_ERROR_OK) {
			if(new_sf_count >= 1) {
				/* We have several slotframes, we need to send an array */
				CONTENT_PRINTF("]");
				content_ptr = content;
			} else {
				/* We added only one slotframe, don't send an array:
				* Start one byte farther and reduce content_len accordingly. */
				content_ptr = content + 1;
				content_len--;
			}
	
			REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
			REST.set_response_payload(response, (uint8_t *)content, content_len);	 
		}
	}
}

static void plexi_delete_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	content_len = 0;
	/* A buffer where the subresource part of the URI will be stored */
	char uri_subresource[32];
	const char *uri_path = NULL;
	int uri_len = REST.get_url(request, &uri_path);
	int base_len = strlen(resource_6top_slotframe.url);
	/* Check that there is a subresource, and that we have enough space to store it */
	if(uri_len > base_len + 1 && uri_len - base_len - 1 < sizeof(uri_subresource)) {
		char *end;
		/* Extract the subresource path, have it null-terminated */
		strlcpy(uri_subresource, uri_path+base_len+1, uri_len-base_len);
		/* Convert subresource to unsigned */
		unsigned fd = (unsigned)strtoul(uri_subresource, &end, 10);
		/* Check convertion */
		if(end != uri_subresource && *end == '\0' && errno != ERANGE) {
			/* Actually remove the slotframe */
			struct tsch_slotframe *sf = tsch_schedule_get_slotframe_from_handle(fd);
			if(sf && tsch_schedule_remove_slotframe(sf)) {
				printf("PLEXI: deleted slotframe %u\n", fd);
				CONTENT_PRINTF("[%u]", fd);
				REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
				REST.set_response_payload(response, (uint8_t *)content, content_len);
			}
		}
	}
}

/******************************************************/
/* Resource and handler to GET, POST and DELETE cells */ 
/******************************************************/
PARENT_RESOURCE(resource_6top_cells,		/* name */
		"obs;title=\"6top cells\"",			/* attributes */
		plexi_get_cells_handler,			/* GET handler */
		plexi_put_post_cells_handler,		/* POST handler */
		NULL,								/* PUT handler */
		plexi_delete_cells_handler);		/* DELETE handler */

static void plexi_get_cells_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	unsigned int accept = -1;
	
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		content_len = 0;
		char *end;
		char uri_subresource[32];
		const char *uri_path = NULL;
		const char *query = NULL;
		int uri_len = REST.get_url(request, &uri_path);
		int query_len = REST.get_query(request, &query);
		int base_len = strlen(resource_6top_cells.url);
		REST.get_header_accept(request, &accept);
		const char *query_value = NULL;
		unsigned long  id = -1;
		int query_value_len = REST.get_query_variable(request, "cellID", &query_value);
		if(!query_value) {
			query_value_len = REST.get_query_variable(request, "frameID", &query_value);
		}
		if(query_value) {
			id = (unsigned)strtoul(query_value, &end, 10);
		}
		if(uri_len > base_len + 1 && uri_len - base_len - 1 < sizeof(uri_subresource)) {
			strlcpy(uri_subresource, uri_path + base_len + 1, uri_len - base_len);
		}
		struct tsch_slotframe* slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(NULL);
		int item_counter = 0;
		int to_print = 0;
		CONTENT_PRINTF("[");
		while(slotframe) {
			struct tsch_link* link = (struct tsch_link*)tsch_schedule_get_next_link_of(slotframe, NULL);
			while(link) {
				to_print = 0;
				if(!query_value || (!strncmp("frameID",query,7) && link->slotframe_handle == id) || (!strncmp("cellID",query,6) && link->handle == id)) {
					if(item_counter > 0) {
						CONTENT_PRINTF(",");
					} else if(query_value && uri_len == base_len && !strncmp("cellID",query,6) && link->handle == id) {
						content_len = 0;
					}
					to_print = 1;
				}
				if(to_print) {
					item_counter++;
					if(!strcmp("cellID",uri_subresource)) {
						CONTENT_PRINTF("%u",link->handle);
					} else if(!strcmp("frameID",uri_subresource)) {
						CONTENT_PRINTF("%u",link->slotframe_handle);
					} else if(!strcmp("slotOffset",uri_subresource)) {
						CONTENT_PRINTF("%u",link->timeslot);
					} else if(!strcmp("channelOffset",uri_subresource)) {
						CONTENT_PRINTF("%u",link->channel_offset);
					} else if(!strcmp("linkOption",uri_subresource)) {
						CONTENT_PRINTF("%u",link->link_options);
					} else if(!strcmp("linkType",uri_subresource)) {
						CONTENT_PRINTF("%u",link->link_type);
					} else {
						CONTENT_PRINTF("{\"cd\":%u,\"fd\":%u,\"so\":%u,\"co\":%u,\"lo\":%u,\"lt\":%u}",link->handle, link->slotframe_handle,link->timeslot,link->channel_offset,link->link_options,link->link_type);
					}
				}
				link = (struct tsch_link*)tsch_schedule_get_next_link_of(slotframe, link);
			}
			slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(slotframe);
		}
		if(!query || uri_len != base_len || strncmp("cellID",query,6)) {
			CONTENT_PRINTF("]");
		}
		if(content_len>0) {
			REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
			REST.set_response_payload(response, (uint8_t *)content, content_len);
		}
	}
}

static void plexi_delete_cells_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	
	if(IS_OPTION((coap_packet_t *)request, COAP_OPTION_BLOCK1)) {
		return;
	}
	
	content_len = 0;

	/* A buffer where the subresource part of the URI will be stored */
    char uri_subresource[32];
    const char *uri_path = NULL;
    int uri_len = REST.get_url(request, &uri_path);
    int base_len = strlen(resource_6top_cells.url);
    /* Check that there is a subresource, and that we have enough space to store it */
    if(uri_len > base_len + 1 && uri_len - base_len - 1 < sizeof(uri_subresource)) {
		char *end;
		/* Extract the subresource path, have it null-terminated */
		strlcpy(uri_subresource, uri_path+base_len+1, uri_len-base_len);
		/* Convert subresource to unsigned */
		unsigned cd = (unsigned)strtoul(uri_subresource, &end, 10);
		/* Check convertion */
		if(end != uri_subresource && *end == '\0' && errno != ERANGE) {
			/* Actually remove the link */
			struct tsch_link *link = tsch_schedule_get_link_from_handle(cd);
			if(link) {
				struct tsch_slotframe *sf = tsch_schedule_get_slotframe_from_handle(link->slotframe_handle);
				if(sf && tsch_schedule_remove_link(sf, link)) {
					printf("RICH: deleted link %u\n", cd);
					CONTENT_PRINTF("[%u]", cd);
					REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
					REST.set_response_payload(response, (uint8_t *)content, content_len);
				}
			}
		}
	}
}

static void plexi_put_post_cells_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	int first_item = 1;
	const uint8_t *payload;
	if(IS_OPTION((coap_packet_t *)request, COAP_OPTION_BLOCK1)) {
		return;
	}
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		char *content_ptr;
		int state;
		int new_link_count = 0; /* The number of newly added links */

		int so = 0; /* slot offset */
		int co = 0; /* channel offset */
		int fd = 0; /* slotframeID (handle) */
		int lo = 0; /* link options */
		int lt = 0; /* link type */
		linkaddr_t na; /* node address */

		char field_buf[32] = "";
		char value_buf[32] = "";
		struct jsonparse_state js;
		
		int payload_length = REST.get_request_payload(request, &payload);
		jsonparse_setup(&js, (const char *)payload, payload_length);
		while((state=jsonparse_find_field(&js, field_buf, sizeof(field_buf)))) {
			switch(state) {
				case '{': /* New element */
					so = co = fd = lo = lt = 0;
					linkaddr_copy(&na, &linkaddr_null);
					break;
				case '}': { /* End of current element */
					struct tsch_slotframe *slotframe;
					struct tsch_link *link;
					slotframe = (struct tsch_slotframe*)tsch_schedule_get_slotframe_from_handle((uint16_t)fd);
					if(slotframe) {
						/* Add link */
						if((link = (struct tsch_link *)tsch_schedule_add_link(slotframe, (uint8_t)lo, lt, &na, (uint16_t)so, (uint16_t)co))) {
							new_link_count++;
							printf("PLEXI: added link %u, so %u, co %u, fd %u, lo %u, lt %u\n", link->handle, so, co, fd, lo, lt);
							printf("PLEXI: link address: ");
							PRINTLLADDR(&na);
							printf("\n");
							/* Update response */
							if(!first_item) {
								CONTENT_PRINTF(",");
							}
							first_item = 0;
							CONTENT_PRINTF("%u", link->handle);
						}
					} else {
						printf("PLEXI:! slotframe %u not found\n", fd);
					}
					break;
				}
				case JSON_TYPE_NUMBER:
					if(!strncmp(field_buf, "so", sizeof(field_buf))) {
						so = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, "co", sizeof(field_buf))) {
						co = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, "fd", sizeof(field_buf))) {
						fd = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, "lo", sizeof(field_buf))) {
						lo = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, "lt", sizeof(field_buf))) {
						lt = jsonparse_get_value_as_int(&js);
					}
					break;
				case JSON_TYPE_STRING:
					if(!strncmp(field_buf, "na", sizeof(field_buf))) {
						jsonparse_copy_value(&js, value_buf, sizeof(value_buf));
						na_to_linkaddr(value_buf, sizeof(value_buf), &na);
					}
					break;
			}
		}
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		REST.set_response_payload(response, (uint8_t *)content, content_len);
	}
}

void rich_scheduler_interface_init() {
  static struct uip_ds6_notification n;
  rest_init_engine();
  rest_activate_resource(&resource_rpl_dag, "rpl/dag");
  rest_activate_resource(&resource_6top_nbrs, "6top/nbrs");
  rest_activate_resource(&resource_6top_slotframe,  "6top/slotFrame");
  rest_activate_resource(&resource_6top_cells, "6top/cellList");
  /* A callback for routing table changes */
  uip_ds6_notification_add(&n, route_changed_callback);
  // TODO: add notification when ds6_neighbors change
  //process_start(&ischeduler, NULL);
  printf("PLEXI: initializing scheduler interface\n");
}

/* Utility function for json parsing */
int jsonparse_find_field(struct jsonparse_state *js, char *field_buf, int field_buf_len) {
	int state;
	while((state=jsonparse_next(js))) {
		switch(state) {
			case JSON_TYPE_PAIR_NAME:
				jsonparse_copy_value(js, field_buf, field_buf_len);
				/* Move to ":" */
				jsonparse_next(js);
				/* Move to value and return its type */
				return jsonparse_next(js);
			default:
				return state;
		}
	}
	return 0;
}
/* Utility function. Converts na field (string containing the lower 64bit of the IPv6) to
 * 64-bit MAC. */
static int na_to_linkaddr(const char *na_inbuf, int bufsize, linkaddr_t *linkaddress) {
	int i;
	char next_end_char = ':';
	const char *na_inbuf_end = na_inbuf + bufsize - 1;
	char *end;
	unsigned val;
	for(i=0; i<4; i++) {
		if(na_inbuf >= na_inbuf_end) {
			return 0;
		}
		if(i == 3) {
			next_end_char = '\0';
		}
		val = (unsigned)strtoul(na_inbuf, &end, 16);
		/* Check conversion */
		if(end != na_inbuf && *end == next_end_char && errno != ERANGE) {
			linkaddress->u8[2*i] = val >> 8;
			linkaddress->u8[2*i+1] = val;
			na_inbuf = end+1;
		} else {
			return 0;
		}
	}
	/* We consider only links with IEEE EUI-64 identifier */
	linkaddress->u8[0] ^= 0x02;
	return 1;
}