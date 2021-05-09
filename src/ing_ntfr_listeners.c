/* ing_ntfr_listeners.c
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
#define _GNU_SOURCE

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#include <mmx-frontapi.h>

#include "ing_ntfr_defines.h"
#include "ing_ntfr_messages.h"
#include "ing_ntfr_listeners.h"

/*
 * Constants
 */
#define NTF_PORT_MMXEP 15027

#define NTF_MMX_IFTABLE_PREFIX "Device.X_Inango_L2.IfTable."
#define NTF_MMX_IFNAME_PARAM_NAME "Name"
#define NTF_MMX_LINKUPDOWNTRAP_PARAM_NAME "X_Inango_LinkUpDownTrapEnable"

static char g_buffer[16 * 1024];
static char g_mem_pool[4 * 1024];
static pthread_rwlock_t ntf_ifidx_db_lock = PTHREAD_RWLOCK_INITIALIZER;

/*
 */
struct ntf_ifidx_db_table
{
    int    initialized;
    int    under_update;
    struct timespec timestamp;
    struct ntf_ifidx_db_entry table[NTF_MAX_IFIDX_NUM];
} ntf_ifidx_db_t;
struct ntf_ifidx_db_table ntf_ifidx_db_local;

/**
 * this is shared object to multiple threads(snmp and netconf)
 * all operations on the object must be protected by locks(ntf_ifidx_db_lock)
 */
struct ntf_ifidx_db_table *ntf_ifidx_db = NULL;
/**
 * function extract only first index and name from mmx_full_name
 * @param  mmx_full_name - full mmx obj instance name
 *                      ex: Device.X_Inango_L2.IfTable.3.Name
 * @param  prefix_len     - the length of the base part of the mmxObjName
 *                      ex: strlen("Device.X_Inango_L2.IfTable.")
 * @param  index        - output param index extracted from mmx_obj_name
 *                      ex: 3
 * @param  param_name   - output param param name extracted from mmx_obj_name
 *                      ex: "Name"
 * @return              - status code EXIT_SUCCESS
 */
static int extract_ifindex_and_paramname(char *mmx_full_name, int prefix_len, int *index, char ** param_name)
{
    char *curr;
    char cindex[5] ;
    char *pos = NULL;
    curr = mmx_full_name + prefix_len;
    pos = strchr(curr, '.');

    strncpy(cindex, curr, pos - curr);
    cindex[pos - curr] = 0;
    *index = atoi(cindex);
    *param_name = pos + 1;
    return EXIT_SUCCESS;
}

/*
 * Initialize local interface database
 * to get from mmx-ep the table "Device.X_Inango_L2.IfTable." and keep the structure ntf_ifidx_db_local
 * this code uses shared variables. Must be called safely only from one thread at a time.
 */
