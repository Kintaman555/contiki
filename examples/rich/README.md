# HOWTO - Setting up the RPL Border router, RPL bridge, and other RICH nodes

In this folder we have a fully functional demonstrator for RICH stack (CoAP + IPv6/RPL + TSCH on NXP) with the components:

1. **RPL border router**: to start the wireless network and connect it to other networks. 
2. **RPL bridge** which forwards the UDP traffic from/to the host PC to the wireless network so you can connect the smart good simulator to RICH, 
3. and a **wireless node** that does nothing except acting as a wireless forwarder. It is possible to implement other meaningful processes on this node to make an embedded smart controller/actuator that utilizes RICH stack.

These components can be compiled for either NXP JN5168 or Z1 platforms, and should work out-of-the-box.

## RPL Border Router

Setup the UART flow-control mode for the router from rpl-border-router/project-conf.h

* Enable either **HW flow control**
```C
#define UART_HW_FLOW_CTRL  1
#define UART_XONXOFF_FLOW_CTRL 0
```
* or **SW flow control**
```C
#define UART_HW_FLOW_CTRL  0
#define UART_XONXOFF_FLOW_CTRL 1
```
* You can disable both, but it is not recommended.

Compile and flash a node with the rpl-border-router.jn5168.bin image. Either a USB dongle or a dev-board would work.

From a Linux terminal, go to `contiki-private/examples/rich/rpl-border-router` and do either
`make connect-router-hw` if you have **HW flow control**
or `make connect-router-sw` if you have **SW flow control**

This will start a tunnel interface (tun0) on the host machine.
All traffic towards our network (prefix aaaa::1/64) will now be routed to the border router.

# 6top-like CoAP Resources

They are enabled by default in these examples. They implementation is under `tools\rich-scheduler-interface.c` 

* Install a CoAP plugin for your browser e.g., [Copper plugin]( https://addons.mozilla.org/en-US/firefox/addon/copper-270430/)
* Then go to
coap://[*node-ip-address*]:**5684**
  * Notice the custom coap port 5684
  
## RPL-Bridge
Similarily configure the flow control options, and compile and flash a node with the rpl-bridge.jn5168.bin image.

From contiki-private/examples/rich/rpl-bridge, do
`make connect-bridge-hw` or `make connect-router-sw` depending on the configured flow-control options.

This will start a tun interface on the host machine.
The dongle node will join the TSCH and RPL network and acquire a global IPv6 address.
The host computer will get this IPv6 address and assign it to the tun interface.

All traffic coming from the Internet towards the dongle's global IPv6 will be:

1. first routed through the RPL network all the way to the node running Contiki
2. if the packet is UDP with port not equal to 5684 (Conitki is using this port), the packet will be forwarded to the linux host.

All traffic from the host Linux machine will be routed to the Internet through the dongle

**Important note:** the Linux machine does not have a full Internet connectivity, as it won't receive anything but UDP messages with port other than 5684.
This means you can not ping, use other ICMP messages, or TCP form this machine.
Acting as a CoAP client and server is all the machine is expected to do.

## Node

The directory contiki-private/examples/rich/node contains a basic RICH node running TSCH, RPL, and the CoAP scheduler resources.
You can compile and program more NXP nodes to run this, forming a larger network.

## Quick demo setup

You need to flash and program a border-router and a bridge on two nodes:

1. plug the border-router node, then go to contiki-private/examples/rich/rpl-border-router and start the router process by typing: 
`make connect-router-sw`
this would start the wireless network with the IPv6 prefix: aaaa::
You need to keep this process running in a terminal all the time.
The IP address of this end would be ´aaaa::1´
2. on another pc (or Linux virtual machine) plug the rpl-bridge node, then go to contiki-private/examples/rich/rpl-bridge and start the bridge process by typing: 
`make connect-bridge-hw`
the node would connect to the router, request an IP address and start a "tunnel" network interface that has the automatic IP address it got. You don't need (and can not) provide a manual IP address. To see the assigned IP address, either look at the output of the connect command, or open another terminal and check the IP address on the interface tun0 by typing:
`ifconfig tun0`
When tun0 gets and IP address, it means it has connected successfully to the wireless network. 
You can start the smart-appliance simulator on the bridge end, and the scheduler on the border-router end.
