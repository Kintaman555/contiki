/**
 *  \file RICH CoAP Scheduler Interface for Contiki 3.x
 *  
 *  \author George Exarchakos <g.exarchakos@tue.nl>
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

#include "plexi.h"

static void plexi_dag_event_handler(void);
static void plexi_nbrs_event_handler(void);
static void plexi_get_dag_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_get_neighbors_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_get_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_post_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_delete_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_get_links_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_delete_links_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void plexi_post_links_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

int jsonparse_find_field(struct jsonparse_state *js, char *field_buf, int field_buf_len);
static int na_to_linkaddr(const char *na_inbuf, int bufsize, linkaddr_t *linkaddress);
static int linkaddr_to_na(char* buf, linkaddr_t *addr);

#define MAX_DATA_LEN REST_MAX_CHUNK_SIZE
#define NO_LOCK 0
#define DAG_GET_LOCK 1
#define NEIGHBORS_GET_LOCK 2
#define SLOTFRAME_GET_LOCK 3
#define SLOTFRAME_DEL_LOCK 4
#define SLOTFRAME_POST_LOCK 5
#define LINK_GET_LOCK 6
#define LINK_DEL_LOCK 7
#define LINK_POST_LOCK 8

static char content[MAX_DATA_LEN];
static int content_len = 0;
static char inbox_msg[MAX_DATA_LEN];
static int inbox_msg_len = 0;
static int inbox_msg_lock = NO_LOCK;

#define DEBUG DEBUG_PRINT
#define SM_UPDATE_INTERVAL (60 * CLOCK_SECOND)
#define CONTENT_PRINTF(...) { \
	if(content_len < sizeof(content)) \
		content_len += snprintf(content+content_len, sizeof(content)-content_len, __VA_ARGS__); \
}
#define CLIP(value, level) if (value > level) { value = level; }

/*********** RICH scheduler resources *************************************************************/

/**************************************************************************************************/
/** Observable dodag resource and event handler to obtain RPL parent and children 				  */ 
/**************************************************************************************************/
EVENT_RESOURCE(resource_rpl_dag,								/* name */
               "obs;title=\"RPL DAG Parent and Children\"",		/* attributes */
               plexi_get_dag_handler,							/* GET handler */
               NULL,											/* POST handler */
               NULL,											/* PUT handler */
               NULL,											/* DELETE handler */
               plexi_dag_event_handler);						/* event handler */


/** Builds the JSON response to requests for the rpl/dag resource details.						   *
 *	The response is a JSON object of two elements:												   *
 *	{																							   *
 *		"parent": [IP addresses of preferred parent according to RPL],							   *
 *		"child": [IP addresses of RPL children]													   *
 *	}
*/
static void plexi_get_dag_handler(void *request,
									void *response,
									uint8_t *buffer, 
									uint16_t preferred_size, 
									int32_t *offset) {
	if(inbox_msg_lock != NO_LOCK && inbox_msg_lock != DAG_GET_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later.", 29);
		return;
	}
	inbox_msg_lock = NO_LOCK;
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	/* make sure the request accepts JSON reply or does not specify the reply type */
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		content_len = 0;
		CONTENT_PRINTF("{");
		// TODO: get details per dag id other than the default
		/* Ask RPL to provide the preferred parent or if not known (e.g. LBR) leave it empty */
		rpl_parent_t *parent = rpl_get_any_dag()->preferred_parent;
		if(parent != NULL) {
			/* retrieve the IP address of the parent */
			uip_ipaddr_t *parent_addr = rpl_get_parent_ipaddr(parent); 
			CONTENT_PRINTF("\"%s\":[\"%x:%x:%x:%x\"]", DAG_PARENT_LABEL,
				UIP_HTONS(parent_addr->u16[4]), UIP_HTONS(parent_addr->u16[5]),
				UIP_HTONS(parent_addr->u16[6]), UIP_HTONS(parent_addr->u16[7])
			);
		} else {
			CONTENT_PRINTF("\"%s\":[]", DAG_PARENT_LABEL);
		}
		
		CONTENT_PRINTF(",\"%s\":[", DAG_CHILD_LABEL);
		
		uip_ds6_route_t *r;
		int first_item = 1;
		uip_ipaddr_t *last_next_hop = NULL;
		uip_ipaddr_t *curr_next_hop = NULL;
		/* Iterate over routing table and record all children */
		for (r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
			/* the first entry in the routing table is the parent, so it is skipped */
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
		/* Build the header of the reply */
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON); 
		/* Build the payload of the reply */
		REST.set_response_payload(response, (uint8_t *)content, content_len);
	}
}

/* Notify all clients who observe changes to rpl/dag resource i.e. to the RPL dodag connections */
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

