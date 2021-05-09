/* ing_ntfr_listener_syslog.c
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

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "ing_ntfr_defines.h"
#include "ing_ntfr_messages.h"
#include "ing_ntfr_listeners.h"
#include "ing_ntfr_settings.h"


#define SYSLOG_PORT 514


/* UDP socket to send syslog messages */
static int sockfd = -1;

/* syslog-server IP address */
static char syslog_address[16];


/*
 * Export database from auto-generated file
 */
struct ntf_syslog_db_entry ntf_syslog_db[NTF_MAX_DB_MESSAGE_NUM];

/*
 * Check if syslog-server address has changed.
 * Returns: zero if not changed, not zero if changed.
 * If address is changed, also copy new address to 'syslog_address' global varibale.
 */
static int update_syslog_address()
{
    char buf[16];
    int ret;

    ret = 0;

    if (!ntfsettings_get("syslog_srv", buf, sizeof(buf)))
    {
        if (strcmp(buf, syslog_address))
        {
            strcpy(syslog_address, buf);
            ret = 1;
        }
    }

    return ret;
}

/*
 * Create socket to access syslog server.
 * Socket is recreated if syslog server address has changed.
 */
static int update_socket()
{
    int ret;
    struct sockaddr_in addr;

    ret = 0;

    if (update_syslog_address())
    {
        /* syslog address has changed */
        if (sockfd != -1)
        {
            close(sockfd);
            sockfd = -1;
        }

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            ERR("socket() failed: %s (%d)", strerror(errno), errno);
            ret = 1;
            goto out;
        }

        update_syslog_address();

        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SYSLOG_PORT);
        addr.sin_addr.s_addr = inet_addr(syslog_address);

        if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
        {
            ERR("connect() failed: %s (%d)", strerror(errno), errno);
            close(sockfd);
            sockfd = -1;
            ret = 1;
            goto out;
        }
    }

out:
    return ret;
}

/*
 * Send a message directly to remote syslog-server.
 * The code is taken from logread.c file.
 */
static void send_syslog(int level, const char *format, ...)
{
    va_list ap;
    char buf[512];
    char *c;
    int p;
    size_t buflen;
    time_t t;

    va_start(ap, format);

    p = level;
    t = time(NULL);
    c = ctime(&t);

    snprintf(buf, sizeof(buf), "<%u>", p);
    strncat(buf, c + 4, 16);
/*
    if (hostname)
    {
        strncat(buf, hostname, sizeof(buf) - strlen(buf) - 1);
        strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
    }
    if (log_prefix)
    {
        strncat(buf, log_prefix, sizeof(buf) - strlen(buf) - 1);
        strncat(buf, ": ", sizeof(buf) - strlen(buf) - 1);
    }
*/
    buflen = strlen(buf);
    vsnprintf(buf + buflen, sizeof(buf) - buflen, format, ap);

    write(sockfd, buf, strlen(buf));

    va_end(ap);
}

/*
 *
 */
int ntf_syslog_init( void *args )
{
    return update_socket();
}

/*
 *
 */
int ntf_call_syslog( struct ing_notification *notif )
{
    int severity, i;

    switch( notif->severity )
    {
    case NTF_SEVERITY_ALERT:
        severity = LOG_ALERT;
        break;
    case NTF_SEVERITY_CRIT:
        severity = LOG_CRIT;
        break;
    case NTF_SEVERITY_ERROR:
        severity = LOG_ERR;
        break;
    case NTF_SEVERITY_WRN:
        severity = LOG_WARNING;
        break;
    case NTF_SEVERITY_NTF:
        severity = LOG_NOTICE;
        break;
    case NTF_SEVERITY_INFO:
        severity = LOG_INFO;
        break;
    case NTF_SEVERITY_DBG:
        severity = LOG_DEBUG;
        break;
    }

    /* Set syslog facility */
    severity |= LOG_USER;

    update_socket();

    for( i = 0 ; i < NTF_MAX_DB_MESSAGE_NUM; ++i )
    {
        if ( notif->msg_id == ntf_syslog_db[i].msg_id )
        {
            /* ToDo selfmade parsing for search %s ??? */
            switch( ntf_syslog_db[i].param_num )
            {
            case 1:
                send_syslog( severity, ntf_syslog_db[i].msg_text,
                        notif->params[ ntf_syslog_db[i].params[0].input_idx - 1 ] );
                break;
            case 2:
                send_syslog( severity, ntf_syslog_db[i].msg_text,
                        notif->params[ ntf_syslog_db[i].params[0].input_idx - 1 ],
                        notif->params[ ntf_syslog_db[i].params[1].input_idx - 1 ] );
                break;
            case 3:
                send_syslog( severity, ntf_syslog_db[i].msg_text,
                        notif->params[ ntf_syslog_db[i].params[0].input_idx - 1 ],
                        notif->params[ ntf_syslog_db[i].params[1].input_idx - 1 ],
                        notif->params[ ntf_syslog_db[i].params[2].input_idx - 1 ] );
                break;
            case 5:
                send_syslog( severity, ntf_syslog_db[i].msg_text,
                        notif->params[ ntf_syslog_db[i].params[0].input_idx - 1 ],
                        notif->params[ ntf_syslog_db[i].params[1].input_idx - 1 ],
                        notif->params[ ntf_syslog_db[i].params[2].input_idx - 1 ],
                        notif->params[ ntf_syslog_db[i].params[3].input_idx - 1 ],
                        notif->params[ ntf_syslog_db[i].params[4].input_idx - 1 ] );
                break;
            }
            break;
        }
    }

    return 0;
}

/*
 */
int ntf_syslog_clean()
{
    if (sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }

    return 0;
}