static int ntf_local_ifidx_db_init()
{
#define NTF_SOCKET_TIMEOUT              (5)
#define MAX_NUMBER_EP_RESP_FRAGMENTS    (4)

    int i, j, ifIndex, len;
    int mmx_res = 0;
    char* param_name;
    nvpair_t *param;
    size_t rcvd = 0, idx_num, recv_count;
    mmx_ep_connection_t mmxep_conn;
    ep_packet_t *packet;

    ep_message_t msg = {
            .header = {
                    /* TODO: now callerId = SNMP as request is sent only by Snmp ntfr listener.
                     * Maybe we need to prepare some generic callerId - not frontend-oriented */
                    .callerId   = MMX_API_CALLERID_SNMP,
                    .txaId      = 1,
                    .respIpAddr = htonl( INADDR_LOOPBACK ),
                    .respFlag   = 0,
                    .respMode   = MMX_API_RESPMODE_SYNC,
                    .respPort   = NTF_PORT_MMXEP,
                    .msgType    = MSGTYPE_GETVALUE,
                    .moreFlag   = 0
            }
    };
    
    msg.body.getParamValue.arraySize = 1;
    strcpy( msg.body.getParamValue.paramNames[0], NTF_MMX_IFTABLE_PREFIX"*." );

    memset( &ntf_ifidx_db_local, 0, sizeof( ntf_ifidx_db_local ) );
    memset(   g_buffer,    0, sizeof(   g_buffer   ) );
    memset(  g_mem_pool,   0, sizeof(  g_mem_pool  ) );

    mmx_res = mmx_frontapi_connect( &mmxep_conn, NTF_PORT_MMXEP, 
                                                 NTF_SOCKET_TIMEOUT );
    if ( mmx_res != FA_OK )
    {
        LOG("Cannot connect to MMX (res %d)", mmx_res);
        return -1;
    }

    packet = (ep_packet_t*)g_buffer;
    memset(packet->flags, 0, sizeof(packet->flags));

    mmx_res = mmx_frontapi_message_build( &msg, packet->msg, sizeof(g_buffer) );
    if ( mmx_res != FA_OK )
    {
        LOG("Cannot build to MMX message (res %d)", mmx_res);
        return -1;
    }
    
    mmx_res = mmx_frontapi_send_req( &mmxep_conn, packet );
    if ( mmx_res != FA_OK )
    {
        LOG("Failed to send request to MMX (res %d)", mmx_res);
        return -1;
    }

    idx_num = 0; recv_count = 0;
    len = strlen(NTF_MMX_IFTABLE_PREFIX);
    for( ;; )
    {
        rcvd = 0;
        memset( g_buffer, 0, sizeof(g_buffer) );
        mmx_res = mmx_frontapi_receive_resp( &mmxep_conn, 1, g_buffer, 
                                             sizeof(g_buffer), &rcvd );
        if ( mmx_res != FA_OK )
        {
            LOG("Failed to receive response from MMX (res %d)", mmx_res);
            return -1;
        }
        ++recv_count;

        mmx_frontapi_msg_struct_init( &msg, g_mem_pool, sizeof( g_mem_pool) );

        mmx_res =  mmx_frontapi_message_parse( g_buffer, &msg );
        if ( mmx_res != FA_OK )
        {
            LOG("Failed to parse MMX response (res %d)", mmx_res);
            return -1;
        }
        // the first loop stored in db ifname and ifindex
        for( i = 0; i < msg.body.getParamValueResponse.arraySize; ++i )
        {
            param = &(msg.body.getParamValueResponse.paramValues[i]);
            if ( param->name != NULL )
            {
                extract_ifindex_and_paramname(param->name, len, &ifIndex, &param_name);
                if ( strcmp ( NTF_MMX_IFNAME_PARAM_NAME,param_name ) == 0 )
                {
                    strcpy( &ntf_ifidx_db_local.table[idx_num].ifName[0], param->pValue );
                    ntf_ifidx_db_local.table[idx_num].ifIndex = ifIndex;
                    ++idx_num;
                    if ( idx_num >= NTF_MAX_IFIDX_NUM )
                        break;
                }
            }
        }
        // the second loop stored in db X_Inango_LinkUpDownTrapEnable
        for( i = 0; i < msg.body.getParamValueResponse.arraySize; ++i )
        {
            param = &(msg.body.getParamValueResponse.paramValues[i]);
            if ( param->name != NULL )
            {
                extract_ifindex_and_paramname(param->name, len, &ifIndex, &param_name);
                if ( strcmp ( NTF_MMX_LINKUPDOWNTRAP_PARAM_NAME,param_name ) == 0 )
                {
                    for( j = 0; j < idx_num; ++j )
                    {
                        if (ntf_ifidx_db_local.table[j].ifIndex == ifIndex)
                        {
                            if (strcmp(param->pValue, "true") == 0){
                                ntf_ifidx_db_local.table[j].link_trap_enable = 1;
                            } else {
                                ntf_ifidx_db_local.table[j].link_trap_enable = 0;
                            }
                            break;
                        }
                    }
                }
            }
        }

        if ( ( msg.header.moreFlag == 0 ) || ( recv_count > MAX_NUMBER_EP_RESP_FRAGMENTS ) )
            break;
    }

    mmx_frontapi_close( &mmxep_conn );
    clock_gettime( CLOCK_MONOTONIC, &ntf_ifidx_db_local.timestamp );
    ntf_ifidx_db_local.initialized = 1;
    return 0;
}