/**************************************************************************************************/
/** Observable neighbor list resource and event handler to obtain all neighbor data 			  */ 
/**************************************************************************************************/
EVENT_RESOURCE(resource_6top_nbrs,								/* name */
		"obs;title=\"6top neighbours\"",						/* attributes */
		plexi_get_neighbors_handler,							/* GET handler */
		NULL,													/* POST handler */
		NULL,													/* PUT handler */
		NULL,													/* DELETE handler */
		plexi_nbrs_event_handler);								/* event handler */

/** Builds the JSON response to requests for 6top/nbrs resource.								   *
 * The response consists of an object with the neighbors and the time elapsed since the last       *
 * confirmed interaction with that neighbor. That is:											   *
 * 	{																							   *
		"IP address of neigbor 1":time in miliseconds,											   *
		...																						   *
		"IP address of neigbor 2":time in miliseconds											   *
	}																							   *
*/
static void plexi_get_neighbors_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	if(inbox_msg_lock != NO_LOCK && inbox_msg_lock != NEIGHBORS_GET_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later.", 29);
		return;
	}
	inbox_msg_lock = NO_LOCK;
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

/**************************************************************************************************
 **              Slotframe Resource to GET, POST or DELETE slotframes               			  * 
 **************************************************************************************************/

PARENT_RESOURCE(resource_6top_slotframe,		/* name */
		"obs;title=\"6top Slotframe\";",     	/* attributes */
		plexi_get_slotframe_handler,			/*GET handler*/
		plexi_post_slotframe_handler,			/*POST handler*/
		NULL,									/*PUT handler*/
		plexi_delete_slotframe_handler);		/*DELETE handler*/ 
 
/** Builds the response to requests for 6top/slotFrame resource and its subresources. Each slotframe
 * is an object like:
 *	{
 *		"id":<id>,
 *		"slots":<num of slots in frame>
 *	}
 * The response to 6top/slotFrame is an array of the complete slotframe objects above.
 *	[
 *		{
 *			"id":<id>,
 *			"slots":<num of slots in frame>
 *		}
 *	]
 * Subresources are also possible:
 * 	The response to 6top/slotFrame/id is an array of slotframe ids
 * 	The response to 6top/slotFrame/slots is an array of slotframe sizes 
 * 
 * Queries are also possible.
 * Queries over 6top/slotFrame generate arrays of complete slotFrame objects.
 * For example:
 * 	The response to 6top/slotFrame?id=4 is the complete slotframe object with id=4.
 * 		Since id is the unique handle of a slotframe, the response will be just one slotframe object
 *		 (not an array).
 *		{
 *			"id":<id>,
 *			"slots":<num of slots in frame>
 *		}
 * 	The response to 6top/slotFrame?slots=4 is an array with the complete slotframe objects of size 4. That is:
 *	[
 *		{
 *			"id":<id>,
 *			"slots":<num of slots in frame>
 *		}
 *	]
 * 
 * Queries over subresources e.g. 6top/slotFrame/id generate arrays of the subresource.
 * For example:
 * 	The response to 6top/slotFrame/slots?id=4 is an array of the size of slotframe with id=4: e.g. [101]
 * 	The response to 6top/slotFrame/id?slots=101 is an array of the ids of slotframes with slots=101: e.g. [1,3,4]
 *	Obviously, response to 6top/slotFrame/id?id=4 will give: [4], and
 *	response to 6top/slotFrame/slots?slots=101 will give an array will all elements equal to 101 and as many as the number of slotframes with that size: e.g. [101,101,101]
 */ 
