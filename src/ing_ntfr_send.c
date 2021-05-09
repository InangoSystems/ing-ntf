/* ing_ntfr_send.c
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

/*
 * Global variables
 */
static int prnt_help = 0;           /* print help   */
static struct ing_notification tag; /* notification */

/*
 * Print help about usage command line parameters
 */
static void print_usage()
{
    printf( "nftrsend - little command line backend for manual notification sending\n"
            "\nParameters:\n"
            "\t-i, --id\tnotification ID (in digit format)\n"
            "\t-m, --module\tmodule ID\n"
            "\t-l, --severity\tnotification severity level\n"
            "\t-p, --parameter\tnotification parameter\n"
            "\t-h, --help\tdisplay this help\n"
            "\nExample:\n\tntfrsend -i 2 -m 0 -l 4 -p eth0 -p up -p down\n" );
}

/*
 * Proceed command line parameters
 */
static void proceed_input_args( int argc, char *argv[] )
{
    int need_exit, opt;
    const char options[] = ":i:m:l:p:h";
    static struct option longoptions[] = {
        { "id",        required_argument, NULL, 'i' },
        { "module",    required_argument, NULL, 'm' },
        { "severity",  required_argument, NULL, 'l' },
        { "parameter", required_argument, NULL, 'p' },
        { "help",      no_argument,       NULL, 'h' },
        { 0,           0,                 0,    0   }
    };

    need_exit = 0;

    while( ( opt = getopt_long( argc, argv,
                                options, longoptions, NULL ) ) != -1 )
    {
        switch ( opt )
        {
        case 0: /* getopt_long produce options value */
            break;
        case 'i': /* notification ID */
            tag.msg_id = atoi( optarg );
            break;
        case 'm': /* module ID */
            tag.module_id = atoi( optarg );
            break;
        case 'l': /* severity level */
            tag.severity = atoi( optarg );
            break;
        case 'p': /* notification parameter */
            tag.params[tag.param_num] = calloc( NTF_PARAM_LENGTH_MAX, 1 );
            strncpy( tag.params[tag.param_num], optarg, NTF_PARAM_LENGTH_MAX );
            ++tag.param_num;
            break;
        case 'h': /* need to print help */
        case ':':
        case '?':
        default:
            prnt_help = 1;
            need_exit  = 1;
            break;
        }
    }

    /* print usage note */
    if ( prnt_help != 0 )
        print_usage();
    /* exit if need */
    if ( need_exit != 0 )
        exit( 0 );
}

/*
 * Main application thread
 */
int main( int argc, char *argv[] )
{
    ntf_stat_t res;
    int i;
    tag.msg_id    = 0;
    tag.module_id = 0;
    tag.severity  = NTF_SEVERITY_NTF;
    tag.param_num = 0;

    proceed_input_args( argc, argv );

    res = ing_notification_send( &tag, 0 );
    switch ( res )
    {
    case NTF_ST_BAD_INPUT_PARAMS:
        printf( "Invalid input arguments\n" );
        return -1;
    case NTF_ST_OK:
        printf( "notification successfully sent\n" );
        break;
    default:
        printf( "notification failed to send\n" );
        break;
    }
    /* free allocated memory */
    for ( i = 0; i < tag.param_num; ++i )
        free( tag.params[i] );

    return 0;
}
