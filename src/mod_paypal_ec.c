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

#include <httpd.h>
#include <http_config.h>
#include <http_core.h>
#include <http_log.h>
#include <mod_auth.h>
#include <curl/curl.h>
#include <apr_strings.h>
#include "mod_paypal_ec.h"

static const command_rec ec_directives[] = {
   AP_INIT_TAKE1("Pricelist", app_config_path_handler, NULL, OR_AUTHCFG, "Pricelist for books"),
   AP_INIT_TAKE1("ApiEndPoint", api_endpoint_handler, NULL, RSRC_CONF, "PayPal endpoint for API call"),
   AP_INIT_TAKE1("ApiUserName", api_username_handler, NULL, RSRC_CONF, "User name for API call"),
   AP_INIT_TAKE1("ApiPassword", api_password_handler, NULL, RSRC_CONF, "API Password"),
   AP_INIT_TAKE1("ApiSignature", api_signature_handler, NULL, RSRC_CONF, "API signature"),
   AP_INIT_TAKE1("ApiVersion", api_version_handler, NULL, RSRC_CONF, "API version"),
   AP_INIT_TAKE1("CurrencyCode", api_currency_code, NULL, RSRC_CONF, "Currency Code"),
   AP_INIT_TAKE1("ExpressCheckoutWebUrl", web_url_handler, NULL, RSRC_CONF, "URL for EC call"),
   AP_INIT_TAKE1("ExpressCheckoutInContextUrl", incontext_url_handler, NULL, RSRC_CONF, "Incontext URL"),
   AP_INIT_TAKE1("ExpressCheckoutType", ec_type_handler, NULL, RSRC_CONF, "Type of EC"),
   {NULL}
};

static void loadConfigFile(apr_pool_t *pool, apr_hash_t *data) 
{
	apr_file_t *fd;
	apr_status_t rv;
	char line[BUFSIZE]; 
	char* key ;
	char* value;
	apr_hash_index_t *index;
	char* last;
	apr_ssize_t len ;
	const char* delim = "=\n\r";
	
	rv = apr_file_open(&fd, config.path,APR_READ,APR_OS_DEFAULT, pool);
	if (rv != APR_SUCCESS) 
	{
		AP_LOG_POOL_ERR(0, pool,"can't open %s", config.path);
		return;
	}      
		   
	while ( apr_file_gets (line, BUFSIZE,fd) == APR_SUCCESS ) 
	{
		key = apr_strtok(line, delim, &last);
		if(key != NULL) 
		{
			value = apr_strtok(NULL,delim, &last);
		}
		apr_hash_set(data, apr_pstrdup(pool,key), APR_HASH_KEY_STRING, apr_pstrdup(pool,value));
	}
	apr_file_close(fd);
	config.loaded = 1;
}

static const char *app_config_path_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.path = arg;
    if (config.loaded != 1)
    {
        config.book_data = apr_hash_make(cmd->pool);
        loadConfigFile(cmd->pool, config.book_data);
    }
    return NULL;
}

static const char *api_endpoint_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.endPoint= arg;
    return NULL;
}

static const char *api_username_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.userName= arg;
    return NULL;
}

static const char *api_password_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.password= arg;
    return NULL;
}

static const char *api_signature_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.signature= arg;
    return NULL;
}

static const char *api_version_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.version= arg;
    return NULL;
}

static const char *api_currency_code(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.currency_code = arg;
    return NULL;
}

static const char *web_url_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.ecWebUrl= arg;
    return NULL;
}

static const char *incontext_url_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.ecIncontextUrl= arg;
    return NULL;
}

static const char *ec_type_handler(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.ecType = arg;
    return NULL;
}


