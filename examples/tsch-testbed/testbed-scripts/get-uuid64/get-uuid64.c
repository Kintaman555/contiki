#include "contiki.h"
#include "net/rime.h"
#include "node-id.h"
#include "sys/etimer.h"
#include "net/netstack.h"

#include <stdio.h> /* For printf() */

/*---------------------------------------------------------------------------*/
PROCESS(test_testbed_process, "Test Testbed process");
AUTOSTART_PROCESSES(&test_testbed_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_testbed_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  printf("Starting process\n");
  
  while(1) {
    int i;
    etimer_set(&et, (random_rand() % (CLOCK_SECOND * 4)));
    PROCESS_WAIT_UNTIL(etimer_expired(&et));

    printf("Node id: %u, Rime address: ", node_id);
    for(i=0; i<7; i++) {
      printf("%02x:", linkaddr_node_addr.u8[i]);
    }
    printf("%02x\n", linkaddr_node_addr.u8[7]);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
