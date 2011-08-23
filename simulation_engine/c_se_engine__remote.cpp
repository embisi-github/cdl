/*a Copyright
  
  This file 'c_se_engine__submodule.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Documentation

CDL remote sim
Add a bit to eh_batch_mode 
Add a remote submodule server method set to the engine
* Needs to know clock, input, output mapping to bits in a packet
* Needs to know what module to instantiate with arguments/options
* Needs to know what clients to permit (optional)
Add ability to poll that submodule server for 'n' seconds

Add a new 'remoting' veneer for a submodule
* Permits arbitrary module type and instance replacement
* Needs to know what clocks, inputs and outputs it is 'faking'
* Needs to know IP address and port to connect to
* On instantiation claim the server
* On deletion release the server
* On preclock, record clocks that need to be called, and capture inputs
* On clock issue 'clock' packet with input state
* wait for response packet (with timeout)
* Drive outputs with values from response packet

remoting module needs to know:
clocks
inputs
outputs
order of bits for each in remote input/output packet

Have a poll routine which waits for a UDP message

remote_submodule_server_open_socket()
int mysock;
struct sockaddr_in myaddr;
  mysock = socket(PF_INET,SOCK_DGRAM,0);
 myaddr.sin_family = AF_INET;
 myaddr.sin_port = htons( 1234 );
 myaddr.sin_addr = htonl( INADDR_ANY );
  bind(mysock, &myaddr, sizeof(myaddr));


Send out response
 sendto( int sockfd,
    void *buff,
    size_t nbytes,
    int flags,
      const struct sockaddr* to,
      socklen_t addrlen);

Poll for response packet
remote_submodule_server__poll( remote_socket, timeout )

FD_ZERO(&readSet); 
FD_SET(fd, &readSet); 

FD_ZERO(&errSet); 
FD_SET(fd, &errSet); 

ret = select(fd + 1, &readSet, NULL, &errSet, NULL); 
if (ret < 0) 
printf("select returned ret=%d, errno=%d\n", ret, errno); 

if ( FD_ISSET(fd, &readSet) ) 
printf("readSet fired\n"); 
if ( FD_ISSET(fd, &errSet) ) 
printf("errSet fired\n"); 


Handle receive packet
remote_handle_packet()
ssize_t recvfrom( int sockfd,
    void *buff,
    size_t nbytes,
    int flags,
      struct sockaddr* from,
      socklen_t *fromaddrlen);
IP UDP message of 'claim' with 8 bytes of message - response of 'already claimed' with current claimant 'message' or 'success' (success indicates setting of claimant ARP entry and state of 'claimed')
IP UDP message of 'release' - response of 'released' state <= unclaimed or 'wrong claimant' if IP address or port mismatch
IP UDP message of 'force release' - response of 'released' - state <= unclaimed
IP UDP message of 'set inputs' with sequence number - response of 'inputs set' or 'bad sequence number' (sets sequence number if okay - bad if new sequence number != old sequence number+1)
IP UDP message of 'get sequence number' to get claimant sequence number
IP UDP message of 'clock tick' with clock mask - response of 'output values' or 'bad sequence number'
IP UDP message of 'get output values' - response of 'output values' or 'bad sequence number'
All IP UDP messages must be to well known port TBD on our IP address, unicast MAC address
All 'valid' IP UDP messages except claim and force release must be from the claimant's IP address and port
All responses for valid messages go to the claimant's MAC, IP and port
All claim responses and release responses go to the requester's MAC, IP and port

 */
/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_exec_file.h"
#include "sl_work_list.h"
#include "se_engine_function.h"
#include "se_errors.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Types
 */
/*t t_udp_packet
 */
typedef struct
{
    int length;
    t_sl_uint64 address;
    union
    {
        char *buffer[8*1024];
        t_udp_claim_packet claim;
    } data;
} t_udp_packet;

/*a Remote server
 */
/*f remote_submodule_server_create
 */
t_se_remote_submodule_server_ptr c_engine::remote_submodule_server_create( t_se_remote_server_address *address )
{
    t_se_remote_submodule_server_ptr rss = (t_se_remote_submodule_server_ptr)malloc(sizeof(t_se_remote_submodule_server));

    struct sockaddr_in socket_address;

    rss->skt = socket(PF_INET,SOCK_DGRAM,0);
    socket_address.sin_family = AF_INET;
    if (address->ip_address!=0)
    {
        socket_address.sin_addr = htonl( address->ip_address );
    }
    else
    {
        socket_address.sin_addr = htonl( INADDR_ANY );
    }
    socket_address.sin_port = htons( address->port );
    bind( rss->skt, &socket_address, sizeof(socket_address) );

    rss->claimed = 0;
    rss->claimant = 0;
    rss->sequence_number = 0;
    return rss;
}

