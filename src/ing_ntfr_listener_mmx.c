/* ing_ntfr_listener_mmx.c
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
 
/* This file contains implementation of MMX notification listener
 */

#include "mmx-frontapi.h"

#include "ing_ntfr_defines.h"
#include "ing_ntfr_messages.h"
#include "ing_ntfr_settings.h"
#include "ing_ntfr_listeners.h"

/*
 * Constants
 */
#define  NTF_MMX_SOCKET_TIMEOUT  3
#define  NTF_MMX_PORT            15028


/*   Global data used by MMX listener code */

/*
 * Export database from auto-generated file
 */
extern ntf_mmx_msg_db_entry_t ntf_mmx_msg_db[NTF_MAX_DB_MESSAGE_NUM];

/* Connection to the MMX entry-point*/
mmx_ep_connection_t ntf_mmxmsg_epconn;

ep_message_t ntf_ep_req;

int ntf_mmx_txaid;


/* ****************************
 *  Static helper functions
 * ***************************/

/*
 * transaction ID for MMX request
 */
static int ntf_mmx_get_txaid()
{
    if (ntf_mmx_txaid >= 0  && ntf_mmx_txaid < 65533)
        ntf_mmx_txaid++;
    else
        ntf_mmx_txaid = 1;
        
    return ntf_mmx_txaid;
}

/* Prepare the constant part of MMX request header */
static void ntf_mmx_prepare_msg_header (ep_message_t *ep_req)
{
    memset((char*)ep_req, 0, sizeof(ep_message_t));
    
    ep_req->header.msgType     = MSGTYPE_DISCOVERCONFIG,
    ep_req->header.callerId    = MMX_API_CALLERID_WEB; // TODO: Think about NTF caller id
    ep_req->header.respIpAddr  = htonl( INADDR_LOOPBACK );
    ep_req->header.respPort    = NTF_MMX_PORT;
    ep_req->header.respMode    = MMX_API_RESPMODE_NORESP; // MMX response is not needed
}


/* ***************************
 *   MMX listener methods
 * ***************************/ 

int ntf_mmx_init( void *args )
{
    int res_con = 0;
    
    res_con = mmx_frontapi_connect( &ntf_mmxmsg_epconn, NTF_MMX_PORT,
                                    NTF_MMX_SOCKET_TIMEOUT );
    
    if ( res_con != 0 )
    { 
        ERR("Failed to create MMC connection (%d)", res_con);
        return -1;
    }

    ntf_mmx_txaid = 0;
    
    ntf_mmx_prepare_msg_header (&ntf_ep_req);

    return 0;
}


int ntf_mmx_clean()
{
    mmx_frontapi_close(&ntf_mmxmsg_epconn);
    return 0;
}

int ntf_send_mmx_notif( struct ing_notification *notif )
{
    int i = 0;
    int found = 0;
    int status = 0, res = 0;
    ep_packet_t *packet;
    char buffer[1024] = {0}; 
    
    while (!found)
    {
        /*LOG("i = %d, msg_id in db = %d, msg id in the ntf %d", i, 
             ntf_mmx_msg_db[i].msg_id, notif->msg_id);*/
        
        /* Check the end of the messages list */
        if ( ntf_mmx_msg_db[i].msg_id == NTF_MSG_NOTUSED )
            break;

        if ( notif->msg_id == ntf_mmx_msg_db[i].msg_id )
        {
            found = 1;
            
            /* Fill request type and body of the MMX request  */
            memset((char *)&ntf_ep_req.body, 0, sizeof(ntf_ep_req.body));
            
            if (notif->msg_id == NTF_MSG_MMXMODULESTARTED)
            {
                ntf_ep_req.header.msgType = ntf_mmx_msg_db[i].mmx_req_type;
                
                strncpy( ntf_ep_req.body.discoverConfig.backendName, 
                         notif->params[0], MSG_MAX_STR_LEN );
                         
                LOG("Notification MMXMODULESTARTED received; module name %s",
                     notif->params[0]);
                         
            }
            else if (notif->msg_id == NTF_MSG_MMXOBJCHANGED)
            {
                ntf_ep_req.header.msgType = ntf_mmx_msg_db[i].mmx_req_type;
                
                strncpy( ntf_ep_req.body.discoverConfig.objName, 
                         notif->params[0], MSG_MAX_STR_LEN);
                         
                LOG("Ntf MMXOBJCHANGED received; obj name %s; msg type %d",
                     notif->params[0], ntf_ep_req.header.msgType);
                
            }
            
            ntf_ep_req.header.txaId = ntf_mmx_get_txaid();
        }
        i++;
    }
    
    /* Send request to MMX  Entry-Point */
    if (found)
    {
        packet = (ep_packet_t*)buffer;
        memset(packet->flags, 0, sizeof(packet->flags));

        status = mmx_frontapi_message_build (&ntf_ep_req, packet->msg, sizeof(buffer));
        if (status == FA_OK )
        {
           status = mmx_frontapi_send_req (&ntf_mmxmsg_epconn, packet);
           if (status == FA_OK)
           {
               LOG("MMX request %d (%s) was successfully sent to MMX", 
                    ntf_ep_req.header.msgType, msgtype2str(ntf_ep_req.header.msgType));
               res = 0;
           }
           else
           {
               LOG("Cannot send DISCOVERCONFIG request to MMX (%d)", status);
               res = -1;
           }
       }
       else
       {
           LOG("Cannot build DISCOVERCONFIG request (%d)", status);
           res = -2;
       }
    }
    else
    {
        //LOG("Notification %d is unknown for MMX listener. Ignore", notif->msg_id);
    }
    
    return res;
}