static void plexi_get_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	if(inbox_msg_lock != NO_LOCK && inbox_msg_lock != SLOTFRAME_GET_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later.", 29);
		return;
	}
	inbox_msg_lock = NO_LOCK;
	content_len = 0;
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		char *end;
		char *uri_path = NULL;
		const char *query = NULL;
		int uri_len = REST.get_url(request, (const char**)(&uri_path));
		*(uri_path+uri_len) = '\0';
		int base_len = strlen(resource_6top_slotframe.url);
		char *uri_subresource = uri_path+base_len;
		if(*uri_subresource == '/')
			uri_subresource++;
		int query_len = REST.get_query(request, &query);
		char *query_value = NULL;
		unsigned long value = -1;
		int query_value_len = REST.get_query_variable(request, FRAME_ID_LABEL, (const char**)(&query_value));
		if(!query_value) {
			query_value_len = REST.get_query_variable(request, FRAME_SLOTS_LABEL, (const char**)(&query_value));
		}
		if(query_value) {
			*(query_value+query_value_len) = '\0';
			value = (unsigned)strtoul((const char*)query_value, &end, 10);
		}
		if((uri_len > base_len + 1 && strcmp(FRAME_ID_LABEL,uri_subresource) && strcmp(FRAME_SLOTS_LABEL,uri_subresource)) || (query && !query_value)) {
			coap_set_status_code(response, NOT_IMPLEMENTED_5_01);
			coap_set_payload(response, "Supports only slot frame id XOR size as subresource or query", 60);
			return;
		}
		int item_counter = 0;
		CONTENT_PRINTF("[");
		struct tsch_slotframe* slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(NULL);
		while(slotframe) {
			if(!query_value || (!strncmp(FRAME_ID_LABEL,query,sizeof(FRAME_ID_LABEL)-1) && slotframe->handle == value) || \
				(!strncmp(FRAME_SLOTS_LABEL,query,sizeof(FRAME_SLOTS_LABEL)-1) && slotframe->size.val == value)) {
				if(item_counter > 0) {
					CONTENT_PRINTF(",");
				} else if(query_value && uri_len == base_len && !strncmp(FRAME_ID_LABEL,query,sizeof(FRAME_ID_LABEL)-1) && slotframe->handle == value) {
					content_len = 0;
				}
				item_counter++;
				if(!strcmp(FRAME_ID_LABEL,uri_subresource)) {
					CONTENT_PRINTF("%u",slotframe->handle);
				} else if(!strcmp(FRAME_SLOTS_LABEL,uri_subresource)) {
					CONTENT_PRINTF("%u",slotframe->size.val);
				} else {
					CONTENT_PRINTF("{\"%s\":%u,\"%s\":%u}", FRAME_ID_LABEL, slotframe->handle, FRAME_SLOTS_LABEL, slotframe->size.val);
				}
			}
			slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(slotframe);
		}
		if(!query || uri_len != base_len || strncmp(FRAME_ID_LABEL,query,sizeof(FRAME_ID_LABEL)-1)) {
			CONTENT_PRINTF("]");
		}
		if(item_counter>0) {
			REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
			REST.set_response_payload(response, (uint8_t *)content, content_len);
		} else {
			coap_set_status_code(response, NOT_FOUND_4_04);
			return;
		}
	} else {
		coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
		return;
	}
}

/**
 *  Installs a number of slotframes with the provided ids and amount of slots in the payload.
 *  
 *  Request to add new slotframes with a JSON payload of the form:
 *	[
 *		{
 *			"id":<id 1>,
 *			"slots":<num of slots in frame 1>
 *		},
 *		{
 *			"id":<id 2>,
 *			"slots":<num of slots in frame 2>
 *		},
		...
 *	]
 *  
 *  Response is an array of 0 and 1 indicating unsuccessful and successful creation of the slotframe.
 *  The location of the 0 and 1 in the array is the same as the location of their corresponding slotframes in the request.
 *  For example: if [0,1,...] is the response, the first slotframe in the request was not created whereas the second was created...
 *	
 *	A slotframe is not created if the provided id already exists or
 *	the schedule is not editable at that moment because it is being changed by another process.
 */
static void plexi_post_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)  {
	if(inbox_msg_lock != NO_LOCK && inbox_msg_lock != SLOTFRAME_POST_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later.", 29);
		return;
	}
	inbox_msg_lock = NO_LOCK;
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
					if(!strncmp(field_buf, FRAME_ID_LABEL, sizeof(field_buf))) {
						fd = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, FRAME_SLOTS_LABEL, sizeof(field_buf))) {
						ns = jsonparse_get_value_as_int(&js);
					} 
					break;
			}
		}
		CONTENT_PRINTF("]");
		/* Check if json parsing succeeded */
		if(js.error == JSON_ERROR_OK) {
			REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
			REST.set_response_payload(response, (uint8_t *)content, content_len);	 
		} else {
			coap_set_status_code(response, BAD_REQUEST_4_00);
			coap_set_payload(response, "Can only support JSON payload format", 36);
		}
	} else {
		coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
		return;
	}
}

/** Deletes an existing slotframe.
 *	DELETE request is on the resource 6top/slotFrame with a query on specific slotframe ID. Hence,
 *	DELETE 6top/slotFrame?id=2
 *
 *	Note: more generic queries are not supported. e.g. To delete all slotFrames of size 101 slots
 *	(i.e. 6top/slotFrame?slots=101) is not yet supported. To achieve the same combine:
 *		1. GET 6top/slotFrame/id?slots=101 returns an array of ids: [x,y,z]
 *		2. Loop over results of 1 with DELETE 6top/slotFrame?id=w, where w={x,y,z}
 */
