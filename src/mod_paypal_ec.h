/*
 * mod_paypal_ec - Apache Module to secure URIs using PayPal Express Checkout
 * Copyright 2013 PayPal, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * 	   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MOD_PAYPAL_EC_H_
#define _MOD_PAYPAL_EC_H_

#define BUFSIZE 1024

#define AP_LOG_POOL_ERR(...) \
	ap_log_perror(APLOG_MARK, APLOG_ERR, __VA_ARGS__);	

#define AP_LOG_REQUEST_ERR(...) \
	ap_log_rerror(APLOG_MARK, APLOG_ERR, __VA_ARGS__); 

typedef struct {
   const char* path;
   const char* endPoint;
   const char* userName;
   const char* password;
   const char* signature;
   const char* version;
   const char* ecWebUrl;
   const char* ecIncontextUrl;
   const char* ecType;
   const char* appContext;
   const char* currency_code;
   apr_hash_t *book_data;
   int loaded;
}app_config;

static app_config config;

typedef struct {
    apr_pool_t* pool;
    char *ptr;
}string;

static int ec_handler(request_rec *r);
static void register_hooks(apr_pool_t *p);
static void parseInput(char* token, apr_table_t* data,apr_pool_t* pool);
static int validate(request_rec *r, apr_table_t* data);
static char* substring(const char* str, size_t begin, size_t len);
static int sendResponse(request_rec *r, const char* str);
static int setExpressCheckout(request_rec *r, apr_table_t* data);
static int doExpressCheckout(request_rec *r, apr_table_t* data);
static int getExpressCheckout(request_rec *r, apr_table_t* data);
static void redirectToPayPal(request_rec *r, apr_table_t* data);
static int sendFile(request_rec *r, apr_table_t *data);
static int authenticate_user(request_rec *r);
static void loadConfigFile(apr_pool_t *p, apr_hash_t *data);
static const char *app_config_path_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *api_endpoint_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *api_username_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *api_password_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *api_signature_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *api_version_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *api_currency_code(cmd_parms *cmd, void *cfg, const char *arg);
static const char *web_url_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *incontext_url_handler(cmd_parms *cmd, void *cfg, const char *arg);
static const char *ec_type_handler(cmd_parms *cmd, void *cfg, const char *arg);

#endif

