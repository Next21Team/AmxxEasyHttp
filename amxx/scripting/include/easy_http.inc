#if defined _easy_http_included
    #endinput
#endif
#define _easy_http_included

#if AMXX_VERSION_NUM >= 175
 #pragma reqlib easy_http
 #if !defined AMXMODX_NOAUTOLOAD
  #pragma loadlib easy_http
 #endif
#else
 #pragma library easy_http
#endif

enum EzHttpErrorCode {
    OK = 0,
    CONNECTION_FAILURE,
    EMPTY_RESPONSE,
    HOST_RESOLUTION_FAILURE,
    INTERNAL_ERROR,
    INVALID_URL_FORMAT,
    NETWORK_RECEIVE_ERROR,
    NETWORK_SEND_FAILURE,
    OPERATION_TIMEDOUT,
    PROXY_RESOLUTION_FAILURE,
    SSL_CONNECT_ERROR,
    SSL_LOCAL_CERTIFICATE_ERROR,
    SSL_REMOTE_CERTIFICATE_ERROR,
    SSL_CACERT_ERROR,
    GENERIC_SSL_ERROR,
    UNSUPPORTED_PROTOCOL,
    REQUEST_CANCELLED,
    TOO_MANY_REDIRECTS,
    UNKNOWN_ERROR = 1000,
};

enum EzHttpProgress
{
    EZH_DownloadNow = 0,
    EZH_DownloadTotal,
    EZH_UploadNow,
    EZH_UploadTotal
};

// options natives
native EzHttpOptions:ezhttp_create_options();
native ezhttp_option_set_user_agent(EzHttpOptions:options_id, const user_agent[]);
native ezhttp_option_add_url_parameter(EzHttpOptions:options_id, const key[], const value[]);
native ezhttp_option_add_form_payload(EzHttpOptions:options_id, const key[], const value[]);
native ezhttp_option_set_body(EzHttpOptions:options_id, const body[]);
native ezhttp_option_set_header(EzHttpOptions:options_id, const key[], const value[]);
native ezhttp_option_set_cookie(EzHttpOptions:options_id, const key[], const value[]);
native ezhttp_option_set_timeout(EzHttpOptions:options_id, timeout_ms);
native ezhttp_option_set_proxy(EzHttpOptions:options_id, const proxy_url[]);
native ezhttp_option_set_proxy_auth(EzHttpOptions:options_id, const user[], const password[]);
native ezhttp_option_set_auth(EzHttpOptions:options_id, const user[], const password[]);
native ezhttp_option_set_user_data(EzHttpOptions:options_id, const data[], len);

// request natives
native EzHttpRequest:ezhttp_get(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
native EzHttpRequest:ezhttp_post(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
native ezhttp_is_request_exists(EzHttpRequest:request_id);
native ezhttp_cancel_request(EzHttpRequest:request_id);
native ezhttp_request_progress(EzHttpRequest:request_id, progress[EzHttpProgress]); // it's not recommend to use this too frequently (every frame)

// response natives. You can use these only in response on_complete callback!
native ezhttp_get_http_code(EzHttpRequest:request_id);
native ezhttp_get_data(EzHttpRequest:request_id, buffer[], max_len);
native ezhttp_get_url(EzHttpRequest:request_id, buffer[], max_len);
native ezhttp_save_data_to_file(EzHttpRequest:request_id, const file_path[]); // file_path must be relative to mod dir
native ezhttp_save_data_to_file2(EzHttpRequest:request_id, file_handle); // file_handle must be opened via fopen with write permissions. Returns written bytes count
native ezhttp_get_headers_count(EzHttpRequest:request_id);
native ezhttp_get_headers(EzHttpRequest:request_id, const key[], value[], max_len);
native Float:ezhttp_get_elapsed(EzHttpRequest:request_id);
native ezhttp_get_cookies_count(EzHttpRequest:request_id);
native ezhttp_get_cookies(EzHttpRequest:request_id, const key[], value[], max_len);
native EzHttpErrorCode:ezhttp_get_error_code(EzHttpRequest:request_id);
native ezhttp_get_error_message(EzHttpRequest:request_id, buffer[], max_len);
native ezhttp_get_redirect_count(EzHttpRequest:request_id);
native ezhttp_get_uploaded_bytes(EzHttpRequest:request_id);
native ezhttp_get_downloaded_bytes(EzHttpRequest:request_id);
native ezhttp_get_user_data(EzHttpRequest:request_id, data[]);