static void plexi_delete_slotframe_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	if(inbox_msg_lock != NO_LOCK && inbox_msg_lock != SLOTFRAME_DEL_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later.", 29);
		return;
	}
	inbox_msg_lock = NO_LOCK;
	content_len = 0;
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);

	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		char *end;
		char *uri_path = NULL;
		int uri_len = REST.get_url(request, (const char**)(&uri_path));
		*(uri_path+uri_len) = '\0';
		int base_len = strlen(resource_6top_slotframe.url);
		if(uri_len > base_len + 1) {
			coap_set_status_code(response, NOT_IMPLEMENTED_5_01);
			coap_set_payload(response, "Subresources are not supported for DELETE method", 48);
			return;
		}
		const char *query = NULL;
		int query_len = REST.get_query(request, &query);
		char *query_value = NULL;
		int query_value_len = REST.get_query_variable(request, FRAME_ID_LABEL, (const char**)(&query_value));
		unsigned long value = -1;
		/* Check that there is a subresource, and that we have enough space to store it */
		if((uri_len == base_len || uri_len == base_len+1) && query && query_value) {
			*(query_value+query_value_len) = '\0';
			int id = (unsigned)strtoul((const char*)query_value, &end, 10);
			/* Actually remove the slotframe */
			struct tsch_slotframe *sf = tsch_schedule_get_slotframe_from_handle(id);
			if(sf && tsch_schedule_remove_slotframe(sf)) {
				int slots = sf->size.val;
				printf("PLEXI: deleted slotframe {%s:%u, %s:%u}\n", FRAME_ID_LABEL, id, FRAME_SLOTS_LABEL, slots);
				CONTENT_PRINTF("{\"%s\":%u, \"%s\":%u}", FRAME_ID_LABEL, id, FRAME_SLOTS_LABEL, slots);
				REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
				REST.set_response_payload(response, (uint8_t *)content, content_len);
			}
			coap_set_status_code(response, DELETED_2_02);
		} else if(!query) {
			// TODO: make sure it is idempotent
			struct tsch_slotframe* sf = NULL;
			short int first_item = 1;
			while((sf=(struct tsch_slotframe*)tsch_schedule_get_next_slotframe(NULL))) {
				if(first_item) {
					CONTENT_PRINTF("[");
					first_item = 0;
				} else {
					CONTENT_PRINTF(",");
				}
				int slots = sf->size.val;
				int id = sf->handle;
				if(sf && tsch_schedule_remove_slotframe(sf)) {
					printf("PLEXI: deleted slotframe {%s:%u, %s:%u}\n", FRAME_ID_LABEL, id, FRAME_SLOTS_LABEL, slots);
					CONTENT_PRINTF("{\"%s\":%u, \"%s\":%u}", FRAME_ID_LABEL, id, FRAME_SLOTS_LABEL, slots);
				}
			}
			if(!first_item)
				CONTENT_PRINTF("]");
			REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
			REST.set_response_payload(response, (uint8_t *)content, content_len);
			coap_set_status_code(response, DELETED_2_02);
		} else if(query) {
			coap_set_status_code(response, NOT_IMPLEMENTED_5_01);
			coap_set_payload(response, "Supports only slot frame id as query", 29);
			return;
		}
	} else {
		coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
		return;
	}
}


/**************************************************************************************************/
/** Resource and handler to GET, POST and DELETE links 											  */ 
/**************************************************************************************************/
PARENT_RESOURCE(resource_6top_links,		/* name */
		"obs;title=\"6top links\"",			/* attributes */
		plexi_get_links_handler,			/* GET handler */
		plexi_post_links_handler,			/* POST handler */
		NULL,								/* PUT handler */
		plexi_delete_links_handler);		/* DELETE handler */

/** Gets (details about) a set of links as specified by the query.
 *  A request can specify to get a subresource of a link.
 *  GET /6top/cellList would provide an array of all links (all frames) of the requested node in the following format:
 *   [
 *     {
 *		link:<link id>,
 *		frame:<slotframe id>,
 *		slot:<timeslot offset>,
 *		channel:<channel offset>,
 *		option:<link options>,
 *		type:<link type>,
 *	},
 *     . . .
 *     {
 *		link:<link id>,
 *		frame:<slotframe id>,
 *		slot:<timeslot offset>,
 *		channel:<channel offset>,
 *		option:<link options>,
 *		type:<link type>,
 *	}
 *    ]
 *
 *  GET /6top/cellList/frame would provide an array of the slotframe IDs of all links of the requested node in the following format:
 *   [0,0,0,0,0,2,2,2,2,3,3,3,3,3,3,3,. . .,6,6,6]
 *   Instead of "frame" subresource any other subresource may be used e.g. /6top/cellist/slot
 *
 *  GET /6top/cellList?link=0 would provide the dictionary with the complete details of the cell with handle =0.
 *     {
 *		link:0,
 *		frame:<slotframe id>,
 *		slot:<timeslot offset>,
 *		channel:<channel offset>,
 *		option:<link options>,
 *		type:<link type>,
 *	}
 * 
 * GET /6top/cellList/slot?link=0 will give a scalar with the slot of the link with handle =0.
 *     [2]
 */
