/* ing_ntfr.c
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
/* This file contains API and helpers functions for notifier shared library
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ing_ntfr_defines.h"

/*
 * Serialize notification
 */
static ntf_stat_t ntfproto_encode( struct ing_notification *notif,
                                   char buffer[], size_t *buff_len )
{
    size_t blen;
    char *pbuff, **pstring;
    int len, i;

    if ( ( notif == NULL ) || ( buffer == NULL ) || ( buff_len == NULL ) )
        return NTF_ST_BAD_INPUT_PARAMS;

    pbuff = buffer;
    blen  = *buff_len;

    len = snprintf( pbuff, blen,
                    "%i;%i;%i;%i;",
                    notif->msg_id,
                    notif->module_id,
                    notif->severity,
                    notif->param_num );

    if ( len < 0 )
        return NTF_ST_GENERAL_ERROR;
    if ( (size_t)len >= blen - 1 )
        return NTF_ST_NOMEMORY;

    blen  -= (size_t)len;
    pbuff += (size_t)len;

    pstring = notif->params;
    for( i = 0; i < notif->param_num; ++i )
    {
        len = snprintf( pbuff, blen, "%s;", *pstring );
        if ( len < 0 )
            return NTF_ST_GENERAL_ERROR;
        if ( (size_t)len >= blen - 1 )
            return NTF_ST_NOMEMORY;
        blen  -= (size_t)len;
        pbuff += (size_t)len;
        ++pstring;
    }
    ( *buff_len ) = pbuff - buffer;

    return NTF_ST_OK;
}

enum ntf_parse_states
{
    NTF_PS_NID = 0,
    NTF_PS_MID,
    NTF_PS_SEVERITY,
    NTF_PS_PNUM,
    NTF_PS_PAR,
    NTF_PS_LAST
};

/*
 * Deserialize notification
 */
static ntf_stat_t ntfproto_decode( struct ing_notification *notif,
                                   char string[], size_t str_len,
                                   char *param_pool, size_t *pool_len )
{
    char* ( *params)[NTF_PARAM_IN_MSG_MAX];
    char *par, *ppool, *psave, *pbegin;
    size_t len, free_pool_size, state;
    int param_num;

    /* validate pointers to input parameters */
    if ( ( notif == NULL ) || ( str_len == 0 )
      || ( param_pool == NULL ) || ( pool_len == NULL ) )
        return NTF_ST_BAD_INPUT_PARAMS;

    state          = NTF_PS_NID;
    param_num      = 0;
    free_pool_size = ( *pool_len );
    ppool          = param_pool;
    params         = &notif->params;

    pbegin = (char*)malloc(strlen(string)+1);
    strcpy(pbegin, string);
    par = pbegin;
    psave = strchr(par, ';');
    while (par != NULL && psave != NULL )
    {
        *psave = '\0'; // change delimiter (";") to \0
        switch( state )
        {
        case NTF_PS_NID:
            notif->msg_id = atoi( par );
            ++state;
            break;
        case NTF_PS_MID:
            notif->module_id = atoi( par );
            ++state;
            break;
        case NTF_PS_SEVERITY:
            notif->severity = atoi( par );
            ++state;
            break;
        case NTF_PS_PNUM:
            notif->param_num = atoi( par );
            ++state;
            break;
        case NTF_PS_PAR:
            len = strlen( par );
            if ( len < free_pool_size )
            {
                strcpy( ppool, par );
                (*params)[param_num] = ppool;
                ppool += len;
                *ppool = '\x0';
                ++ppool;
                ++param_num;
                free_pool_size -= ( len + 1 );
                if ( param_num > notif->param_num )
                    return NTF_ST_GENERAL_ERROR;
            }
            else
                return NTF_ST_NOMEMORY;
            break;
        }

        par = ++psave;
        psave = strchr(par, ';');
    }
    free(pbegin);

    /* not all data has been parsed
     */
    if ( state != NTF_PS_PAR )
        return NTF_ST_UNKNOWN_MSG_ID;

    return NTF_ST_OK;
}

/*
 * Send notification
 */
