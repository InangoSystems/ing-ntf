/* ing_ntfr_listeners_data.c
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
/* This file contain databases for listeners
 */

/* This is automatically generated file  */

#include <stddef.h>
#include <pthread.h>

#include "ing_ntfr_defines.h"
#include "ing_ntfr_messages.h"
#include "ing_ntfr_listeners.h"

#define NCNTF_MMXEVENT_TEMPLATE \
    "<mmx-event>" \
        "<type/>" \
        "<content/>" \
    "</mmx-event>"

#define NCNTF_DOWNLOAD_OP_COMPLETE_TEMPLATE \
    "<download-operation-complete>" \
    "</download-operation-complete>"


struct ntf_snmp_db_entry ntf_snmp_db[] = {
    {
        NTF_MSG_LINKDOWN,
        ".1.3.6.1.6.3.1.1.5.3",
        2,
        &ntf_validate_link_trap_enable,
        3, { { 1, NTF_TYPE_INT, ".1.3.6.1.2.1.2.2.1.1", &ntf_ifName_to_ifIndex },
             { 2, NTF_TYPE_INT, ".1.3.6.1.2.1.2.2.1.7", &ntf_ifOperStatus_mmx_to_snmp },
             { 3, NTF_TYPE_INT, ".1.3.6.1.2.1.2.2.1.8", NULL } }
    },
    {
        NTF_MSG_LINKUP,
        ".1.3.6.1.6.3.1.1.5.4",
        3,
        &ntf_validate_link_trap_enable,
        3, { { 1, NTF_TYPE_INT, ".1.3.6.1.2.1.2.2.1.1", &ntf_ifName_to_ifIndex },
             { 2, NTF_TYPE_INT, ".1.3.6.1.2.1.2.2.1.7", &ntf_ifOperStatus_mmx_to_snmp },
             { 3, NTF_TYPE_INT, ".1.3.6.1.2.1.2.2.1.8", NULL } }
    },
    /* This element serves as a stop-element to prevent iterating beyond the end */
    {
        NTF_MSG_NOTUSED
    }
};

struct ntf_syslog_db_entry ntf_syslog_db[] = {
    {
        NTF_MSG_NTFRDEBUG1,
        "dbg notification: %s",
        1, { { 1, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_NTFRDEBUG2,
        "dbg notification: %s %s",
        2, { { 1, NTF_TYPE_STR, "string", NULL },
             { 2, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_LINKDOWN,
        "link %s is down",
        1, { { 1, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_LINKUP,
        "link %s is up",
        1, { { 1, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_DFEDISCOVER,
        "DFE %s is discovered (fwLevel %s, macAddr %s)",
        3, { { 1, NTF_TYPE_INT, "string", NULL },
             { 2, NTF_TYPE_INT, "string", NULL },
             { 3, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_DFELOST,
        "DFE %s is lost",
        1, { { 1, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_MMXMODULESTARTED,
        "MMX module '%s' successfully started",
        1, { { 1, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_MMXMODULEINITFAILED,
        "MMX module '%s' init failed",
        1, { { 1, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_MMXOBJCHANGED,
        "MMX Object '%s' changed",
        1, { { 1, NTF_TYPE_STR, "string", NULL } }
    },
    {
        NTF_MSG_COPY_OP_COMPLETE,
        "MMX copy operation complete (operation-id %s, start %s, complete %s, status %s, error %s)",
        5, { { 1, NTF_TYPE_INT, "operation-id", NULL },
             { 2, NTF_TYPE_INT, "start-time", NULL },
             { 3, NTF_TYPE_INT, "complete-time", NULL },
             { 4, NTF_TYPE_INT, "operation-state", NULL },
             { 5, NTF_TYPE_STR, "error-log", NULL } }
    }
};

struct ntf_netconf_db_entry ntf_netconf_db[] = {
    {
        NTF_MSG_LINKDOWN,
        NCNTF_MMXEVENT_TEMPLATE,
        "link-down",
        &ntf_validate_link_trap_enable,
        3, { { 1, NTF_TYPE_STR, "if-name", NULL },
             { 2, NTF_TYPE_STR, "admin-status", &ntf_ifAdminStatus_mmx_to_yang },
             { 3, NTF_TYPE_STR, "oper-status", &ntf_ifOperStatus_mmx_to_yang } }
    },
    {
        NTF_MSG_LINKUP,
        NCNTF_MMXEVENT_TEMPLATE,
        "link-up",
        &ntf_validate_link_trap_enable,
        3, { { 1, NTF_TYPE_STR, "if-name", NULL },
             { 2, NTF_TYPE_STR, "admin-status", &ntf_ifAdminStatus_mmx_to_yang },
             { 3, NTF_TYPE_STR, "oper-status", &ntf_ifOperStatus_mmx_to_yang } }
    },
    {
        NTF_MSG_DFEDISCOVER,
        NCNTF_MMXEVENT_TEMPLATE,
        "dfe-discovered",
        NULL,
        3, { { 1, NTF_TYPE_INT, "chip-id", NULL },
             { 2, NTF_TYPE_INT, "fw-level", &ntf_dfeFwLevel_mmx_to_yang },
             { 3, NTF_TYPE_STR, "mac-address", NULL } }
    },
    {
        NTF_MSG_DFELOST,
        NCNTF_MMXEVENT_TEMPLATE,
        "dfe-lost",
        NULL,
        1, { { 1, NTF_TYPE_INT, "chip-id", NULL } }
    },
    {
        NTF_MSG_COPY_OP_COMPLETE,
        NCNTF_DOWNLOAD_OP_COMPLETE_TEMPLATE,
        "download-operation-complete",
        NULL,
        5, { { 1, NTF_TYPE_INT, "operation-id", NULL },
             { 2, NTF_TYPE_INT, "start-time", &ntf_datetime_libc2yang },
             { 3, NTF_TYPE_INT, "complete-time", &ntf_datetime_libc2yang },
             { 4, NTF_TYPE_INT, "operation-state", NULL },
             { 5, NTF_TYPE_STR, "error-log", NULL } }
    },
#if 0
    {
        NTF_MSG_NTFRDEBUG1,
        NCNTF_MMXEVENT_TEMPLATE,
        "debug",
        NULL,
        1, { { 1, NTF_TYPE_STR, "message", NULL } }
    },
    {
        NTF_MSG_NTFRDEBUG2,
        NCNTF_MMXEVENT_TEMPLATE,
        "debug",
        NULL,
        2, { { 1, NTF_TYPE_STR, "message", NULL },
             { 2, NTF_TYPE_STR, "", NULL } } /* the node has par_info (3rd param - empty-string) */
                                             /* equal to the prev node par_info ("message")      */
    },
#endif
    /* This element serves as a stop-element to prevent iterating beyond the end */
    {
        NTF_MSG_NOTUSED
    }
};

ntf_mmx_msg_db_entry_t ntf_mmx_msg_db[] = {
    {
        NTF_MSG_MMXMODULESTARTED,
        10,  /* MSGTYPE_DISCOVERCONFIG, */
        1, { { 1, NTF_TYPE_STR, "mmx-module-name", NULL } }
    },
    {
        NTF_MSG_MMXOBJCHANGED,
        10,  /* MSGTYPE_DISCOVERCONFIG, */
         1, { { 1, NTF_TYPE_STR, "mmx-obj-name", NULL } }
    },
    /* This element serves as a stop-element to prevent iterating beyond the end */
    {
        NTF_MSG_NOTUSED
    }
};