static void plexi_get_links_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	if(inbox_msg_lock != NO_LOCK && inbox_msg_lock != LINK_GET_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later", 28);
		return;
	}
	inbox_msg_lock = NO_LOCK;
	content_len = 0;
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		char *end;

		char *uri_path = NULL;
		const char *query = NULL;
		int uri_len = REST.get_url(request, (const char**)(&uri_path));
		*(uri_path+uri_len) = '\0';
		int base_len = strlen(resource_6top_links.url);

		/* Parse the query options and support only the slotframe, the slotoffset and channeloffset queries */
		int query_len = REST.get_query(request, &query);
		char *query_frame = NULL, *query_slot = NULL, *query_channel = NULL;
		unsigned long frame = -1, slot = -1, channel = -1;
		short int flag = 0;
		int query_frame_len = REST.get_query_variable(request, FRAME_ID_LABEL, (const char**)(&query_frame));
		int query_slot_len = REST.get_query_variable(request, LINK_SLOT_LABEL, (const char**)(&query_slot));
		int query_channel_len = REST.get_query_variable(request, LINK_CHANNEL_LABEL, (const char**)(&query_channel));
		if(query_frame) {
			*(query_frame+query_frame_len) = '\0';
			frame = (unsigned)strtoul(query_frame, &end, 10);
			flag|=4;
		}
		if(query_slot) {
			*(query_slot+query_slot_len) = '\0';
			slot = (unsigned)strtoul(query_slot, &end, 10);
			flag|=2;
		}
		if(query_channel) {
			*(query_channel+query_channel_len) = '\0';
			channel = (unsigned)strtoul(query_channel, &end, 10);
			flag|=1;
		}
		if(query_len > 0 && (!flag || (query_frame && frame < 0) || (query_slot && slot < 0) || (query_channel && channel < 0))) {
			coap_set_status_code(response, NOT_IMPLEMENTED_5_01);
			coap_set_payload(response, "Supports queries only on slot frame id and/or slotoffset and channeloffset", 74);
			return;
		}

		/* Parse subresources and make sure you can filter the results */
		char *uri_subresource = uri_path+base_len;
		if(*uri_subresource == '/')
			uri_subresource++;
		if((uri_len > base_len + 1 && strcmp(LINK_ID_LABEL,uri_subresource) && strcmp(FRAME_ID_LABEL,uri_subresource) \
			 && strcmp(LINK_SLOT_LABEL,uri_subresource) && strcmp(LINK_CHANNEL_LABEL,uri_subresource) \
			 && strcmp(LINK_OPTION_LABEL,uri_subresource) && strcmp(LINK_TYPE_LABEL,uri_subresource) \
			  && strcmp(LINK_TARGET_LABEL,uri_subresource))) {
			coap_set_status_code(response, NOT_FOUND_4_04);
			coap_set_payload(response, "Invalid subresource", 19);
			return;
		}
		struct tsch_slotframe* slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(NULL);
		int first_item = 1;
		while(slotframe) {
			if(!(flag&4) || frame == slotframe->handle) {
				struct tsch_link* link = (struct tsch_link*)tsch_schedule_get_next_link_of(slotframe, NULL);
				while(link) {
					if((!(flag&2) && !(flag&1)) || ((flag&2) && !(flag&1) && slot == link->timeslot) || (!(flag&2) && (flag&1) && channel == link->channel_offset) || ((flag&2) && (flag&1) && slot == link->timeslot && channel == link->channel_offset)){
						if(first_item) {
							if(flag < 7 || uri_len > base_len + 1)
								CONTENT_PRINTF("[");
							first_item = 0;
						} else {
							CONTENT_PRINTF(",");
						}
						if(!strcmp(LINK_ID_LABEL,uri_subresource)) {
							CONTENT_PRINTF("%u",link->handle);
						} else if(!strcmp(FRAME_ID_LABEL,uri_subresource)) {
							CONTENT_PRINTF("%u",link->slotframe_handle);
						} else if(!strcmp(LINK_SLOT_LABEL,uri_subresource)) {
							CONTENT_PRINTF("%u",link->timeslot);
						} else if(!strcmp(LINK_CHANNEL_LABEL,uri_subresource)) {
							CONTENT_PRINTF("%u",link->channel_offset);
						} else if(!strcmp(LINK_OPTION_LABEL,uri_subresource)) {
							CONTENT_PRINTF("%u",link->link_options);
						} else if(!strcmp(LINK_TYPE_LABEL,uri_subresource)) {
							CONTENT_PRINTF("%u",link->link_type);
						} else if(!strcmp(LINK_TARGET_LABEL,uri_subresource)) {
							if(!linkaddr_cmp(&link->addr, &linkaddr_null)) {
								char na[32];
								linkaddr_to_na(na, &link->addr);
								CONTENT_PRINTF("\"%s\"",na);
							} else {
								coap_set_status_code(response, NOT_FOUND_4_04);
								coap_set_payload(response, "Link has no target node address.", 32);
								return;
							}
						} else {
							CONTENT_PRINTF("{\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u",\
								LINK_ID_LABEL, link->handle, FRAME_ID_LABEL, link->slotframe_handle, \
								LINK_SLOT_LABEL, link->timeslot, LINK_CHANNEL_LABEL, link->channel_offset,\
								LINK_OPTION_LABEL, link->link_options, LINK_TYPE_LABEL, link->link_type);
							if(!linkaddr_cmp(&link->addr, &linkaddr_null)) {
								char na[32];
								linkaddr_to_na(na, &link->addr);
								CONTENT_PRINTF(",\"%s\":\"%s\"",LINK_TARGET_LABEL,na);
							}
							CONTENT_PRINTF("}");
						}
					}
					link = (struct tsch_link*)tsch_schedule_get_next_link_of(slotframe, link);
				}
			}
			slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(slotframe);
		}
		if(flag < 7 || uri_len > base_len + 1)
			CONTENT_PRINTF("]");
		if(!first_item) {
			REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
			REST.set_response_payload(response, (uint8_t *)content, content_len);
		} else {
			coap_set_status_code(response, NOT_FOUND_4_04);
			return;
		}
	} else {
		coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
		return;
	}
}

