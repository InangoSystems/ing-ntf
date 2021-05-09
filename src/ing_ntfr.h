/* ing_ntfr.h
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
/* This fail contains API constant and functions for communicate with
 * notification system core
 */

#ifndef ING_NTFR_H
#define ING_NTFR_H

/*
 * Boolean value
 */
#define NTF_TRUE  1
#define NTF_FALSE 0

/*
 * Severity level
 */
#define NTF_SEVERITY_ALERT  1
#define NTF_SEVERITY_CRIT   2
#define NTF_SEVERITY_ERROR  3
#define NTF_SEVERITY_WRN    4
#define NTF_SEVERITY_NTF    5
#define NTF_SEVERITY_INFO   6
#define NTF_SEVERITY_DBG    7

/*
 * Notification flags for sending and receiving commands
 */
#define NTF_MSG_DONOTWAIT 1
#define NTF_MSG_WAIT      0

/*
 * Limiths
 */
#define NTF_PARAM_IN_MSG_MAX 16
#define NTF_PARAM_LENGTH_MAX 256

/*
 * Include messages defines
 */
#include "ing_ntfr_messages.h"

/*
 * Result code values
 */
enum ntf_result
{
    NTF_ST_OK = 0,
    NTF_ST_GENERAL_ERROR,
    NTF_ST_UNKNOWN_MSG_ID,
    NTF_ST_BAD_INPUT_PARAMS,
    NTF_ST_FAIL_NETWORK_OPERATION,
    NTF_ST_TIMEOUT,
    NTF_ST_NOMEMORY,
    NTF_ST_LAST
} ntf_result_t;
typedef unsigned long ntf_stat_t;

/*
 * Notification structure
 */
typedef struct ing_notification
{
    int msg_id;                         /* notification ID                */
    int module_id;                      /* module ID                      */
    int severity;                       /* severity level of notification */
    int param_num;                      /* parameter count                */
    char *params[NTF_PARAM_IN_MSG_MAX]; /* parameters of notification     */
} ntf_notification_t;

/*
 * Send notification
 *
 * Input:
 *  notif - pointer to notification structure
 *  flags - additional flags for notificator
 */
ntf_stat_t ing_notification_send( struct ing_notification *notif, int flags );

/*
 * Create listener handler
 *
 * Input:
 *  port    - value of listener port
 *  timeout - wait time (in seconds) for what will be block receiving operation
 *
 * Output:
 *  handle for listener
 */
int ing_listener_init( unsigned short port,
                       unsigned long timeout /* sec */ );

/*
 * Free listener handler
 *
 * Input:
 *  handle - listener handle for free
 */
void ing_listener_free( int handle );

/*
 * Receive notification
 *
 * Input:
 *  listener_handle - listener handle
 *  listener_name   - string containing name of the listener
 *  notif           - pointer to notification stricture for receiving
 *  flags           - flags for operation, it can be NTF_MSG_WAIT for block
 *                    receive operation for the time what was set in
 *                    listener initialization function
 *                    or
 *                    NTF_MSG_DONOTWAIT for immideatly return from function
 *                    even if do data was receive
 *  param_pool      - pool for storing parameters value
 *  pool_len        - length of parameter pool
 */
ntf_stat_t ing_notification_recv( int listener_handle, char *listener_name,
                                  struct ing_notification *notif, int flags,
                                  char *param_pool, size_t *pool_len );

#endif /* ING_NTFR_H */
