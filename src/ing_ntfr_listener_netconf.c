/* ing_ntfr_listener_netconf.c
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
 *
 * This file contains NETCONF notification listener implementation
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <microxml.h>

#include "ing_ntfr_defines.h"
#include "ing_ntfr_messages.h"
#include "ing_ntfr_listeners.h"
#include "ing_ntfr_settings.h"


/*
 * Export database from auto-generated file
 */
extern struct ntf_netconf_db_entry ntf_netconf_db[NTF_MAX_DB_MESSAGE_NUM];


#define NCNTF_MMXEVENT_MSGSIZE            (1024)
char ntf_netconf_message[NCNTF_MMXEVENT_MSGSIZE];

#define NCNTF_MMXEVENT_CONTENT_PARAMSIZE   (256)

#define NETCONF_SRV_ADDR       (INADDR_LOOPBACK)
#define NETCONF_SRV_PORT                  (7890)

/*
 * UDP socket to send notification message to.
 *
 *  Incoming lower-layer notification is parsed, converted
 *  to XML format suitable for NETCONF and then is transferred
 *  via the socket to the NETCONF server
 */
static int sockfd  = -1;


static struct ntf_netconf_db_entry* select_notif_from_netconf_db( struct ing_notification *notif )
{
    int i;
    struct ntf_netconf_db_entry *netconf_notif = NULL;

    /* match the entry in db - by msg_id */
    for ( i = 0 ; i < NTF_MAX_DB_MESSAGE_NUM; ++i )
    {
        // no more valid items expected in ntf_netconf_db
        if ( ntf_netconf_db[i].msg_id == NTF_MSG_NOTUSED )
            break;

        if ( ntf_netconf_db[i].msg_id == notif->msg_id )
        {
            netconf_notif = &ntf_netconf_db[i];
            break;
        }
    }

    if ( netconf_notif == NULL || netconf_notif->type == NULL || strcmp( netconf_notif->type, "" ) == 0 )
    {
        /* unknown msg_id or invalid entry type */
        goto reterr;
    }

    /* validate found entry params: first param MUST NOT have empty-string par_info */
    if ( strcmp( netconf_notif->params[0].par_info, "" ) == 0 )
    {
        ERR( "DB notification (%s) found but invalid: empty-string param #0", netconf_notif->type );
        goto reterr;
    }

    return netconf_notif;
reterr:
    return NULL;
}

static int prepare_netconf_message( struct ing_notification *notif, struct ntf_netconf_db_entry *netconf_notif )
{
    int i, j, k, len;
    mxml_node_t *xml_notif;
    mxml_node_t *node, *content_node;

    char param_content[NCNTF_MMXEVENT_CONTENT_PARAMSIZE] = {0};
    char multiparam_content[NCNTF_MMXEVENT_CONTENT_PARAMSIZE] = {0};

    xml_notif = mxmlLoadString(NULL, netconf_notif->template, MXML_OPAQUE_CALLBACK);
    if ( xml_notif == NULL )
    {
        ERR( "Cannot load notification XML template" );
        return -1;
    }

    if (strcmp(mxmlGetElement(xml_notif), "mmx-event") == 0)
    {
        node = mxmlFindElement(xml_notif, xml_notif, "type", NULL, NULL, MXML_DESCEND);
        if ( node == NULL )
        {
            ERR( "Missing node 'type' in loaded notification XML template" );
            mxmlDelete( xml_notif );
            return -1;
        }
        mxmlNewText(node, 0, netconf_notif->type);
        node = mxmlFindElement(xml_notif, xml_notif, "content", NULL, NULL, MXML_DESCEND);
        if ( node == NULL )
        {
            ERR( "Missing node 'content' in loaded notification XML template" );
            mxmlDelete( xml_notif );
            return -1;
        }
    } else {
        node = xml_notif;
    }


    /* process params - some content nodes can be made of multiple params values */
    for ( i = 0; i < netconf_notif->param_num; i++ )
    {
        memset( param_content, 0, NCNTF_MMXEVENT_CONTENT_PARAMSIZE );
        memset( multiparam_content, 0, NCNTF_MMXEVENT_CONTENT_PARAMSIZE );
        if ( notif->params[i] == NULL )
        {
            ERR( "Missing data for parameter %d in request (tag_name: %s)", i, netconf_notif->params[i].par_info );
            mxmlDelete( xml_notif );
            return -1;
        }

        len = strlen(notif->params[i]);
        if ( len == 0 )
        {
            /* empty notification parameter would not be inserted to Xml */
            continue;
        }

        for ( j = i + 1; j < netconf_notif->param_num; j++ )
        {
            if ( strcmp( netconf_notif->params[j].par_info, "" ) != 0 )
            {
                break;
            }

            len = len + strlen(notif->params[j]);
        }

        j--;

        content_node = mxmlNewElement(node, netconf_notif->params[i].par_info);
        if ( j == i )
        {
            if ( netconf_notif->params[i].convert_func == NULL )
            {
                /* content node is made of sole param value without convert func */
                mxmlNewText(content_node, 0, notif->params[i]);
            }
            else
            {
                /* content node is made of sole param value with convert func */
                if ( netconf_notif->params[i].convert_func( notif->params[i],
                                                            &param_content[0], NULL ) != 0 )
                {
                    ERR( "Cannot convert value of node 'content/%s'",
                         netconf_notif->params[i].par_info );
                    mxmlDelete( xml_notif );
                    return -1;
                }
                mxmlNewText(content_node, 0, param_content);
            }
        }
        else /* j > i */
        {
            /* content node is made of multiple params values */
            if ( len >= NCNTF_MMXEVENT_CONTENT_PARAMSIZE )
            {
                ERR( "Node 'content/%s' value it too large (%d > %d)",
                      netconf_notif->params[i].par_info, len,
                      NCNTF_MMXEVENT_CONTENT_PARAMSIZE - 1 );
                mxmlDelete( xml_notif );
                return -1;
            }

            for ( k = i; k <= j; k++ )
            {
                strcat(multiparam_content, notif->params[k]);
            }
            mxmlNewText(content_node, 0, multiparam_content);

            i = j;
        }
    }

    memset( ntf_netconf_message, 0, NCNTF_MMXEVENT_MSGSIZE );
    if ( mxmlSaveString(xml_notif, ntf_netconf_message, NCNTF_MMXEVENT_MSGSIZE, MXML_NO_CALLBACK) <= 0 )
    {
        ERR( "Cannot save XML notification to string" );
        mxmlDelete( xml_notif );
        return -1;
    }

    mxmlDelete( xml_notif );
    return strlen(ntf_netconf_message);
}