/** Deletes an existing link.
 *	DELETE request is on the resource 6top/cellList with a query on specific link linkID. Hence,
 *	DELETE 6top/cellList?linkID=2
 *	The response to this request is the complete link object as:
 *	{
 *		cd:<link id>,
 *		fd:<slotframe id>,
 *		so:<timeslot offset>,
 *		co:<channel offset>,
 *		lo:<link options>,
 *		lt:<link type>,
 *	}
 *
 *	Note: more generic queries are not supported. e.g. To delete all slotFrames of size 101 slots
 *	(i.e. 6top/slotFrame?slots=101) is not yet supported. To achieve the same combine:
 *		1. GET 6top/slotFrame/id?slots=101 returns an array of ids: [x,y,z]
 *		2. Loop over results of 1 with DELETE 6top/slotFrame?id=w, where w={x,y,z}
 */
static void plexi_delete_links_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	if(inbox_msg_lock != NO_LOCK && inbox_msg_lock != LINK_DEL_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later", 28);
		return;
	}
	inbox_msg_lock = NO_LOCK;
	content_len = 0;
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);

	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		char *end;

		char *uri_path = NULL;
		const char *query = NULL;
		int uri_len = REST.get_url(request, (const char**)(&uri_path));
		*(uri_path+uri_len) = '\0';
		int base_len = strlen(resource_6top_links.url);

		/* Parse the query options and support only the slotframe, the slotoffset and channeloffset queries */
		int query_len = REST.get_query(request, &query);
		char *query_frame = NULL, *query_slot = NULL, *query_channel = NULL;
		unsigned long frame = -1, slot = -1, channel = -1;
		short int flag = 0;
		int query_frame_len = REST.get_query_variable(request, FRAME_ID_LABEL, (const char**)(&query_frame));
		int query_slot_len = REST.get_query_variable(request, LINK_SLOT_LABEL, (const char**)(&query_slot));
		int query_channel_len = REST.get_query_variable(request, LINK_CHANNEL_LABEL, (const char**)(&query_channel));
		if(query_frame) {
			*(query_frame+query_frame_len) = '\0';
			frame = (unsigned)strtoul(query_frame, &end, 10);
			flag|=4;
		}
		if(query_slot) {
			*(query_slot+query_slot_len) = '\0';
			slot = (unsigned)strtoul(query_slot, &end, 10);
			flag|=2;
		}
		if(query_channel) {
			*(query_channel+query_channel_len) = '\0';
			channel = (unsigned)strtoul(query_channel, &end, 10);
			flag|=1;
		}
		if(query_len > 0 && (!flag || (query_frame && frame < 0) || (query_slot && slot < 0) || (query_channel && channel < 0))) {
			coap_set_status_code(response, NOT_IMPLEMENTED_5_01);
			coap_set_payload(response, "Supports queries only on slot frame id and/or slotoffset and channeloffset", 74);
			return;
		}

		/* Parse subresources and make sure you can filter the results */
		if(uri_len > base_len + 1) {
			coap_set_status_code(response, NOT_IMPLEMENTED_5_01);
			coap_set_payload(response, "Subresources are not supported for DELETE method", 48);
			return;
		}

		struct tsch_slotframe* slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(NULL);
		int first_item = 1;
		while(slotframe) {
			if(!(flag&4) || frame == slotframe->handle) {
				struct tsch_link* link = (struct tsch_link*)tsch_schedule_get_next_link_of(slotframe, NULL);
				while(link) {
					struct tsch_link* next_link = (struct tsch_link*)tsch_schedule_get_next_link_of(slotframe, link);
					int link_handle = link->handle;
					int link_slotframe_handle = link->slotframe_handle;
					int link_timeslot = link->timeslot;
					int link_channel_offset = link->channel_offset;
					int link_link_options = link->link_options;
					int link_link_type = link->link_type;
					char na[32];
					*na = '\0';
					if(!linkaddr_cmp(&link->addr, &linkaddr_null)) linkaddr_to_na(na, &link->addr);

					int deleted = tsch_schedule_remove_link(slotframe,link);
					if(((!(flag&2) && !(flag&1)) || ((flag&2) && !(flag&1) && slot == link_timeslot) || (!(flag&2) && (flag&1) && channel == link_channel_offset) || ((flag&2) && (flag&1) && slot == link_timeslot && channel == link_channel_offset)) && deleted) {
						if(first_item) {
							if(flag < 7)
								CONTENT_PRINTF("[");
							first_item = 0;
						} else {
							CONTENT_PRINTF(",");
						}
						printf("PLEXI: deleted link {\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u}", \
							LINK_ID_LABEL, link_handle, FRAME_ID_LABEL, link_slotframe_handle, LINK_SLOT_LABEL, link_timeslot, \
							LINK_CHANNEL_LABEL, link_channel_offset, LINK_OPTION_LABEL, link_link_options, LINK_TYPE_LABEL, link_link_type);
						CONTENT_PRINTF("{\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u",\
							LINK_ID_LABEL, link_handle, FRAME_ID_LABEL, link_slotframe_handle, \
							LINK_SLOT_LABEL, link_timeslot, LINK_CHANNEL_LABEL, link_channel_offset,\
							LINK_OPTION_LABEL, link_link_options, LINK_TYPE_LABEL, link_link_type);
						if(strlen(na) > 0) CONTENT_PRINTF(",\"%s\":\"%s\"",LINK_TARGET_LABEL,na);
						CONTENT_PRINTF("}");
					}
					link = next_link;
				}
			}
			slotframe = (struct tsch_slotframe*)tsch_schedule_get_next_slotframe(slotframe);
		}
		if(flag < 7)
			CONTENT_PRINTF("]");
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		if(flag != 7 || !first_item)
			REST.set_response_payload(response, (uint8_t *)content, content_len);
		coap_set_status_code(response, DELETED_2_02);
	} else {
		coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
		return;
	}
}

