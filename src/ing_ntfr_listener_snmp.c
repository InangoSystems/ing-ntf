/* ing_ntfr_listener_snmp.c
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
/* This file contains notification listeners implementation
 */

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ing_ntfr_defines.h"
#include "ing_ntfr_messages.h"
#include "ing_ntfr_settings.h"
#include "ing_ntfr_listeners.h"

/*
 * Constants
 */
 /*
  * Define snmp command for snmptrap v1 and snmptrap v2:
  * snmptrap v1 format : snmptrap -L n -v 1 -c <community> <trap-addr> <enterprise-id> <agent-addr> <trap-id> <ent-trap-value> <time> <varbinds>
  * snmptrap v2c format : snmptrap -L n -v 2c -c <community> <trap-addr> <time> <enterprise-id> <varbinds>
 */
#define NTF_SNMP_TRAP_STR "/usr/bin/snmptrap -L n -v 2c -c %s %s \"\" %s %s >/dev/null\n "\
                          "/usr/bin/snmptrap -L n -v 1 -c %s %s %s %s %d 0 \"\" %s >/dev/null"
/* first  - SNMP address,
 * second - trap OID
 */

/*
 * Export database from auto-generated file
 */
extern struct ntf_snmp_db_entry ntf_snmp_db[NTF_MAX_DB_MESSAGE_NUM];

/*
 * Get SNMP server address
 */
char* ntf_get_snmp_server_addr( char *buffer, size_t buff_len )
{
    if ( ntfsettings_get( "snmp_srv", buffer, buff_len ) != 0 )
        return NULL;
    return buffer;
}
char* ntf_get_snmp_community( char *buffer, size_t buff_len )
{
    if ( ntfsettings_get( "snmp_community", buffer, buff_len ) != 0 ){
        return NULL;
    }
    return buffer;
}

/*
 * Convert notifier type to snmp type
 */
char* ntf_snmp_convert_type( int ntf_type )
{
    switch( ntf_type )
    {
    case NTF_TYPE_INT:
        return "i";
    case NTF_TYPE_STR:
        return "s";
    }
    return NULL;
}

/*
 * Add parameter to command
 */
int ntf_snmp_add_param( char buffer[], size_t buff_len,
                        struct ntf_snmp_db_entry *snmp_trap,
                        struct ing_notification *notif,
                        size_t param_num )
{
    char conv_param[64] = { 0 };
    size_t pos;
    int res = -1;

    pos = strlen( buffer );
    if ( pos >= buff_len || param_num > snmp_trap->param_num)
        return res;

    if ( snmp_trap->params[param_num].convert_func != NULL )
    {
        if (snmp_trap->params[param_num].convert_func( notif->params[snmp_trap->params[param_num].input_idx - 1],
                                                           &conv_param[0], NULL ) != 0)
        {
                    ERR( "Cannot convert value %s", notif->params[snmp_trap->params[param_num].input_idx - 1]);
                    return res;

        }
    }
    else
        strcpy( conv_param, notif->params[snmp_trap->params[param_num].input_idx - 1] );

    if (sprintf( &buffer[pos], "  %s %s %s",
                        snmp_trap->params[param_num].par_info,
                        ntf_snmp_convert_type( snmp_trap->params[param_num].par_type ),
                        &conv_param[0] ) <= 0)
    {
        ERR("error prepare buffer with string: '%s %s %s'",
                        snmp_trap->params[param_num].par_info,
                        ntf_snmp_convert_type( snmp_trap->params[param_num].par_type ),
                        &conv_param[0]);
    }
    else
    {
        res = 0;
    }
    return res;
}

/*
 *
 */
int ntf_snmp_init( void __attribute__((__unused__)) *args )
{
    ntf_ifidx_db_init();
    
    return 0;
}
int ntf_snmp_clean( void __attribute__((__unused__)) *args )
{
    ntf_ifidx_db_deinit();

    return 0;
}

/*
 * send SNMP trap
 */
int ntf_call_snmp_trap(struct ing_notification *notif )
{
    char community[64] = { 0 };
    char trap_addr[64] = { 0 };
    char  cmd[1024] = { 0 };
    char  args[896] = { 0 };
    int  i, j, res;
    int msg_found;
    FILE *pf;

    msg_found = 0;

    for( i = 0 ; i < NTF_MAX_DB_MESSAGE_NUM; ++i )
    {
        // check the last entry in ntf_snmp_db
        if ( ntf_snmp_db[i].msg_id == NTF_MSG_NOTUSED )
            break;

        if ( notif->msg_id == ntf_snmp_db[i].msg_id )
        {
            if (ntf_snmp_db[i].validate != NULL){
                res = ntf_snmp_db[i].validate(notif);
                if (res == 1){
                    LOG("sending notification is not needed (msg id %d)", notif->msg_id);
                    return 0;
                } else if (res != 0){
                    ERR("validate notification error (msg id %d)", notif->msg_id);
                    return res;
                }
            }
            ntf_get_snmp_community( &community[0], sizeof( community ) );
            ntf_get_snmp_server_addr( &trap_addr[0], sizeof( trap_addr ) );
            for( j = 0; j < ntf_snmp_db[i].param_num; ++j )
                if (ntf_snmp_add_param( args, sizeof( args ),
                                                   &ntf_snmp_db[i], notif, (size_t)j ) != 0)
                {
                    ERR( "Cannot prepare SNMP notification (%d) message (param idx: %d)", notif->msg_id, j );
                    return -1;
                }
            msg_found = 1;
            sprintf( cmd, NTF_SNMP_TRAP_STR,
                // prepare msg for snmp version 2c
                community,trap_addr, ntf_snmp_db[i].trap_oid, args,
                // prepare msg for snmp version 1
                // TODO. need put correct value for agent address
                community,trap_addr, ntf_snmp_db[i].trap_oid, trap_addr, ntf_snmp_db[i].trap_type, args );
            break;
        }
    }

    if ( !msg_found )
    {
        return 0;
    }
    LOG( "SNMP trap cmd:\n  %s", cmd );

    /* ToDo add '&' to the end of string */
    /*
    // Not working code, if use popen for execute command, then
    // command is executed, but not real send notification (cannot see packets were sent, via tcpdump)
    pf = popen( cmd, "r" );
    if ( pf == NULL )
        return -1;

    fgets( answ, sizeof( answ ), pf );
    LOG( "Answer from snmptrap: %s", answ );

    pclose( pf );
    */

    if (system(cmd) != 0)
        ERR( "Cannot send SNMP notification");

    return 0;
}