/**
 * updates ifidx db if necessary
 * if the database does not exist create
 * if the update is already running, return 0
 * otherwise, run the update, putting the flag under_update
 * @return 0 - if update is already running
 *         otherwise, update_status(result function ntf_local_ifidx_db_init)
 */
int ntf_ifidx_db_init()
{
    int under_update = 0;
    int res = 0;
    /*
    try to lock the database 
    in case of failure, someone is already using it(updates), update is not necessary
    otherwise under_update equal to 1 if the database is already update
    else need update
     */
    if (pthread_rwlock_trywrlock(&ntf_ifidx_db_lock) == 0){
        if (ntf_ifidx_db == NULL){
            ntf_ifidx_db = malloc(sizeof(struct ntf_ifidx_db_table));
            memset( ntf_ifidx_db, 0, sizeof(struct ntf_ifidx_db_table) );
        }
        if (ntf_ifidx_db->under_update)
            under_update = 1;
        else
            ntf_ifidx_db->under_update = 1;
        pthread_rwlock_unlock(&ntf_ifidx_db_lock);
    } else {
        return res;
    }

    if (under_update)
        return res;
    /*
    Prepare the local database(update from mmx-ep)
     */
    if ((res = ntf_local_ifidx_db_init()) != 0 )
    {
        ERR("error initialized local db");
        return res;
    }
     
    /*
    to copy a local database to complete the upgrade process 
     */
    INF( "updating ifIndex table" );
    pthread_rwlock_wrlock(&ntf_ifidx_db_lock);

    memcpy(ntf_ifidx_db, &ntf_ifidx_db_local, sizeof(ntf_ifidx_db_local));
    ntf_ifidx_db->under_update = 0;

    pthread_rwlock_unlock(&ntf_ifidx_db_lock);
    return res;
}

void ntf_ifidx_db_deinit()
{
    pthread_rwlock_wrlock(&ntf_ifidx_db_lock);
    if (ntf_ifidx_db != NULL){
        free(ntf_ifidx_db);
    }
    pthread_rwlock_unlock(&ntf_ifidx_db_lock);
}
/*
 * Return pointer to first entry of ifIndex database
 */
struct ntf_ifidx_db_entry* get_ifidx_first()
{
    return &ntf_ifidx_db->table[0];
}

/*
 * Find network interface entry by its name using a cache.
 * Returns:
 *  -1, if interface is not found
 *  0, if interface entry is found
 */
static int ntf_get_ifentry( char *ifname, struct ntf_ifidx_db_entry *ifentry )
{
    int i, res = -1;
    struct timespec curr_time;
    if ( ifname == NULL )
        return res;
    clock_gettime( CLOCK_MONOTONIC, &curr_time );
    /*
    define if should upgrade the database
    if the database does not exist or is not relevant
        need update, 
    else use older db.
     */
    if (pthread_rwlock_rdlock(&ntf_ifidx_db_lock) == 0){
        if ( ntf_ifidx_db==NULL || 
            (( curr_time.tv_sec - ntf_ifidx_db->timestamp.tv_sec ) > NTF_IFIDX_UPDATE_TIMEOUT) )
        {
            pthread_rwlock_unlock(&ntf_ifidx_db_lock);
            ntf_ifidx_db_init();
        }
        else
            pthread_rwlock_unlock(&ntf_ifidx_db_lock);
    }
    /*
    search and copy value from db
     */
    pthread_rwlock_rdlock(&ntf_ifidx_db_lock);
    if ( ntf_ifidx_db!=NULL )
    {
        for ( i = 0; i < NTF_MAX_IFIDX_NUM; ++i )
        {
            if ( ntf_ifidx_db->table[i].ifName[0] != 0 )
            {
                if ( strcmp( ifname, &ntf_ifidx_db->table[i].ifName[0] ) == 0 )
                {
                    memcpy(ifentry, &ntf_ifidx_db->table[i], sizeof(struct ntf_ifidx_db_entry));
                    res = 0;
                    break;
                }
            }
        }
    }
    pthread_rwlock_unlock(&ntf_ifidx_db_lock);
    return res;
}