static void plexi_post_links_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
	if(inbox_msg_lock == NO_LOCK) {
		inbox_msg_len = 0;
		*inbox_msg='\0';
	} else if(inbox_msg_lock != LINK_POST_LOCK) {
		coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
		coap_set_payload(response, "Server too busy. Retry later.", 29);
		return;
	}
	content_len = 0;
	int first_item = 1;	
	unsigned int accept = -1;
	REST.get_header_accept(request, &accept);
	
	if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
		const uint8_t *request_content;
		int request_content_len;
		request_content_len = coap_get_payload(request, &request_content);
		if(inbox_msg_len+request_content_len>MAX_DATA_LEN) {
			coap_set_status_code(response, NOT_IMPLEMENTED_5_01);
			coap_set_payload(response, "Server reached internal buffer limit. Shorten payload.", 54);
			return;
		}
		if(coap_block1_handler(request, response, inbox_msg, &inbox_msg_len, MAX_DATA_LEN)) {
			inbox_msg_lock = LINK_POST_LOCK;
			return;
		}
		// TODO: It is assumed that the node processes the post request fast enough to return the
		//       response within the window assumed by client before retransmitting
		inbox_msg_lock = NO_LOCK;
		int i=0;
		for(i=0; i<inbox_msg_len; i++) {
			if(inbox_msg[i]=='[') {
				coap_set_status_code(response, BAD_REQUEST_4_00);
				coap_set_payload(response, "Array of links is not supported yet. POST each link separately.", 63);
				return;
			}
		}
		int state;

		int so = 0; 	//* slot offset *
		int co = 0; 	//* channel offset *
		int fd = 0; 	//* slotframeID (handle) *
		int lo = 0; 	//* link options *
		int lt = 0; 	//* link type *
		linkaddr_t na; 	//* node address *

		char field_buf[32] = "";
		char value_buf[32] = "";
		struct jsonparse_state js;
		jsonparse_setup(&js, (const char *)inbox_msg, inbox_msg_len);
		while((state=jsonparse_find_field(&js, field_buf, sizeof(field_buf)))) {
			switch(state) {
				case '{': //* New element *
					so = co = fd = lo = lt = 0;
					linkaddr_copy(&na, &linkaddr_null);
					break;
				case '}': { //* End of current element *
					struct tsch_slotframe *slotframe;
					struct tsch_link *link;
					slotframe = (struct tsch_slotframe*)tsch_schedule_get_slotframe_from_handle((uint16_t)fd);
					if(slotframe) {
						if((link = (struct tsch_link *)tsch_schedule_add_link(slotframe, (uint8_t)lo, lt, &na, (uint16_t)so, (uint16_t)co))) {
							char buf[32];
							linkaddr_to_na(buf, &na);
							printf("PLEXI: added {\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u,\"%s\":%u", \
								LINK_ID_LABEL, link->handle, FRAME_ID_LABEL, fd, LINK_SLOT_LABEL, so, \
								LINK_CHANNEL_LABEL, co, LINK_OPTION_LABEL, lo, LINK_TYPE_LABEL, lt);
							if(!linkaddr_cmp(&na, &linkaddr_null)) {
								char buf[32];
								linkaddr_to_na(buf, &na);
								printf(",\"%s\":\"%s\"",LINK_TARGET_LABEL,buf);
							}
							printf("}\n");
							//* Update response *
							if(!first_item) {
								CONTENT_PRINTF(",");
							} else {
								CONTENT_PRINTF("[");
							}
							first_item = 0;
							CONTENT_PRINTF("%u", link->handle);
						} else {
							coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
							coap_set_payload(response, "Link could not be added", 23);
							return;
						}
					} else {
						coap_set_status_code(response, NOT_FOUND_4_04);
						coap_set_payload(response, "Slotframe handle not found", 26);
						return;
					}
					break;
				}
				case JSON_TYPE_NUMBER:
					if(!strncmp(field_buf, LINK_SLOT_LABEL, sizeof(field_buf))) {
						so = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, LINK_CHANNEL_LABEL, sizeof(field_buf))) {
						co = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, FRAME_ID_LABEL, sizeof(field_buf))) {
						fd = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, LINK_OPTION_LABEL, sizeof(field_buf))) {
						lo = jsonparse_get_value_as_int(&js);
					} else if(!strncmp(field_buf, LINK_TYPE_LABEL, sizeof(field_buf))) {
						lt = jsonparse_get_value_as_int(&js);
					}
					break;
				case JSON_TYPE_STRING:
					if(!strncmp(field_buf, LINK_TARGET_LABEL, sizeof(field_buf))) {
						jsonparse_copy_value(&js, value_buf, sizeof(value_buf));
						int x = na_to_linkaddr(value_buf, sizeof(value_buf), &na);
						if(!x) {
							coap_set_status_code(response, BAD_REQUEST_4_00);
							coap_set_payload(response, "Invalid target node address", 27);
							return;
						}
					}
					break;
			}
		}
		CONTENT_PRINTF("]");
		
		REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
		REST.set_response_payload(response, (uint8_t *)content, content_len);
	}
}

