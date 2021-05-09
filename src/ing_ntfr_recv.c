/* ing_ntfr_recv.c
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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "ing_ntfr.h"
#include "ing_ntfr_defines.h"

/*
 * Print help about usage command line parameters
 */
void print_usage()
{
    printf( "nftrrecv - little command line listener for Inango notification system\n"
            "\nParameters:\n"
            "\tnot required\n" );
}

/*
 * Proceed command line parameters
 */
void proceed_input_args( int argc, char *argv[] )
{
    int opt;
    const char options[] = ":h";
    static struct option longoptions[] = {
        { "help",      no_argument,       NULL, 'h' },
        { 0,           0,                 0,    0   }
    };

    while( ( opt = getopt_long( argc, argv,
                                options, longoptions, NULL ) ) != -1 )
    {
        switch ( opt )
        {
        case 0: /* getopt_long produce options value */
            break;
        case 'h': /* need to print help */
        case ':':
        case '?':
        default:
            print_usage();
            exit( 0 );
            break;
        }
    }
}

/*
 * Main application thread
 */
int main( int argc, char *argv[] )
{
    int ntf_handle, i;
    char **pstring;
    size_t len;
    struct ing_notification notification;
    char param_pool[512] = { 0 };

    proceed_input_args( argc, argv );

    ntf_handle = ing_listener_init( NTF_PORT_LISTENER_LOGGER, 5 );
    if ( ntf_handle < 0 )
    {
        printf( "socket create fail\n" );
        return -1;
    }
    len = sizeof( param_pool );

    for( ;; )
    {
        if ( ing_notification_recv( ntf_handle, "logger",
                                   &notification, NTF_MSG_WAIT,
                                   &param_pool[0], &len ) == NTF_ST_OK )
        {
            printf( "[notification]\n" );
            printf( "ID [%8i] from module [%8i] with severity [",
                     notification.msg_id,
                     notification.module_id );
            switch( notification.severity )
            {
            case NTF_SEVERITY_ALERT:
                printf( "\033[1;2;5;41m     ALERT    \033[0m" );
                break;
            case NTF_SEVERITY_CRIT:
                printf( "\033[1;2;41m   CRITICAL   \033[0m" );
                break;
            case NTF_SEVERITY_ERROR:
                printf( "\033[1;31;40m     ERROR    \033[0m" );
                break;
            case NTF_SEVERITY_WRN:
                printf( "\033[1;33;40m    WARNING   \033[0m" );
                break;
            case NTF_SEVERITY_NTF:
                printf( " NOTIFICATION " );
                break;
            case NTF_SEVERITY_INFO:
                printf( "  \033[1;2mINFORMATION\033[0m " );
                break;
            case NTF_SEVERITY_DBG:
                printf( "     \033[2mDEBUG\033[0m    " );
                break;
            default:
                printf( "    \033[1;2mUNKNOWN\033[0m   " );
            }
            printf( "]\nParameters:\n" );
            pstring = notification.params;
            for( i = 0; i < notification.param_num; ++i )
            {
                printf( "%3i: %s\n", i + 1, *pstring );
                ++pstring;
            }
            printf( "\n" );
            memset( param_pool, 0, sizeof( param_pool ) );
            len = sizeof( param_pool );
        }
    }

    return 0;
}
