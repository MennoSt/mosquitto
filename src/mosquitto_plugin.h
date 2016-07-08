/*
Copyright (c) 2012-2016 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#ifndef MOSQUITTO_PLUGIN_H
#define MOSQUITTO_PLUGIN_H

#define MOSQ_AUTH_PLUGIN_VERSION 2

#define MOSQ_ACL_NONE 0x00
#define MOSQ_ACL_READ 0x01
#define MOSQ_ACL_WRITE 0x02

#include <stdbool.h>

struct mosquitto;

struct mosquitto_auth_opt {
	char *key;
	char *value;
};

struct mosquitto_acl_msg {
	const char *topic;
	const void *payload;
	long payloadlen;
	int qos;
	bool retain;
};

/*
 * To create an authentication plugin you must include this file then implement
 * the functions listed in the "Plugin Functions" section below. The resulting
 * code should then be compiled as a shared library. Using gcc this can be
 * achieved as follows:
 *
 * gcc -I<path to mosquitto_plugin.h> -fPIC -shared plugin.c -o plugin.so
 *
 * Authentication plugins can implement one or both of authentication and
 * access control. If your plugin does not wish to handle either of
 * authentication or access control it should return MOSQ_ERR_PLUGIN_DEFER. In
 * this case, the next plugin will handle it. If all plugins return
 * MOSQ_ERR_PLUGIN_DEFER, the request will be denied.
 *
 * For each check, the following flow happens:
 *
 * * The default password file and/or acl file checks are made. If either one
 *   of these is not defined, then they are considered to be deferred. If either
 *   one accepts the check, no further checks are made. If an error occurs, the
 *   check is denied
 * * The first plugin does the check, if it returns anything other than
 *   MOSQ_ERR_PLUGIN_DEFER, then the check returns immediately. If the plugin
 *   returns MOSQ_ERR_PLUGIN_DEFER then the next plugin runs its check.
 * * If the final plugin returns MOSQ_ERR_PLUGIN_DEFER, then access will be
 *   denied.
 */


/* =========================================================================
 *
 * Plugin Functions
 *
 * You must implement these functions in your plugin.
 *
 * ========================================================================= */

/*
 * Function: mosquitto_auth_plugin_version
 *
 * The broker will call this function immediately after loading the plugin to
 * check it is a supported plugin version. Your code must simply return
 * MOSQ_AUTH_PLUGIN_VERSION.
 */
int mosquitto_auth_plugin_version(void);

/*
 * Function: mosquitto_auth_plugin_init
 *
 * Called after the plugin has been loaded and <mosquitto_auth_plugin_version>
 * has been called. This will only ever be called once and can be used to
 * initialise the plugin.
 *
 * Parameters:
 *
 *	user_data :      The pointer set here will be passed to the other plugin
 *	                 functions. Use to hold connection information for example.
 *	auth_opts :      Pointer to an array of struct mosquitto_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int mosquitto_auth_plugin_init(void **user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count);

/*
 * Function: mosquitto_auth_plugin_cleanup
 *
 * Called when the broker is shutting down. This will only ever be called once.
 * Note that <mosquitto_auth_security_cleanup> will be called directly before
 * this function.
 *
 * Parameters:
 *
 *	user_data :      The pointer provided in <mosquitto_auth_plugin_init>.
 *	auth_opts :      Pointer to an array of struct mosquitto_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int mosquitto_auth_plugin_cleanup(void *user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count);

/*
 * Function: mosquitto_auth_security_init
 *
 * Called when the broker initialises the security functions when it starts up.
 * If the broker is requested to reload its configuration whilst running,
 * <mosquitto_auth_security_cleanup> will be called, followed by this function.
 * In this situation, the reload parameter will be true.
 *
 * Parameters:
 *
 *	user_data :      The pointer provided in <mosquitto_auth_plugin_init>.
 *	auth_opts :      Pointer to an array of struct mosquitto_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *	reload :         If set to false, this is the first time the function has
 *	                 been called. If true, the broker has received a signal
 *	                 asking to reload its configuration.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int mosquitto_auth_security_init(void *user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count, bool reload);

/* 
 * Function: mosquitto_auth_security_cleanup
 *
 * Called when the broker cleans up the security functions when it shuts down.
 * If the broker is requested to reload its configuration whilst running,
 * this function will be called, followed by <mosquitto_auth_security_init>.
 * In this situation, the reload parameter will be true.
 *
 * Parameters:
 *
 *	user_data :      The pointer provided in <mosquitto_auth_plugin_init>.
 *	auth_opts :      Pointer to an array of struct mosquitto_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *	reload :         If set to false, this is the first time the function has
 *	                 been called. If true, the broker has received a signal
 *	                 asking to reload its configuration.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int mosquitto_auth_security_cleanup(void *user_data, struct mosquitto_auth_opt *auth_opts, int auth_opt_count, bool reload);

/*
 * Function: mosquitto_auth_acl_check
 *
 * Called by the broker when topic access must be checked. access will be one
 * of MOSQ_ACL_READ (for subscriptions) or MOSQ_ACL_WRITE (for publish).
 *
 * Return:
 *	MOSQ_ERR_SUCCESS if access was granted.
 *	MOSQ_ERR_ACL_DENIED if access was not granted.
 *	MOSQ_ERR_UNKNOWN for an application specific error.
 *	MOSQ_ERR_PLUGIN_DEFER if your plugin does not wish to handle this check.
 */
int mosquitto_auth_acl_check(void *user_data, int access, const struct mosquitto *client, struct mosquitto_acl_msg *msg);

/*
 * Function: mosquitto_auth_unpwd_check
 *
 * Called by the broker when a username/password must be checked.
 *
 * Return:
 *	MOSQ_ERR_SUCCESS if the user is authenticated.
 *	MOSQ_ERR_AUTH if authentication failed.
 *	MOSQ_ERR_UNKNOWN for an application specific error.
 *	MOSQ_ERR_PLUGIN_DEFER if your plugin does not wish to handle this check.
 */
int mosquitto_auth_unpwd_check(void *user_data, const char *username, const char *password);

/*
 * Function: mosquitto_psk_key_get
 *
 * Called by the broker when a client connects to a listener using TLS/PSK.
 * This is used to retrieve the pre-shared-key associated with a client
 * identity.
 *
 * Examine hint and identity to determine the required PSK (which must be a
 * hexadecimal string with no leading "0x") and copy this string into key.
 *
 * Parameters:
 *	user_data :   the pointer provided in <mosquitto_auth_plugin_init>.
 *	hint :        the psk_hint for the listener the client is connecting to.
 *	identity :    the identity string provided by the client
 *	key :         a string where the hex PSK should be copied
 *	max_key_len : the size of key
 *
 * Return value:
 *	Return 0 on success.
 *	Return >0 on failure.
 *	Return MOSQ_ERR_PLUGIN_DEFER if your plugin does not wish to handle this check.
 */
int mosquitto_auth_psk_key_get(void *user_data, const char *hint, const char *identity, char *key, int max_key_len);

#endif