void rich_scheduler_interface_init() {
  static struct uip_ds6_notification n;
  rest_init_engine();
  rest_activate_resource(&resource_rpl_dag, DAG_RESOURCE);
  rest_activate_resource(&resource_6top_nbrs, NEIGHBORS_RESOURCE);
  rest_activate_resource(&resource_6top_slotframe,  FRAME_RESOURCE);
  rest_activate_resource(&resource_6top_links, LINK_RESOURCE);
  /* A callback for routing table changes */
  uip_ds6_notification_add(&n, route_changed_callback);
  // TODO: add notification when ds6_neighbors change
  //process_start(&ischeduler, NULL);
  printf("\n*** PLEXI: initializing scheduler interface ***\n");
}

/* Utility function for json parsing */
int jsonparse_find_field(struct jsonparse_state *js, char *field_buf, int field_buf_len) {
	int state=jsonparse_next(js);
	while(state) {
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
		state=jsonparse_next(js);
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

static int linkaddr_to_na(char *buf, linkaddr_t *addr) {
	char *pointer = buf;
	unsigned int i;
	for(i = 0; i < sizeof(linkaddr_t); i++) {
		if(i > 1 && i!=3 && i!=4 && i!=7) {
			*pointer = ':';
			pointer++;
		}
		if(i==4){
			continue;
		}
		if(i==0) {
			sprintf(pointer, "%x", addr->u8[i]^0x02);
			pointer++;
		} else {
			sprintf(pointer, "%02x", addr->u8[i]);
			pointer+=2;
		}
	}
	sprintf(pointer, "\0");
	return strlen(buf);
}