/*
 * Parameters conversion
 */
int ntf_ifName_to_ifIndex( char *pvalue_in, char *pvalue_out, void *arg )
{
    struct ntf_ifidx_db_entry ifentry ;
    int res = -1;
    if ( ( pvalue_in == NULL ) || ( pvalue_out == NULL ) )
        return res;

    res = ntf_get_ifentry( pvalue_in, &ifentry );
    if (res == 0)
        sprintf( pvalue_out, "%d", ifentry.ifIndex );
    else
        ERR("Not found interface entry in ifidx cached table");

    return res;
}

int ntf_ifOperStatus_mmx_to_snmp( char *pvalue_in, char *pvalue_out, void *arg )
{
    if ( ( pvalue_in == NULL ) || ( pvalue_out == NULL ) )
        return -1;
    if ( (strcmp( pvalue_in, "1" ) == 0) || (strcmp( pvalue_in, "up" ) == 0) )
        strcpy( pvalue_out, "1" );
    else if ( (strcmp( pvalue_in, "2" ) == 0) || (strcmp( pvalue_in, "down" ) == 0) )
        strcpy( pvalue_out, "2" );
    else if ( (strcmp( pvalue_in, "3" ) == 0) || (strcmp( pvalue_in, "testing" ) == 0) )
        strcpy( pvalue_out, "3" );
    else if ( (strcmp( pvalue_in, "5" ) == 0) || (strcmp( pvalue_in, "dormant" ) == 0) )
        strcpy( pvalue_out, "5" );
    else if ( (strcmp( pvalue_in, "6" ) == 0) || (strcmp( pvalue_in, "notPresent" ) == 0) )
        strcpy( pvalue_out, "6" );
    else if ( (strcmp( pvalue_in, "7" ) == 0) || (strcmp( pvalue_in, "lowerLayerDown" ) == 0) )
        strcpy( pvalue_out, "7" );
    else
        strcpy( pvalue_out, "4" ); // 4 means OperStatus "unknown"

    return 0;
}

/*
 * The function gets link_up_down_trap_enable for interface
 * returns 0 if link_trap_enable equals "true"
 *         1 if link_trap_enable equals "false"
 *         -1 otherwise
 */
int ntf_validate_link_trap_enable(struct ing_notification *notif)
{
    struct ntf_ifidx_db_entry ifentry;
    char *ifname;
    int res = -1;

    if ( notif == NULL ){
        ERR("notification equal is NULL");
        return res;
    }

    if (notif->msg_id != NTF_MSG_LINKDOWN && notif->msg_id != NTF_MSG_LINKUP){
        ERR("not supported the message type for this handler(%d)", notif->msg_id);
        return res;
    }

    ifname = notif->params[0];
    res = ntf_get_ifentry( ifname, &ifentry );
    if (res == 0){
        if (ifentry.link_trap_enable) {
            res = 0;
        } else {
            res = 1;
        }
    } else {
        ERR("Not found interface entry by ifname (%s) in ntf_ifidx_db cached table", ifname);
    }
    return res;
}

int ntf_ifOperStatus_mmx_to_yang( char *pvalue_in, char *pvalue_out, void *arg )
{
    if ( ( pvalue_in == NULL ) || ( pvalue_out == NULL ) )
        return -1;

    if ( strcmp( pvalue_in, "1" ) == 0 )
        strcpy( pvalue_out, "up" );
    else if ( strcmp( pvalue_in, "2" ) == 0 )
        strcpy( pvalue_out, "down" );
    else if ( strcmp( pvalue_in, "3" ) == 0 )
        strcpy( pvalue_out, "testing" );
    else if ( strcmp( pvalue_in, "4" ) == 0 )
        strcpy( pvalue_out, "unknown" );
    else if ( strcmp( pvalue_in, "5" ) == 0 )
        strcpy( pvalue_out, "dormant" );
    else if ( strcmp( pvalue_in, "6" ) == 0 )
        strcpy( pvalue_out, "not-present" );
    else if ( strcmp( pvalue_in, "7" ) == 0 )
        strcpy( pvalue_out, "lower-layer-down" );
    else
        strcpy( pvalue_out, "unknown" );

    return 0;
}

