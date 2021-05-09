/* ing_ntfr_settings.c
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
/* This file contain functions for read settings
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <pthread.h>

#include "ing_ntfr_defines.h"
#include "ing_ntfr_settings.h"

#define NTF_SETTINGS_KEY_LEN 32
#define NTF_SETTINGS_VALUE_LEN 64

#define NTF_SETTINGS_TBL_MAX 16

/*
 * Settings entry
 */
struct ntf_settings_entry
{
    char key[NTF_SETTINGS_KEY_LEN];
    char value[NTF_SETTINGS_VALUE_LEN];
} ntf_settings_entry_t;

/*
 * All settings of application
 */
struct ntf_settings
{
    int settings_num;
    pthread_mutex_t guard;
    struct ntf_settings_entry settings_table[NTF_SETTINGS_TBL_MAX];
} ntf_settings_t;

/*
 * Global variable
 */
struct ntf_settings settings;
config_t g_cfg;

/*
 * Initialize settings API
 */
int ntfsettings_init()
{
    memset( &settings, 0, sizeof( struct ntf_settings ) );
    pthread_mutex_init( &settings.guard, NULL );
    config_init( &g_cfg );
    if ( config_read_file( &g_cfg, NTF_CONF_FILE_NAME ) == CONFIG_FALSE )
        return -1;
    return 0;
}

/*
 * Free settings API handles
 */
void ntfsettings_free()
{
    config_destroy( &g_cfg );
}

/*
 * Update current settings
 */
int ntf_settings_update()
{
    int i;
    const char *pvalue;

    pthread_mutex_lock( &settings.guard );

    config_destroy( &g_cfg );

    config_init( &g_cfg );
    if ( config_read_file( &g_cfg, NTF_CONF_FILE_NAME ) == CONFIG_FALSE )
    {
        pthread_mutex_unlock( &settings.guard );
        return -1;
    }

    for ( i = 0; i < settings.settings_num; ++i )
    {
        if ( config_lookup_string( &g_cfg,
                                    settings.settings_table[i].key,
                                   &pvalue ) == CONFIG_TRUE )
        {
            if ( strcmp( settings.settings_table[i].value, pvalue ) != 0 )
            {
                LOG( "update value of %s; new value: %s (old value: %s)",
                      settings.settings_table[i].key,
                      pvalue,
                      settings.settings_table[i].value );
                strncpy( settings.settings_table[i].value,
                         pvalue, NTF_SETTINGS_VALUE_LEN );
            }
        }
    }

    ntfsettings_free();
    pthread_mutex_unlock( &settings.guard );

    return 0;
}

/*
 * Load 'key' from configuration file
 */
void ntfsettings_load( char key[] )
{
    const char *pvalue = NULL;
    size_t len;

    if ( config_lookup_string( &g_cfg, key, &pvalue ) == CONFIG_FALSE )
        return;

    len = strlen( pvalue );
    if ( len > 0 )
    {
        strncpy( settings.settings_table[settings.settings_num].key,
                 key, NTF_SETTINGS_KEY_LEN );
        strncpy( settings.settings_table[settings.settings_num].value,
                 pvalue, NTF_SETTINGS_VALUE_LEN );
        ( settings.settings_num )++;
    }
}

/*
 * Get 'key' value from settings
 */
int ntfsettings_get( char key[], char *param, size_t param_len )
{
    int i;
    size_t len;

    len = param_len;
    if ( len > NTF_SETTINGS_VALUE_LEN )
        len = NTF_SETTINGS_VALUE_LEN;

    for( i = 0; i < settings.settings_num; ++i )
    {
        if ( strcmp( key, settings.settings_table[i].key ) == 0 )
        {
            strncpy( param, settings.settings_table[i].value, len );
            return 0;
        }
    }

    return -1;
}