static int authenticate_user(request_rec *r)
{
    const char *authtype;
    AP_LOG_REQUEST_ERR(0, r, "Begin Authenticate_user");
    authtype = ap_auth_type(r);
    if (!authtype || apr_strnatcasecmp(authtype, "ExpressCheckout")) 
    {
        return DECLINED;
    }
    /* We need an authentication realm. */
    if (!ap_auth_name(r)) 
    {
	AP_LOG_REQUEST_ERR(0, r, "need AuthName: %s", r->uri);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    r->ap_auth_type = "ExpressCheckout";
    return ec_handler(r);
}

static int ec_handler(request_rec *r)
{
    char *uri = r->unparsed_uri;
    AP_LOG_REQUEST_ERR(0, r, "Begin ec_handler");
    AP_LOG_REQUEST_ERR(0, r, "the_request=%s",r->the_request);
    AP_LOG_REQUEST_ERR(0, r, "hostname=%s",r->hostname);
    AP_LOG_REQUEST_ERR(0, r, "handler = %s",r->handler);
    config.appContext = ap_construct_url(r->pool, r->unparsed_uri, r);

    if (r->method_number != M_GET)
    {
        return HTTP_METHOD_NOT_ALLOWED;
    }
    apr_table_t*  data = apr_table_make(r->pool, 100000);
    parseInput(uri, data, r->pool);
    int status =0;
    const char* resource_name = apr_table_get(data, "NAME");
    char* amt = apr_hash_get(config.book_data, resource_name, APR_HASH_KEY_STRING);
    if(amt == NULL)
    {
        AP_LOG_REQUEST_ERR(0, r, "Requested resource is not available!");
        const char* errorMsg = "Requested resource is not available!";
        sendResponse(r, errorMsg);
        return HTTP_UNAUTHORIZED;
    }
    apr_table_addn(data, "AMOUNT", amt);
    const char* token = apr_table_get(data, "token");
    const char* statusStr = apr_table_get(data, "status");
    const char* payerId = apr_table_get(data, "PayerID");
    if(token == NULL && statusStr == NULL)
    {
        status = validate(r, data);
        if(status > 0) 
        {
            AP_LOG_REQUEST_ERR(0, r, "Invalid URL!!!. Hence returning the Error Page with the status %d", status);
            return HTTP_UNAUTHORIZED;
	}
	status = setExpressCheckout(r, data);
	if(status > 0) 
	{
	    AP_LOG_REQUEST_ERR(0, r, "PayPal SetExpressCheckout API has been failed with the status %d", status);
	    return HTTP_UNAUTHORIZED;
	}
	redirectToPayPal(r, data);
	return HTTP_MOVED_TEMPORARILY;
    } 
    else if(statusStr != NULL && apr_strnatcasecmp(statusStr, "cancel") == 0) 
    {
	AP_LOG_REQUEST_ERR(0, r, "Payment has been cancelled by the user.");
	const char* errorMsg = "Payment has been cancelled by the user. Make the payment via PayPal to download.";
	sendResponse(r, errorMsg);
	return HTTP_UNAUTHORIZED;
    } 
    else if(token != NULL && strlen(token) > 0 && payerId != NULL && strlen(payerId) > 0 && statusStr != NULL && apr_strnatcasecmp(statusStr, "ok") == 0) 
    {
	status = getExpressCheckout(r, data);
	if(status > 0) 
	{
	    const char* errorMsg = "This token was already processed and it can not be used any more. Download can be succcess only when you make new payment via PayPal.";
	    sendResponse(r, errorMsg);
	    return HTTP_UNAUTHORIZED;
	}
	status = doExpressCheckout(r, data);
	if(status > 0) 
	{
	    const char* errorMsg = "Payment has been failed. Download can be succcess only when you make the payment via PayPal.";
	    sendResponse(r, errorMsg);
	    return HTTP_UNAUTHORIZED;
	}
	status = sendFile(r, data);
	if(status > 0) 
	{
	    return HTTP_INTERNAL_SERVER_ERROR;
	}
	return OK;
    }  
    else 
    {
        AP_LOG_REQUEST_ERR(0, r, "Unable to process the request. Invalid URL .");
	const char* errorMsg = "Unable to process the request. Invalid URL!!!";
	sendResponse(r, errorMsg);
	return HTTP_UNAUTHORIZED;
    }
} 

static int sendFile(request_rec *r, apr_table_t *data) {
   int status = 0;
   apr_file_t *fd;
   apr_size_t sz;
   apr_status_t rv;
   if (r->filename == NULL) {
      AP_LOG_REQUEST_ERR(0, r,"Incomplete request_rec!");
      status = 1;
      return status;
   }
   char* ext = ap_strrchr_c(r->filename, '.') ;
   if(apr_strnatcasecmp(ext, ".pdf") ==0) {
      AP_LOG_REQUEST_ERR(0, r, "Downloading pdf file");
      ap_set_content_type(r, "application/pdf");
   } else {
      ap_set_content_type(r, "application/pdf");
   }
   ap_set_content_length(r, r->finfo.size);
   if (r->finfo.mtime) {
               char *datestring = apr_palloc(r->pool, APR_RFC822_DATE_LEN);
               apr_rfc822_date(datestring, r->finfo.mtime);
               apr_table_setn(r->headers_out, "Last-Modified", datestring);
   }
   rv = apr_file_open(&fd, r->filename,
                        APR_READ|APR_SHARELOCK|APR_SENDFILE_ENABLED,
                        APR_OS_DEFAULT, r->pool);
   if (rv != APR_SUCCESS) {
	AP_LOG_REQUEST_ERR(0, r,"can't open %s", r->filename);
      return HTTP_NOT_FOUND ;
   }
   ap_send_fd(fd, r, 0, r->finfo.size, &sz);
   apr_file_close(fd);
   return status;
}


static int sendResponse(request_rec *r, const char* str) {
   ap_set_content_type(r, "text/html");
   ap_rputs("<HTML> <HEAD><TITLE>Error</TITLE></HEAD><BODY>", r);
   ap_rputs("<strong align=\"center\">", r);
   ap_rputs(str, r);
   ap_rputs("</strong></BODY></HTML>", r);
   return 0;
}


static void redirectToPayPal(request_rec *r, apr_table_t* data) 
{
   const char* token = apr_table_get(data, "TOKEN");
   const char* weburl;
   if(apr_strnatcasecmp(config.ecType, "Basic") ==0) 
   {
	weburl = apr_pstrcat(r->pool, config.ecWebUrl, token, NULL);
   } 
   else if(apr_strnatcasecmp(config.ecType, "InContext") == 0) 
   {
	weburl = apr_pstrcat(r->pool, config.ecIncontextUrl,token, NULL);
   }
   AP_LOG_REQUEST_ERR(0, r, "weburl = %s", weburl);
   apr_table_setn(r->err_headers_out, "Location", weburl);
}

static int validate(request_rec *r, apr_table_t* data) 
{
  const char* amt = apr_table_get(data, "AMOUNT");
  const char* name = apr_table_get(data, "NAME"); 
  int status = 0;
  if(amt == NULL  || strlen(amt) == 0) 
  {
     status = 1;
  }
  if(name == NULL || strlen(name) == 0) 
  {
     status = 2;
  }
  if(status > 0)
  {
     ap_set_content_type(r, "text/html");
     ap_rputs("<HTML> <HEAD><TITLE>Error</TITLE></HEAD><BODY>", r);
     ap_rputs("<strong align=\"center\">", r);
     ap_rputs("Invalid Download URL either item name or amount is missing", r);
     ap_rputs("</strong></BODY></HTML>", r);
  }
  return status;
}

static void parseInput(char* token, apr_table_t* data, apr_pool_t* pool) 
{
   int i = 0;
   int start = 0;
   char* name;
   char* key;
   char* value;
   int nameFound = 1;
   while(token[i] != '\0') 
   {
     i++;
     if(token[i] == '/') {
       start = i +1;
       continue;
     }
     if(token[i] == '?') {
       apr_table_setn(data, "NAME",apr_pstrmemdup(pool,token+start, (i - start)));
       nameFound = 0;
       start = i+1;
       continue;
     }
     if(token[i] == '=') {
       key = apr_pstrmemdup(pool,token+start, (i - start));
       start = i+1;
       continue;
     }
     if(token[i] == '&') {
       value = apr_pstrmemdup(pool,token+start, (i - start));
       start = i+1;
       apr_table_setn(data, key, value);
     }
   }
   if(nameFound != 0) {
      key = "NAME";
   } 
   if(key != NULL && strcmp(key, "AMOUNT") != 0) {
      value = apr_pstrmemdup(pool,token+start, (i - start));
      apr_table_setn(data, key, value);
   }
} 

static size_t writefunc(void *ptr, size_t size, size_t nmemb, string *s)
{
  s->ptr = apr_pstrmemdup(s->pool, ptr, size*nmemb);
  return size*nmemb;
}

static char* populateApiCredential(apr_pool_t* pool) {
   return apr_pstrcat(pool, config.endPoint, "?USER=", config.userName,"&PWD=",config.password, "&SIGNATURE=", config.signature, "&VERSION=", config.version,NULL);
}

static void  populateSetExpressCheckoutURL(char** ptr, apr_table_t* data,apr_pool_t* pool) {
   const char* amt = apr_table_get(data, "AMOUNT");
   const char* name = apr_table_get(data, "NAME");

   *ptr = apr_pstrcat(pool, populateApiCredential(pool),"&METHOD=SetExpressCheckout&","RETURNURL=", config.appContext, "?status=ok", "&CANCELURL=", config.appContext,"?status=cancel", "&PAYMENTREQUEST_0_AMT=", amt,"&PAYMENTREQUEST_0_CURRENCYCODE=",config.currency_code ,"&PAYMENTREQUEST_0_ITEMAMT=", amt,"&PAYMENTREQUEST_0_PAYMENTACTION=Sale&L_PAYMENTREQUEST_0_NAME0=", name, "&L_PAYMENTREQUEST_0_AMT0=", amt, "&L_PAYMENTREQUEST_0_ITEMCATEGORY0=Digital","&REQCONFIRMSHIPPING=0","&NOSHIPPING=1","&L_PAYMENTREQUEST_0_QTY0=1",NULL); 
}

static void  populateDoExpressCheckoutURL(char** ptr, apr_table_t* data,apr_pool_t* pool) {
   const char* amt = apr_table_get(data, "AMOUNT");
   const char* name = apr_table_get(data, "NAME");
   const char* token = apr_table_get(data, "token");
   const char* payerId = apr_table_get(data, "PayerID");
   *ptr = apr_pstrcat(pool, populateApiCredential(pool),"&METHOD=DoExpressCheckoutPayment&", "TOKEN=", token, "&PAYERID=", payerId, "&PAYMENTREQUEST_0_AMT=",amt, "&PAYMENTREQUEST_0_ITEMAMT=", amt, "&PAYMENTREQUEST_0_PAYMENTACTION=Sale&L_PAYMENTREQUEST_0_NAME0=", name, "&L_PAYMENTREQUEST_0_AMT0=", amt, "&L_PAYMENTREQUEST_0_ITEMCATEGORY0=Digital","&L_PAYMENTREQUEST_0_QTY0=1",NULL); 
}

static void  populateGetExpressCheckoutURL(char** ptr, apr_table_t* data,apr_pool_t* pool) {
   const char* token = apr_table_get(data, "token");
   *ptr = apr_pstrcat(pool, populateApiCredential(pool),"&METHOD=GetExpressCheckoutDetails&", "TOKEN=", token,NULL);
}

static int doExpressCheckout(request_rec *r, apr_table_t* data) {
 int status = 0;
    CURL *curl;
    CURLcode res;
    AP_LOG_REQUEST_ERR(0, r, "Begin DoExpressCheckout.");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl)
    {
        string s;
        char* ptr;
        populateDoExpressCheckoutURL(&ptr, data,r->pool);
        AP_LOG_REQUEST_ERR(0, r, "Do EC request = %s",ptr);
	s.pool = r->pool;
        curl_easy_setopt(curl, CURLOPT_URL, ptr);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) 
        {
	    AP_LOG_REQUEST_ERR(0, r, "DoExpressCheckout curl_easy_perform() failed: %s, %d\n",curl_easy_strerror(res), res);
            status = 1;
        } 
        else 
	{
           AP_LOG_REQUEST_ERR(0, r, "response = %s",s.ptr);
           char* line = curl_unescape(s.ptr, strlen(s.ptr));
           parseInput(line, data,r->pool);
	   AP_LOG_REQUEST_ERR(0, r, "DoExpressCheckout APU response is %s", s.ptr);
           const char* ack = apr_table_get(data, "ACK");
	   AP_LOG_REQUEST_ERR(0, r, "DoExpressCheckout ACK status %s", ack);
           if(apr_strnatcasecmp(ack, "Success") == 0) 
           {
	       AP_LOG_REQUEST_ERR(0, r, "DoExpressCheckout ACK status %s", ack);
               const char* transactionId = apr_table_get(data, "PAYMENTINFO_0_TRANSACTIONID");
           } 
           else 
           {
               AP_LOG_REQUEST_ERR(0, r, "DoExpressCheckout API has been failed: %s", s.ptr);
               status = 3;
           }
        }
        curl_easy_cleanup(curl);
    } 
    else
    {
	AP_LOG_REQUEST_ERR(0, r, "DoExpressCheckout CURL is false.");
        status = 2;
    }
    curl_global_cleanup();
    AP_LOG_REQUEST_ERR(0, r, "DoExpressCheckout API has been completed.");
    return status;
}