/*f remote_submodule_server_delete
 */
void c_engine::remote_submodule_server_delete( t_se_remote_submodule_server_ptr rss )
{
    //Delete module if instantiated
    if (rss->skt<=0)
        close(rss->skt);
    free( rss );
}

/*f remote_submodule_server_instantiate_module
 */
t_sl_error_level c_engine::remote_submodule_server_instantiate_module( t_se_remote_submodule_server_ptr rss, const char *type, const char *name, t_sl_option_list option_list )
{
    t_sl_error_level rc;
    rc = instantiate( NULL, type, name, option_list );
    if (rc!=error_level_okay)
        return rc;

    rss->emi = (t_engine_module_instance *)find_module_instance( name );
    // Barf if we have a comb function
    // We need an array for inputs, and array of outputs, an array of some form for clocks, and we should reset the module
}

/*f remote_submodule_server_poll_socket
  timeout is in us
  Poll socket for a time - return -1 for error, 0 for timeout, 1 for packet ready for receive
 */
int c_engine::remote_submodule_server_poll_socket( t_se_remote_submodule_server_ptr rss, int timeout )
{
    fd_set read_fdset;
    fd_set error_fdset;

    FD_ZERO(&read_fdset); 
    FD_ZERO(&error_fdset); 
    FD_SET(rss->skt &read_fdset); 
    FD_SET(rss->skt, &error_fdset); 
    if (timeout>=0)
    {
        struct timeval timeout_tv;
        timeout_tv.sec = 0;
        timeout_tv.usec = timeout;
        rc = select( rss->skt + 1, &read_fdset, NULL, &error_fdset, &timeout_tv );
    }
    else
    {
        rc = select( rss->skt + 1, &read_fdset, NULL, &error_fdset, NULL );
    }
    if (rc<0)
        return -2;
    if (FD_ISSET(rss->skt, &error_fdset))
        return -2;
    if (FD_ISSET(rss->skt, &read_fdset))
        return 1;
    return 0;
}

/*f remote_submodule_server_handle_packet_claim
      IP UDP message of 'claim' with 8 bytes of message - response of 'already claimed' with current claimant 'message' or 'success' (success indicates setting of claimant ARP entry and state of 'claimed')
 */
void c_engine::remote_submodule_server_handle_packet_claim( t_se_remote_submodule_server_ptr rss, t_udp_packet *pkt )
{
    if (rss->claimed)
    {
        if (!remote_submodule_server_check_source(rss, pkt ))
            remote_submodule_server_transmit_packet_already_claimed( rss, pkt );
        return;
    }
    rss->claimed = 1;
    rss->claimant = pkt->address;
    rss->sequence_number = pkt->data.claim.sequence_number;
    remote_submodule_server_transmit_packet_claim_ok( rss, pkt );
}

/*f remote_submodule_server_receive_packet
 */
int c_engine::remote_submodule_server_receive_packet( t_se_remote_submodule_server_ptr rss )
{
    t_udp_packet udp_packet;

    udp_packet.length = recvfrom( rss->skt, (void *)udp_packet.data.buffer, sizeof(udp_packet.data.buffer), 0 /*flags*/, &udp_packet.address, sizeof(udp_packet.address) );
    if (udp_packet.length<=0)
        return -1;
    /*
      IP UDP message of 'release' - response of 'released' state <= unclaimed or 'wrong claimant' if IP address or port mismatch
      IP UDP message of 'force release' - response of 'released' - state <= unclaimed
      IP UDP message of 'set inputs' with sequence number - response of 'inputs set' or 'bad sequence number' (sets sequence number if okay - bad if new sequence number != old sequence number+1)
      IP UDP message of 'get sequence number' to get claimant sequence number
      IP UDP message of 'clock tick' with clock mask - response of 'output values' or 'bad sequence number'
      IP UDP message of 'get output values' - response of 'output values' or 'bad sequence number'
    */

    return 0;
}

/*f remote_submodule_server_poll
  Poll (continuously to a max iterations, which may be -1 to mean forever)
  Returns -1 for not connected, 0 for timeout, 1 for handled packet(s)
 */
int c_engine::remote_submodule_server_poll( t_se_remote_submodule_server_ptr rss, int timeout, int max_iterations )
{
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

