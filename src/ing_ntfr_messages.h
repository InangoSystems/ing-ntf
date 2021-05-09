/* ing_ntfr_messages.h
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
/* This file contain constants for notification ID,
 * predefined in notification system
 */

/* This is automatically generated file  */

#ifndef ING_NTFR_MESSAGES_H
#define ING_NTFR_MESSAGES_H

#if 0

/*
 * Notification parameter type
 */
typedef struct ntf_msgdb_param_type
{
    int type;                         /* data type of parameter */
    char name[NTF_STR_PARAMNAME_MAX]; /* name of parameter      */
} ntf_msgdb_param_type_t;

/*
 * Message database entry
 */
typedef struct ntf_msgdb_entry
{
    int id;                         /* notification ID */
    char name[NTF_STR_MSGNAME_MAX]; /* notification name */
    int param_num;                  /* parameters count */
    struct ntf_msgdb_param_type params[NTF_PARAM_IN_MSG_MAX]; /* type of notification parameters */
} ntf_msgdb_entry_t;

#endif

/* Denotes a message ID for internal usage */
#define NTF_MSG_NOTUSED 0

#define NTF_MSG_NTFRDEBUG1 1
/* { NTF_MSG_NTFRDEBUG1, "DebugMsg1",
 *     1, { { NTF_TYPE_STR, "Param" } }
 * }
 */

#define NTF_MSG_NTFRDEBUG2 2
/* { NTF_MSG_NTFRDEBUG2, "DebugMsg2",
 *     2, { { NTF_TYPE_STR, "Param1" },
 *          { NTF_TYPE_INT, "Param2" } }
 * }
 */

#define NTF_MSG_LINKDOWN 3
/* { NTF_MSG_LINKDOWN, "LinkDown",
 *     3, { { NTF_TYPE_STR, "ifaceName"   },
 *          { NTF_TYPE_INT, "AdminStatus" },
 *          { NTF_TYPE_INT, "OperStatus"  } }
 * }
 */

#define NTF_MSG_LINKUP 4
/* { NTF_MSG_LINKUP, "LinkUp",
 *     3, { { NTF_TYPE_STR, "ifaceName"   },
 *          { NTF_TYPE_INT, "AdminStatus" },
 *          { NTF_TYPE_INT, "OperStatus"  } }
 * }
 */

// TODO not currently used message type.
#define NTF_MSG_COPY_OP_START 5
/* { NTF_MSG_COPY_OP_START, "",
 *     3, { { NTF_TYPE_INT, "operation-id"   },
 *          { NTF_TYPE_STR, "start-time" },
 *          { NTF_TYPE_STR, "operation-state"  } }
 * }
 */

#define NTF_MSG_COPY_OP_COMPLETE 6
/* { NTF_MSG_COPY_OP_COMPLETE, "",
 *     5, { { NTF_TYPE_INT, "operation-id"   },
 *          { NTF_TYPE_STR, "start-time" },
 *          { NTF_TYPE_STR, "complete-time" },
 *          { NTF_TYPE_STR, "operation-state" },
 *          { NTF_TYPE_STR, "error-log"  } }
 * }
 */

#define NTF_MSG_DFEDISCOVER 1000
/* { NTF_MSG_DFEDISCOVER, "DFEDiscovered",
 *     3, { { NTF_TYPE_INT, "chipID"  },
 *          { NTF_TYPE_INT, "fwLevel" },
 *          { NTF_TYPE_STR, "macaddr" } }
 * }
 */

#define NTF_MSG_DFELOST 1001
/* { NTF_MSG_DFELOST, "DFELost",
 *     1, { { NTF_TYPE_INT, "chipID" } }
 * }
 */

#define NTF_MSG_MMXMODULESTARTED 201
/* { NTF_MSG_MMXMODULESTARTED, "MMXModuleStarted",
 *     1, { { NTF_TYPE_STR, "moduleName" } }
 * }
 */

#define NTF_MSG_MMXMODULEINITFAILED 202
/* { NTF_MSG_MMXMODULEINITFAILED, "MMXModuleInitFailed",
 *     1, { { NTF_TYPE_STR, "moduleName" } }
 * }
 */

#define NTF_MSG_MMXOBJCHANGED 203
/* { NTF_MSG_MMXOBJCHANGED, "MMXObjChanged",
 *     1, { { NTF_TYPE_STR, "objName" } }
 * }
 */

#endif /* ING_NTFR_MESSAGES_H */
