/* ing_ntfr_listeners.h
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
/* This file contain prototypes of functions and database types for listeners
 * of notifier
 */
#ifndef ING_NTFR_LISTENERS_H
#define ING_NTFR_LISTENERS_H
#include <pthread.h>
/* Constants
 */
#define NTF_MAX_IFNAME_LEN 32 /* length of ifname                */
#define NTF_MAX_IFIDX_NUM 128           /* number of entry in ifIndex array */
#define NTF_MAX_DB_MESSAGE_NUM 64           /* number of entries in db message per listener */
/*
 * Conversion function type
 */
typedef int ( *ntf_param_convert_func )( char *pvalue_in, char *pvalue_out, void* );

/*
 * Listener function types
 */
typedef int ( *ntf_listener_init )( void *args );
typedef int ( *ntf_listener_func )( struct ing_notification *notif );
typedef int ( *ntf_validate_func )( struct ing_notification *notif );
typedef int ( *ntf_listener_clean )();

/*
 */
struct ntf_listener
{
    pthread_t          thread_id;
    char               name[16];
    unsigned short     port;
    ntf_listener_init  init;
    ntf_listener_func  func;
    ntf_listener_clean clean;
    int enabled;
} ntf_listener_t;

/*
 * Parameter database entry
 */
typedef struct ntf_param_convert
{
    int   input_idx;                     /*  */
    int   par_type;                      /*  */
    char *par_info;                      /*  */
    ntf_param_convert_func convert_func; /*  */
} ntf_param_convert_t;

/* ifIndex database entry from MMX-EP
 */
struct ntf_ifidx_db_entry
{
    char ifName[NTF_MAX_IFNAME_LEN];
    int  ifIndex;
    int  link_trap_enable;
} ntf_ifidx_db_entry_t;

/* **********************************************************
 *  SNMP
 * **********************************************************/
typedef struct ntf_snmp_db_entry
{
    int msg_id;

    char *trap_oid; /* enterprise trap oid */
    int trap_type; /* generic trap type (snmp v1) */
    ntf_validate_func  validate; /* function validate the need of sending the notification */
    int param_num;  /* number of variables in list */

    /* "varbind list" - specification(oid, type, conversion) all variables per notification */
    struct ntf_param_convert params[NTF_PARAM_IN_MSG_MAX]; 
} ntf_snmp_db_entry_t;

int ntf_snmp_init( void *args );
int ntf_snmp_clean( void *args );
int ntf_call_snmp_trap(struct ing_notification *notif );

/* **********************************************************
 *  Syslog
 * **********************************************************/
typedef struct ntf_syslog_db_entry
{
    int msg_id;     /*  */
    char *msg_text; /*  */
    int param_num;
    struct ntf_param_convert params[NTF_PARAM_IN_MSG_MAX];
} ntf_syslog_db_entry_t;

int ntf_syslog_init( void *args );
int ntf_call_syslog( struct ing_notification *notif );
int ntf_syslog_clean();

/* **********************************************************
 *  NETCONF
 * **********************************************************/
/*
 * Sample NETCONF notification
 * 
 * Either a common MMX NETCONF event notification <mmx-event/>
 * or some custom (specific) notification with arbitrary structure,
 * for example <download-operation-complete/>.
 *   
 *  MMX NETCONF event notification example is present below:
 *  (xmlns attribute is omitted, nodes <type/>, <content/>
 *  are mandatory):
 *
 *    <mmx-event>
 *      <type>sample-event</type>
 *      <content>
 *        <sample-event-param-1>foo</sample-event-param-1>
 *        <sample-event-param-2>bar</sample-event-param-2>
 *      </content>
 *    </mmx-event>
 */
typedef struct ntf_netconf_db_entry
{
    int msg_id;

    char  *template;  /* structure xml notification */
    char  *type;      /* value of node /mmx-event/type, only for <mmx-event> notification */
    ntf_validate_func  validate; /* function validate the need of sending the notification */
    int    param_num; /* number of component nodes under /mmx-event/content */

    /* content nodes (event parameters) under /mmx-event/content */
    struct ntf_param_convert  params[NTF_PARAM_IN_MSG_MAX];
} ntf_netconf_db_entry_t;

int ntf_netconf_init( void *args );
int ntf_netconf_clean();
int ntf_send_netconf_notif( struct ing_notification *notif );


/* **********************************************************
 *  MMX notifications
 * **********************************************************/
typedef struct ntf_mmx_msg_db_entry
{
    int msg_id;
    int mmx_req_type; /*  */
    int param_num;
    struct ntf_param_convert params[NTF_PARAM_IN_MSG_MAX];

} ntf_mmx_msg_db_entry_t;

int ntf_mmx_init( void *args );
int ntf_mmx_clean();
int ntf_send_mmx_notif( struct ing_notification *notif );

/*
 * Notification validation funcs
 */
int ntf_validate_link_trap_enable(struct ing_notification *notif);
/*
 * Parameters conversion funtions
 */
int ntf_ifName_to_ifIndex( char *pvalue_in, char *pvalue_out, void *arg );
int ntf_ifOperStatus_mmx_to_snmp( char *pvalue_in, char *pvalue_out, void *arg );
int ntf_ifOperStatus_mmx_to_yang( char *pvalue_in, char *pvalue_out, void *arg );
int ntf_ifAdminStatus_mmx_to_yang( char *pvalue_in, char *pvalue_out, void *arg );
int ntf_dfeFwLevel_mmx_to_yang( char *pvalue_in, char *pvalue_out, void *arg );
int ntf_datetime_libc2yang( char *pvalue_in, char *pvalue_out, void *arg );

/*
 * Management functions
 */
int ntf_ifidx_db_init();
void ntf_ifidx_db_deinit();
struct ntf_ifidx_db_entry* get_ifidx_first();

/*
 * Listener handler
 */
void* ntf_handler( void *args );

#endif // ING_NTFR_LISTENERS_H
