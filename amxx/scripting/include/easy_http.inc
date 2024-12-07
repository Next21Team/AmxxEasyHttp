#if defined _easy_http_included
    #endinput
#endif
#define _easy_http_included

#pragma reqlib easy_http
#if !defined AMXMODX_NOAUTOLOAD
    #pragma loadlib easy_http
#endif

#include <easy_http_json>

enum EzHttpErrorCode {
    EZH_OK = 0,
    EZH_CONNECTION_FAILURE,
    EZH_EMPTY_RESPONSE,
    EZH_HOST_RESOLUTION_FAILURE,
    EZH_INTERNAL_ERROR,
    EZH_INVALID_URL_FORMAT,
    EZH_NETWORK_RECEIVE_ERROR,
    EZH_NETWORK_SEND_FAILURE,
    EZH_OPERATION_TIMEDOUT,
    EZH_PROXY_RESOLUTION_FAILURE,
    EZH_SSL_CONNECT_ERROR,
    EZH_SSL_LOCAL_CERTIFICATE_ERROR,
    EZH_SSL_REMOTE_CERTIFICATE_ERROR,
    EZH_SSL_CACERT_ERROR,
    EZH_GENERIC_SSL_ERROR,
    EZH_UNSUPPORTED_PROTOCOL,
    EZH_REQUEST_CANCELLED,
    EZH_TOO_MANY_REDIRECTS,
    EZH_UNKNOWN_ERROR = 1000,
};

enum EzHttpProgress
{
    EZH_DownloadNow = 0,
    EZH_DownloadTotal,
    EZH_UploadNow,
    EZH_UploadTotal
};

enum EzHttpFtpSecurity
{
    EZH_UNSECURE = 0,
    EZH_SECURE_EXPLICIT
};

enum EzHttpPluginEndBehaviour
{
    EZH_CANCEL_REQUEST = 0,
    EZH_FORGET_REQUEST,
};

/**
 * Creates new options object. This object allows you to configure your request by specifying 
 * such parameters as user agent, query parameters, headers, and etc.
 *
 * @return                  EzHttpOptions handle.
 */
native EzHttpOptions:ezhttp_create_options();

/**
 * Sets user-agent string for a request.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param user_agent        User-agent string.
 *
 * @noreturn
 */
native ezhttp_option_set_user_agent(EzHttpOptions:options_id, const user_agent[]);

/**
 * Adds a query parameter to the URL.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param key               Parameter key.
 * @param value             Parameter value.
 *
 * @noreturn
 */
native ezhttp_option_add_url_parameter(EzHttpOptions:options_id, const key[], const value[]);

/**
 * Adds a key-value pair to the form payload.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param key               Parameter key.
 * @param value             Parameter value.
*
 * @noreturn
 */
native ezhttp_option_add_form_payload(EzHttpOptions:options_id, const key[], const value[]);

/**
 * Sets the request body.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param body              Request body.
 *
 * @noreturn
 */
native ezhttp_option_set_body(EzHttpOptions:options_id, const body[]);

/**
 * Copies serialized string to the requests body.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param options_id        Options identifier created via ezhttp_create_options().  
 * @param json              EzJSON handle.
 * @param pretty            True to format pretty JSON string, false to not.
 *
 * @return                  True if serialization was successful, false otherwise.
 * @error                   If passed handle is not a valid value. If passed options_id is not exists.
 */
native bool:ezhttp_option_set_body_from_json(EzHttpOptions:options_id, EzJSON:json, bool:pretty = false);

/**
 * Sets a timeout for the HTTP request. 
 *
 * @note                     Specify 0 to disable timeout and make the request wait indefinitely.
 *                           Useful for long-polling.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param timeout_ms         The timeout to set in milliseconds.
 *
 * @noreturn
 */
native ezhttp_option_set_timeout(EzHttpOptions:options_id, timeout_ms);

/**
 * Sets a connect timeout for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param timeout_ms         The timeout to set in milliseconds.
 *
 * @noreturn
 */
native ezhttp_option_set_connect_timeout(EzHttpOptions:options_id, timeout_ms);

/**
 * Sets a proxy for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param proxy_url          The URL of the proxy.
 *
 * @noreturn
 */
native ezhttp_option_set_proxy(EzHttpOptions:options_id, const proxy_url[]);

/**
 * Sets a proxy authentication for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param user               The user name for the proxy authentication.
 * @param password           The password for the proxy authentication.
 *
 * @noreturn
 */
native ezhttp_option_set_proxy_auth(EzHttpOptions:options_id, const user[], const password[]);

/**
 * Sets an authentication for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param user               The user name for the authentication.
 * @param password           The password for the authentication.
 *
 * @noreturn
 */
native ezhttp_option_set_auth(EzHttpOptions:options_id, const user[], const password[]);