static int getExpressCheckout(request_rec *r, apr_table_t* data) {
 int status = 0;
    CURL *curl;
    CURLcode res;
	AP_LOG_REQUEST_ERR(0, r, "GetExpressCheckout API has been started.");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        string s;
        char* ptr;
        populateGetExpressCheckoutURL(&ptr, data,r->pool);
        AP_LOG_REQUEST_ERR(0, r, "Get EC request = %s",ptr);
	s.pool = r->pool;
        curl_easy_setopt(curl, CURLOPT_URL, ptr);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
		AP_LOG_REQUEST_ERR(0, r, "GetExpressCheckout curl_easy_perform() failed: %s, %d\n",curl_easy_strerror(res), res);
            status = 1;
        } else {
	   AP_LOG_REQUEST_ERR(0, r, "REsponse = %s",s.ptr);
           char* line = curl_unescape(s.ptr, strlen(s.ptr));
           parseInput(line, data,r->pool);
           const char* ack = apr_table_get(data, "ACK");
           if(apr_strnatcasecmp(ack, "Success") == 0) {
              const char* checkoutStatus = apr_table_get(data, "CHECKOUTSTATUS");
		AP_LOG_REQUEST_ERR(0, r, "GetExpressCheckout API checkout status %s, %s", checkoutStatus, s.ptr);
              if(apr_strnatcasecmp(checkoutStatus, "PaymentCompleted") == 0 || apr_strnatcasecmp(checkoutStatus, "PaymentActionCompleted") == 0) { 
                  status = 3;
		AP_LOG_REQUEST_ERR(0, r, "This token was already used, can't make DoExpressCheckout call again");
              } else if(apr_strnatcasecmp(checkoutStatus, "PaymentActionNotInitiated") == 0){
                  status = 0;
              } 
           } else {
		AP_LOG_REQUEST_ERR(0, r, "GetExpressCheckout API has been failed: %s", s.ptr);
              status = 0;
           }
        }
        curl_easy_cleanup(curl);
    } else {
	AP_LOG_REQUEST_ERR(0, r, "GetExpressCheckout CURL is false.");
        status = 2;
    }
    curl_global_cleanup();
	AP_LOG_REQUEST_ERR(0, r, "GetExpressCheckout API has been completed.");
    return status;
}

