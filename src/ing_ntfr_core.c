/* ing_ntfr_core.c
 *
 * Copyright (c) 2013-2021 Inango Systems LTD.
 *
 * Author: Inango Systems LTD. <support@inango-systems.com>
 * Creation Date: Mar 2016
 *
 * The author may be reached at support@inango-systems.com
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Subject to the terms and conditions of this license, each copyright holder
 * and contributor hereby grants to those receiving rights under this license
 * a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
 * (except for failure to satisfy the conditions of this license) patent license
 * to make, have made, use, offer to sell, sell, import, and otherwise transfer
 * this software, where such license applies only to those patent claims, already
 * acquired or hereafter acquired, licensable by such copyright holder or contributor
 * that are necessarily infringed by:
 *
 * (a) their Contribution(s) (the licensed copyrights of copyright holders and
 * non-copyrightable additions of contributors, in source or binary form) alone;
 * or
 *
 * (b) combination of their Contribution(s) with the work of authorship to which
 * such Contribution(s) was added by such copyright holder or contributor, if,
 * at the time the Contribution is added, such addition causes such combination
 * to be necessarily infringed. The patent license shall not apply to any other
 * combinations which include the Contribution.
 *
 * Except as expressly stated above, no rights or licenses from any copyright
 * holder or contributor is granted under this license, whether expressly, by
 * implication, estoppel or otherwise.
 *
 * DISCLAIMER
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * NOTE
 *
 * This is part of a management middleware software package called MMX that was developed by Inango Systems Ltd.
 *
 * This version of MMX provides web and command-line management interfaces.
 *
 * Please contact us at Inango at support@inango-systems.com if you would like to hear more about
 * - other management packages, such as SNMP, TR-069 or Netconf
 * - how we can extend the data model to support all parts of your system
 * - professional sub-contract and customization services
 *
 */
/* Inango Notifier is a SW component allowing to pass notifications
 * from various applications to network management entities like
 * SNMP agent, TR-069 client, syslog client, etc...
 */
/* This file contains core part of notifier what receive notification from
 * backends and resend it to listeners
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/inotify.h>
#include <time.h>

#include "ing_ntfr_defines.h"
#include "ing_ntfr_listeners.h"
#include "ing_ntfr_settings.h"


/*
 * Constants
 */
enum ntf_core_listeners
{
    NTF_LISTENER_LOGGER = 0,
    NTF_LISTENER_SYSLOG,
    NTF_LISTENER_SNMP,
    NTF_LISTENER_NETCONF,
    NTF_LISTENER_MMX,
    NTF_LISTENER_LAST
} ntf_core_listeners_t;


static int ntf_core_sockets_init( int *recv_sock, int *send_sock )
{
    *recv_sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( *recv_sock == -1 )
    {
        ERR( "cannot create receiving socket: socket() failed: %s (%d)",
              strerror(errno), errno );
        return -1;
    }

    *send_sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( *send_sock == -1 )
    {
        ERR( "cannot create sending socket: socket() failed: %s (%d)",
              strerror(errno), errno );
        close( *recv_sock );
        return -1;
    }

    return 0;
}


int ntf_get_syslog_enabled( char *buffer, size_t buff_len)
{
    if (!ntfsettings_get( "syslog_listener_enabled", buffer, buff_len ) && !strcmp("true", buffer))
        return 1;

    return 0;
}

int ntf_get_netconf_enabled( char *buffer, size_t buff_len)
{
    if (!ntfsettings_get( "netconf_listener_enabled", buffer, buff_len ) && !strcmp("true", buffer))
        return 1;

    return 0;
}

int ntf_get_logger_enabled( char *buffer, size_t buff_len)
{
    if (!ntfsettings_get( "logger_listener_enabled", buffer, buff_len ) && !strcmp("true", buffer))
        return 1;

    return 0;
}

int ntf_get_snmp_enabled( char *buffer, size_t buff_len)
{
    if (!ntfsettings_get( "snmp_listener_enabled", buffer, buff_len ) && !strcmp("true", buffer))
        return 1;

    return 0;
}

int ntf_get_mmx_enabled( char *buffer, size_t buff_len)
{
    if (!ntfsettings_get( "mmx_listener_enabled", buffer, buff_len) && !strcmp("true", buffer))
    {
        return 1;
    }
    return 0;
}


static void ntf_core_sockets_free( int *recv_sock, int *send_sock )
{
    close( *send_sock );
    close( *recv_sock );
}

/*
 * Main application thread
 */