/**
 * Sets a custom request data for the HTTP request.
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param data               The user data to set.
 * @param len                The length of the user data.
 *
 * @noreturn
 */
native ezhttp_option_set_user_data(EzHttpOptions:options_id, const data[], len);

/**
 * Sets a plugin end behaviour for the HTTP request.
 *
 * @param options_id            Options identifier created via ezhttp_create_options().
 * @param plugin_end_behaviour  Member of EzHttpPluginEndBehaviour. The plugin end behaviour to set. 
 *                              Valid values are:
 *                              * EZH_CANCEL_REQUEST to cancel the request;
 *                              * EZH_FORGET_REQUEST to complete the request, but ignore its result
 *                              (callback will not be called).
 *
 * @noreturn
 */
native ezhttp_option_set_plugin_end_behaviour(
    EzHttpOptions:options_id,
    EzHttpPluginEndBehaviour:plugin_end_behaviour
);

/**
 * Sets a queue for the HTTP request.
 * Queues allow you to run all the requests in the queue sequentially (in the order they were called).
 *
 * @param options_id         Options identifier created via ezhttp_create_options().
 * @param queue_id           The queue to for the request.
 *
 * @noreturn
 */
native ezhttp_option_set_queue(EzHttpOptions:options_id, EzHttpQueue:queue_id);

/**
 * Creates a new HTTP request queue. Requests in the queue are executed sequentially.
 *
 * @return                  EzHttpQueue queue handle.
 */
native EzHttpQueue:ezhttp_create_queue();

/**
 * Performs a GET request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Data to send with the request.
 * @param data_len          Length of the data.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_get(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a POST request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Data to send with the request.
 * @param data_len          Length of the data.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_post(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a PUT request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Data to send with the request.
 * @param data_len          Length of the data.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_put(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a PATCH request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Data to send with the request.
 * @param data_len          Length of the data.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_patch(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Performs a DELETE request.
 *
 * @param url               URL to send the request to.
 * @param on_complete       Function to call when the request is complete.
 *                          Signature: public on_complete(EzHttpRequest:request_id)
 * @param options_id        Options identifier created via ezhttp_create_options().
 * @param data              Data to send with the request.
 * @param data_len          Length of the data.
 *
 * @return                  Request identifier.
 */
native EzHttpRequest:ezhttp_delete(
    const url[] = "", 
    const on_complete[] = "", 
    EzHttpOptions:options_id = EzHttpOptions:0,
    const data[] = {}, 
    const data_len = 0 
);

/**
 * Checks if a request exists.
 *
 * @param request_id        Request identifier.
 *
 * @return                  True if the request exists, false otherwise.
 */
native bool:ezhttp_is_request_exists(EzHttpRequest:request_id);

/**
 * Cancels a request.
 *
 * @param request_id        Request identifier.
 *
 * @noreturn
 */
native ezhttp_cancel_request(EzHttpRequest:request_id);

/**
 * Gets the request progress.
 *
 * @note                    It's not recommended to use this too frequently (every frame)
 *
 * @param request_id        Request identifier.
 * @param progress          Array to store the progress values in.
 *
 * @noreturn
 */
native ezhttp_request_progress(EzHttpRequest:request_id, progress[EzHttpProgress]);

/**
 * Gets the HTTP status code of the request.
 *
 * @note                    This native can only be used in the on_complete callback.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The HTTP status code.
 */
native ezhttp_get_http_code(EzHttpRequest:request_id);

/**
 * Gets the response data of the request.
 *
 * @note                    This native can only be used in the on_complete callback.
 *
 * @param request_id        The request identifier.
 * @param buffer            The buffer to store the data in.
 * @param max_len           The maximum length of the buffer.
 *
 * @noreturn
 */
native ezhttp_get_data(EzHttpRequest:request_id, buffer[], max_len);

/**
 * Parses http response body to JSON.
 *
 * @note                    Needs to be freed using ezjson_free() native.
 *
 * @param request_id        request_id  
 * @param with_comments     True if parsing JSON includes comments (it will ignore them), false otherwise
 *
 * @return                  EzJSON handle, EzInvalid_JSON if error occurred
 */
native EzJSON:ezhttp_parse_json_response(EzHttpRequest:request_id, bool:with_comments = false);

/**
 * Returns the URL of the request.
 *
 * @param request_id        The request identifier.
 * @param buffer            The buffer to store the URL.
 * @param max_len           The maximum length of the buffer.
 *
 * @noreturn
 */
native ezhttp_get_url(EzHttpRequest:request_id, buffer[], max_len);

/**
 * Saves the request data to a file.
 *
 * @param request_id        The request identifier.
 * @param file_path         The path to the file to save to. Must be relative to the mod directory.
 *
 * @return                  The number of bytes written to the file.
 */
native ezhttp_save_data_to_file(EzHttpRequest:request_id, const file_path[]);