static int send_netconf_message()
{
    struct sockaddr_in addr = {0};
    addr.sin_family         = PF_INET;
    addr.sin_addr.s_addr    = htonl( NETCONF_SRV_ADDR );
    addr.sin_port           = htons( NETCONF_SRV_PORT );

    /*
    LOG( "Sending prepared NETCONF notification: \n%s", ntf_netconf_message );
    */

    if ( sendto( sockfd, (void*)ntf_netconf_message, strlen(ntf_netconf_message),
                 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in) ) < 0 )
    {
        ERR( "sendto() failed: %s (%d)", strerror(errno), errno );
        return -1;
    }

    /*
    LOG( "NETCONF notification was successfully forwarded" );
    */

    return 0;
}

/*
 * NETCONF listener 'init' function implementation
 */
int ntf_netconf_init( void *args )
{
    /* open communication socket */
    sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( sockfd == -1 )
    {
        return -1;
    }

    return 0;
}

/*
 * NETCONF listener 'clean' function implementation
 */
int ntf_netconf_clean()
{
    if ( sockfd != -1 )
    {
        /* close communication socket */
        close( sockfd );
        sockfd = -1;
    }

    return 0;
}

/*
 * NETCONF listener 'func' function implementation
 *   used to notify NETCONF server of hapenning events
 */
int ntf_send_netconf_notif( struct ing_notification *notif )
{
    int msglen;
    struct ntf_netconf_db_entry *netconf_notif = NULL;
    int res;

    if ( notif == NULL )
    {
        ERR( "Bad input: notification is NULL" );
        return -1;
    }

    netconf_notif = select_notif_from_netconf_db( notif );
    if ( netconf_notif == NULL )
    {
        // ERR( "Notification (%d) is unknown in NETCONF listener. Ignore", notif->msg_id );
        return 0;
    }

    if (netconf_notif->validate != NULL){
        res = netconf_notif->validate(notif);
        if (res == 1){
            LOG("sending notification is not needed (msg id %d)", notif->msg_id);
            return 0;
        } else if (res != 0){
            ERR("validate notification error (msg id %d)", notif->msg_id);
            return res;
        }
    }

    msglen = prepare_netconf_message( notif, netconf_notif );
    if ( msglen <= 0 )
    {
        ERR( "Cannot prepare NETCONF notification (%d) message", notif->msg_id );
        return -1;
    }

    return send_netconf_message();
}