static int setExpressCheckout(request_rec *r, apr_table_t* data) {
    int status = 0;
    CURL *curl;
    CURLcode res;
    AP_LOG_REQUEST_ERR(0, r, "Begin SetExpressCheckout.");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        string s; 
        char* ptr;
        populateSetExpressCheckoutURL(&ptr, data,r->pool);
        AP_LOG_REQUEST_ERR(0, r, "set ec request= %s",ptr);
	s.pool = r->pool;
        curl_easy_setopt(curl, CURLOPT_URL, ptr);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
		AP_LOG_REQUEST_ERR(0, r, "SetExpressCheckout curl_easy_perform() failed: %s, %d\n",curl_easy_strerror(res), res);
            status = 1;
        } else {
	   AP_LOG_REQUEST_ERR(0, r, "response = %s",s.ptr);
           char* line = curl_unescape(s.ptr, strlen(s.ptr));
           parseInput(line, data,r->pool);
           const char* ack = apr_table_get(data, "ACK");
           if(apr_strnatcasecmp(ack, "Success") == 0) {
              const char* token = apr_table_get(data, "TOKEN");
           } else {
		AP_LOG_REQUEST_ERR(0, r, "SetExpressCheckout API has been failed: %s", s.ptr);
              status = 3;
	   }
        }
        curl_easy_cleanup(curl);
    } else {
	AP_LOG_REQUEST_ERR(0, r, "SetExpressCheckout CURL is false.");
        status = 2;
    }
    curl_global_cleanup();
	AP_LOG_REQUEST_ERR(0, r, "SetExpressCheckout API has been completed.");
    return status;
}

static void register_hooks(apr_pool_t *p)
{
    ap_hook_check_user_id(authenticate_user,NULL,NULL,APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA   paypal_ec_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    ec_directives,
    register_hooks
};