ntf_stat_t ing_notification_send( struct ing_notification *notif,
                                  int __attribute__((__unused__)) flags )
{
    char buffer[NTF_STR_MSG_BUFFER_LEN];
    ntf_stat_t res;
    size_t len;
    int sock;
    struct sockaddr_in addr = { 0 };

    len = NTF_STR_MSG_BUFFER_LEN;

    if ( ntfproto_encode( notif, buffer, &len ) != NTF_ST_OK )
        return NTF_ST_BAD_INPUT_PARAMS;

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( sock == -1 )
        return NTF_ST_GENERAL_ERROR;

    addr.sin_family      = PF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    addr.sin_port        = htons( NTF_PORT_SERVER );

    if ( sendto( sock, (void*)buffer, len, 0,
                (struct sockaddr*)&addr, sizeof( struct sockaddr_in ) ) < 0 )
    {
        res = NTF_ST_FAIL_NETWORK_OPERATION;
    }
    else
        res = NTF_ST_OK;

    close( sock );

    LOG( "Notification [%s] is sent to the notifier core", buffer );
    return res;
}

/*
 * Create listener handler
 */
int ing_listener_init( unsigned short port, unsigned long timeout )
{
    int sock, res, sock_opt;
    unsigned long time_lim;
    struct sockaddr_in addr, lo_addr;
    struct timeval waittime;
    struct ip_mreq mreq;

    memset( (void*)&addr,    0, sizeof( struct sockaddr_in ) );
    memset( (void*)&lo_addr, 0, sizeof( struct sockaddr_in ) );
    memset( (void*)&mreq,    0, sizeof( struct ip_mreq ) );

    addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    addr.sin_family = PF_INET;
    addr.sin_port   = htons( port );

    time_lim = timeout;
    if ( time_lim < NTF_MSG_TIMEOUT_MIN )
        time_lim = NTF_MSG_TIMEOUT_MIN;
    if ( time_lim > NTF_MSG_TIMEOUT_MAX )
        time_lim = NTF_MSG_TIMEOUT_MAX;

    waittime.tv_sec  = time_lim;
    waittime.tv_usec = 0u;

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( sock < 0 )
    {
        ERR( "Cannot create socket, err %d (%s)", errno, strerror(errno));
        return -1;
    }

    sock_opt = 1;
    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
                    (void*)&sock_opt, sizeof( int ) ) != 0 )
    {
        ERR( "Cannot reuse address, err %d (%s)", errno, strerror(errno));
        goto reterr;
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO,
                    (void*)&waittime, sizeof( struct timeval ) ) < 0 )
    {
        ERR( "Cannot set timeout, err %d (%s)", errno, strerror(errno));
        goto reterr;
    }

    res = bind( sock, (struct sockaddr*)&addr, sizeof( struct sockaddr_in ) );
    if ( res < 0 )
    {
        ERR( "Failed to bind to port %d, err %d (%s)", port, errno, strerror(errno));
        goto reterr;
    }

    return sock;

reterr:
    close( sock );
    return -1;
}

/*
 * Free listener handler
 */
void ing_listener_free( int handle )
{
    close( handle );
}

/*
 * Receive notification
 */
ntf_stat_t ing_notification_recv( int listener_handle, char *listener_name,
                                  struct ing_notification *notif, int flags,
                                  char *param_pool, size_t *pool_len )
{
    int res, recv_flags;
    char buffer[NTF_STR_MSG_BUFFER_LEN] = { 0 };

    memset( (void*)&buffer[0], 0, NTF_STR_MSG_BUFFER_LEN );

    recv_flags = 0;
    if ( flags & NTF_MSG_DONOTWAIT )
        recv_flags = MSG_DONTWAIT;

    res = recv( listener_handle, buffer, NTF_STR_MSG_BUFFER_LEN, recv_flags );
    if ( res == -1 )
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK )
        {
            return NTF_ST_TIMEOUT;
        }

        ERR("%s listener failed in recv(), err %d (%s)", listener_name, 
             errno, strerror(errno));
        return NTF_ST_FAIL_NETWORK_OPERATION;
    }

    LOG( "%s listener received notification [%s]", listener_name, buffer );
    res = ntfproto_decode( notif, buffer, (size_t)res,
                           param_pool, pool_len );
    return res;
}