/**
 * Saves the request data to a file.
 *
 * @param request_id        The request identifier.
 * @param file_handle       The file handle to write to. Must be opened via fopen with write permissions.
 *
 * @return                  The number of bytes written to the file.
 */
native ezhttp_save_data_to_file2(EzHttpRequest:request_id, file_handle);

/**
 * Returns the number of headers in the response.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of headers in the response.
 */
native ezhttp_get_headers_count(EzHttpRequest:request_id);

/**
 * Returns a header value by name.
 *
 * @param request_id        The request identifier.
 * @param key               The name of the header to retrieve.
 * @param value             The buffer to store the value.
 * @param max_len           The maximum length of the buffer.
 *
 * @return                  True if the header was found, false otherwise
 */
native bool:ezhttp_get_headers(EzHttpRequest:request_id, const key[], value[], max_len);

/**
 * Returns the elapsed time of the request in seconds.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The elapsed time in seconds.
 */
native Float:ezhttp_get_elapsed(EzHttpRequest:request_id);

/**
 * Returns the number of cookies in the response.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of cookies in the response.
 */
native ezhttp_get_cookies_count(EzHttpRequest:request_id);

/**
 * Returns a cookie value by name.
 *
 * @param request_id        The request identifier.
 * @param key               The name of the cookie to retrieve.
 * @param value             The buffer to store the value.
 * @param max_len           The maximum length of the buffer.
 *
 * @return                  True if the cookie was found, false otherwise
 */
native bool:ezhttp_get_cookies(EzHttpRequest:request_id, const key[], value[], max_len);

/**
 * Returns the error code of the connection failure of the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The error code of the request.
 */
native EzHttpErrorCode:ezhttp_get_error_code(EzHttpRequest:request_id);

/**
 * Returns the error message of the request.
 *
 * @param request_id        The request identifier.
 * @param buffer            The buffer to store the error message.
 * @param max_len           The maximum length of the buffer.
 *
 * @noreturn
 */
native ezhttp_get_error_message(EzHttpRequest:request_id, buffer[], max_len);

/**
 * Returns the number of redirects in the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of redirects in the request.
 */
native ezhttp_get_redirect_count(EzHttpRequest:request_id);

/**
 * Returns the number of bytes uploaded in the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of bytes uploaded in the request.
 */
native ezhttp_get_uploaded_bytes(EzHttpRequest:request_id);

/**
 * Returns the number of bytes downloaded in the request.
 *
 * @param request_id        The request identifier.
 *
 * @return                  The number of bytes downloaded in the request.
 */
native ezhttp_get_downloaded_bytes(EzHttpRequest:request_id);

/**
 * Returns the custom data associated with the request set by ezhttp_option_set_user_data.
 *
 * @param request_id        The request identifier.
 * @param data              The buffer to store the user data.
 *
 * @noreturn
 */
native ezhttp_get_user_data(EzHttpRequest:request_id, data[]);

/**
 * Uploads a file to a remote server using FTP.
 *
 * @param user               The user name to use for the FTP connection.
 * @param password           The password to use for the FTP connection.
 * @param host               The host to connect to.
 * @param remote_file        The remote file to upload to.
 * @param local_file         The local file to upload.
 * @param on_complete        The function to call when the upload is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_upload(
    const user[] = "", 
    const password[] = "", 
    const host[] = "", 
    const remote_file[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);

/**
 * Uploads a file to a remote server using FTP by URI.
 *
 * @param uri                The URI to upload to.
 * @param local_file         The local file to upload.
 * @param on_complete        The function to call when the upload is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_upload2(
    const uri[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);

/**
 * Downloads a file from a remote server using FTP.
 *
 * @param user               The user name to use for the FTP connection.
 * @param password           The password to use for the FTP connection.
 * @param host               The host to connect to.
 * @param remote_file        The remote file to download.
 * @param local_file         The local file to save to.
 * @param on_complete        The function to call when the download is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_download(
    const user[] = "", 
    const password[] = "", 
    const host[] = "", 
    const remote_file[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);

/**
 * Downloads a file from a remote server using FTP by URI.
 *
 * @param uri                The URI to download from.
 * @param local_file         The local file to save to.
 * @param on_complete        The function to call when the download is complete.
 *                           Signature: public on_complete(EzHttpRequest:request_id)
 * @param security           Member of EzHttpFtpSecurity. The security strategy use for the FTP connection.
 * @param options_id         The options to use for the request.
 *
 * @return                   The request handle.
 */
native EzHttpRequest:ezhttp_ftp_download2(
    const uri[] = "", 
    const local_file[] = "", 
    const on_complete[] = "", 
    EzHttpFtpSecurity:security = EZH_UNSECURE, 
    EzHttpOptions:options_id = EzHttpOptions:0
);