int main( int __attribute__((__unused__)) argc, char __attribute__((__unused__)) *argv[] )
{
    int recv_sock, send_sock;

    int res, i, name_size, subres;
    char buffer[NTF_STR_MSG_BUFFER_LEN] = { 0 };
    size_t timeout;
    struct timeval waittime;
    struct timespec curr_time, old_time;
    struct sockaddr_in recv_addr, send_addr, lo_addr;
    struct ntf_listener listeners[NTF_LISTENER_LAST];

    memset((char *)listeners, 0, sizeof(listeners));

    memset( (void*)&recv_addr, 0, sizeof( struct sockaddr_in ) );
    memset( (void*)&send_addr, 0, sizeof( struct sockaddr_in ) );
    memset( (void*)&lo_addr,   0, sizeof( struct sockaddr_in ) );

    recv_addr.sin_family       = PF_INET;
    recv_addr.sin_port         = htons( NTF_PORT_SERVER );
    recv_addr.sin_addr.s_addr  = htonl( INADDR_ANY );

    send_addr.sin_family       = PF_INET;
    send_addr.sin_addr.s_addr  = htonl( INADDR_LOOPBACK );

    waittime.tv_sec  = 2;
    waittime.tv_usec = 0;

    name_size = sizeof(ntf_listener_t.name);


    if ( ntfsettings_init() != 0 )
    {
        ERR( "cannot load settings" );
        return -1;
    }




    /* load values of keys */
    ntfsettings_load( "snmp_srv" );
    ntfsettings_load( "snmp_community" );
    ntfsettings_load( "syslog_srv" );
    ntfsettings_load( "syslog_listener_enabled" );
    ntfsettings_load( "netconf_listener_enabled" );
    ntfsettings_load( "logger_listener_enabled" );
    ntfsettings_load( "snmp_listener_enabled" );
    ntfsettings_load( "mmx_listener_enabled" );


    listeners[NTF_LISTENER_LOGGER].enabled = ntf_get_logger_enabled(&buffer[0], sizeof(buffer));
    listeners[NTF_LISTENER_SYSLOG].enabled = ntf_get_syslog_enabled(&buffer[0], sizeof(buffer));
    listeners[NTF_LISTENER_SNMP].enabled = ntf_get_snmp_enabled(&buffer[0], sizeof(buffer));
    listeners[NTF_LISTENER_NETCONF].enabled = ntf_get_netconf_enabled(&buffer[0], sizeof(buffer));
    listeners[NTF_LISTENER_MMX].enabled = ntf_get_mmx_enabled(&buffer[0], sizeof(buffer));
    
    listeners[NTF_LISTENER_LOGGER].port  = NTF_PORT_LISTENER_LOGGER;
    listeners[NTF_LISTENER_LOGGER].init  = NULL;
    listeners[NTF_LISTENER_LOGGER].func  = NULL;
    listeners[NTF_LISTENER_LOGGER].clean = NULL;
    strncpy((char *)listeners[NTF_LISTENER_LOGGER].name, "logger", name_size);

    listeners[NTF_LISTENER_SYSLOG].port  = NTF_PORT_LISTENER_SYSLOG;
    listeners[NTF_LISTENER_SYSLOG].init  = &ntf_syslog_init;
    listeners[NTF_LISTENER_SYSLOG].func  = &ntf_call_syslog;
    listeners[NTF_LISTENER_SYSLOG].clean = &ntf_syslog_clean;
    strncpy((char *)listeners[NTF_LISTENER_SYSLOG].name, "syslog", name_size);

    listeners[NTF_LISTENER_SNMP].port  = NTF_PORT_LISTENER_SNMP; 
    listeners[NTF_LISTENER_SNMP].init  = &ntf_snmp_init;
    listeners[NTF_LISTENER_SNMP].func  = &ntf_call_snmp_trap;
    listeners[NTF_LISTENER_SNMP].clean = &ntf_snmp_clean;
    strncpy((char *)listeners[NTF_LISTENER_SNMP].name, "snmp", name_size);

    listeners[NTF_LISTENER_NETCONF].port  = NTF_PORT_LISTENER_NETCONF;
    listeners[NTF_LISTENER_NETCONF].init  = &ntf_netconf_init;
    listeners[NTF_LISTENER_NETCONF].func  = &ntf_send_netconf_notif;
    listeners[NTF_LISTENER_NETCONF].clean = &ntf_netconf_clean;
    strncpy((char *)listeners[NTF_LISTENER_NETCONF].name, "netconf", name_size);
    
    listeners[NTF_LISTENER_MMX].port  = NTF_PORT_LISTENER_MMX;
    listeners[NTF_LISTENER_MMX].init  = &ntf_mmx_init;
    listeners[NTF_LISTENER_MMX].func  = &ntf_send_mmx_notif;
    listeners[NTF_LISTENER_MMX].clean = &ntf_mmx_clean;
    strncpy((char *)listeners[NTF_LISTENER_MMX].name, "mmx", name_size);


    /* initialize receive/send UDP sockets */
    if ( ntf_core_sockets_init( &recv_sock, &send_sock ) != 0 )
    {
        ntfsettings_free();
        return -1;
    }

    if ( setsockopt( recv_sock, SOL_SOCKET, SO_RCVTIMEO,
                    (void*)&waittime, sizeof( struct timeval ) ) < 0 )
    {
        ERR( "Cannot set socket receive timeout, err %d (%s)", errno, strerror(errno));
        goto reterr;
    }

    if (listeners[NTF_LISTENER_SYSLOG].enabled) {
        if ( pthread_create( &listeners[NTF_LISTENER_SYSLOG].thread_id,
                             NULL, &ntf_handler, &listeners[NTF_LISTENER_SYSLOG] ) != 0 )
        {
            ERR( "Cannot create pthread for syslog handler" );
            goto reterr;
        }
        LOG("Syslog listener thread id: %lu",listeners[NTF_LISTENER_SYSLOG].thread_id );
    } else {
        LOG("syslog listener disabled");
    }


    if (listeners[NTF_LISTENER_SNMP].enabled) {
        if ( pthread_create( &listeners[NTF_LISTENER_SNMP].thread_id,
            NULL, &ntf_handler, &listeners[NTF_LISTENER_SNMP] ) != 0 )
        {
            ERR( "Cannot create pthread for SNMP handler" );
            goto reterr;
        }
        LOG("SNMP listener thread id: %lu",listeners[NTF_LISTENER_SNMP].thread_id );
    } else {
        LOG("SNMP listener disabled");
     }

     
 
    if (listeners[NTF_LISTENER_NETCONF].enabled) {
        if ( pthread_create( &listeners[NTF_LISTENER_NETCONF].thread_id,
            NULL, &ntf_handler, &listeners[NTF_LISTENER_NETCONF] ) != 0 )
        {
            ERR( "Cannot create pthread for NETCONF handler" );
            goto reterr;
        }
        LOG("Netconf listener thread id: %lu",listeners[NTF_LISTENER_NETCONF].thread_id );
    } else {
        LOG("netconf listener disabled");
    }
    

    if (listeners[NTF_LISTENER_MMX].enabled) {
        if ( pthread_create( &listeners[NTF_LISTENER_MMX].thread_id,
            NULL, &ntf_handler, &listeners[NTF_LISTENER_MMX] ) != 0 )
        {
            ERR( "Cannot create pthread for MMX handler" );
            goto reterr;
        }
        LOG("MMX listener thread id: %lu",listeners[NTF_LISTENER_MMX].thread_id );
    } else {
        LOG("mmx listener disabled");
    }

    
    res = bind( recv_sock, (struct sockaddr*)&recv_addr, sizeof( struct sockaddr_in ) );
    if ( res < 0 )
    {
        ERR( "Cannot bind recv sock to server port %u, err %d (%s)", 
              NTF_PORT_SERVER, errno, strerror(errno));
        goto reterr;
    }

    INF( "Notifier core successfully started" );

    clock_gettime( CLOCK_MONOTONIC, &old_time );
    for( ;; )
    {
        /* receive notification */
        memset( (void*)&buffer[0], 0, sizeof( buffer ) );
        memset( (void*)&curr_time, 0, sizeof( struct timespec ) );

        res = recv( recv_sock, (void*)&buffer[0], NTF_STR_MSG_BUFFER_LEN, 0 );
        if ( res >= 0 )
        {
            /* receive notification */
            for( i = 0; i < NTF_LISTENER_LAST; ++i )
            {   
                if (!listeners[i].enabled)
                    continue;
                send_addr.sin_port = htons( listeners[i].port );

                subres = sendto( send_sock,
                             (void*)&buffer[0], (size_t)res,
                              0,
                             (struct sockaddr*)&send_addr,
                              sizeof( struct sockaddr_in) );
                if ( subres < 0 )
                {
                    ERR("Failed to send ntf to listener port %u, err: %d (%s)",
                        listeners[i].port, errno, strerror(errno) );
                }
            }
        }
        else if ( res < -1 )
        {
            /* error */
            ERR("Failed to receive msg, err %d (%s)", errno, strerror(errno));
            goto reterr;
        }
        else
        {
            /* timeout, do nothing */
        }

        clock_gettime( CLOCK_MONOTONIC, &curr_time );
        timeout = curr_time.tv_sec - old_time.tv_sec;

        /* ToDo */
        if ( timeout > NTF_CONF_FILE_MONITOR_TIMEOUT )
        {
            ntf_settings_update();
            memcpy( &old_time, &curr_time, sizeof( struct timespec ) );
        }

    }

    res = 0;
    goto out;

reterr:
    res = -1;
out:
    ntf_core_sockets_free( &recv_sock, &send_sock );
    ntfsettings_free();
    return ( res );
}