int ntf_ifAdminStatus_mmx_to_yang( char *pvalue_in, char *pvalue_out, void *arg )
{
    if ( ( pvalue_in == NULL ) || ( pvalue_out == NULL ) )
        return -1;

    if ( strcmp( pvalue_in, "true" ) == 0 || strcmp( pvalue_in, "1" ) == 0 )
        strcpy( pvalue_out, "up" );
    else if ( strcmp( pvalue_in, "false" ) == 0 || strcmp( pvalue_in, "0" ) == 0 )
        strcpy( pvalue_out, "down" );
    else
        strcpy( pvalue_out, "testing" );

    return 0;
}

int ntf_dfeFwLevel_mmx_to_yang( char *pvalue_in, char *pvalue_out, void *arg )
{
    if ( ( pvalue_in == NULL ) || ( pvalue_out == NULL ) )
        return -1;

    if ( strcmp( pvalue_in, "0" ) == 0 )
        strcpy( pvalue_out, "unknown" );
    else if ( strcmp( pvalue_in, "1" ) == 0 )
        strcpy( pvalue_out, "B0" );
    else if ( strcmp( pvalue_in, "2" ) == 0 )
        strcpy( pvalue_out, "B1" );
    else if ( strcmp( pvalue_in, "3" ) == 0 )
        strcpy( pvalue_out, "APP" );
    else
        strcpy( pvalue_out, "unknown" );

    return 0;
}
/**
 * the function of the Convertion at datetime from libc format to Yang format
 * 
 * @param  xi_date - input datetime string (format "%Y-%m-%d %H:%M:%S")
 * @return         - output datetime string (format "%Y-%m-%dT%H:%M:%SZ")
 */
int ntf_datetime_libc2yang( char *pvalue_in, char *pvalue_out, void *arg )
{
#define FMT_INPUT_DATE "%Y-%m-%d %H:%M:%S"
#define FMT_OUTPUT_DATE "%Y-%m-%dT%H:%M:%SZ"
#define FMT_YANG_DATE_SIZE 24
    struct tm tm;
    if ( ( pvalue_in == NULL ) || ( pvalue_out == NULL ) )
        return -1;
    strptime(pvalue_in, FMT_INPUT_DATE, &tm);
    strftime(pvalue_out, FMT_YANG_DATE_SIZE, FMT_OUTPUT_DATE, &tm);
    return 0;
}

/*
 */
void* ntf_handler( void *args )
{
    int ntf_handle;
    size_t len;
    struct ing_notification notification;
    char param_pool[512] = { 0 };
    ntf_stat_t rescode;
    struct ntf_listener *thread_data;

    thread_data = (struct ntf_listener*)args;

    if ( thread_data->func == NULL )
    {
        ERR( "Listener thread function not found, close thread %s", thread_data->name);
        return NULL;
    }

    if ( thread_data->init != NULL )
        if ( thread_data->init( NULL ) != 0 )
        {
            ERR( "Listener data init failed, close thread %s", thread_data->name);
            return NULL;
        }

    ntf_handle = ing_listener_init( thread_data->port, 5 );
    if ( ntf_handle <= 0 )
    {
        ERR( "Init of listener %s failed", thread_data->name);
        return NULL;
    }
    len = sizeof( param_pool );

    for( ;; )
    {
        rescode = ing_notification_recv( ntf_handle, thread_data->name, 
                                         &notification, NTF_MSG_WAIT, param_pool, &len );
        if ( rescode == NTF_ST_OK )
        {
            thread_data->func( &notification );

            memset( param_pool, 0, sizeof( param_pool ) );
            len = sizeof( param_pool );
        }
    }

    if ( thread_data->clean != NULL )
        if ( thread_data->clean() != 0 )
            ERR( "Failed to clean %s thread data", thread_data->name);

    return NULL;
}
