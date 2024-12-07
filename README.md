AmxxEasyHttp
=======
AmxxEasyHttp is a module for amxmodx which provides an easy to use API for HTTP, HTTPS and FTP requests.

Example
-------
Here is a short example which shows some features of AmxxEasyHttp:

```c
public http_get()
{
    ezhttp_get("https://httpbin.org/get", "http_complete")
}

public http_post()
{
    new EzHttpOptions:options_id = ezhttp_create_options()
    ezhttp_option_set_header(options_id, "Content-Type", "text/plain")
    ezhttp_option_set_body(options_id, "Hello!")

    ezhttp_post("https://httpbin.org/post", "http_complete", options_id)
}

public http_complete(EzHttpRequest:request_id)
{
    if (ezhttp_get_error_code(request_id) != EZH_OK)
    {
        new error[64]
        ezhttp_get_error_message(request_id, error, charsmax(error))
        server_print("Response error: %s", error);
        return
    }

    new data[512]
    ezhttp_get_data(request_id, data, charsmax(data))
    server_print("Response data: %s", data)

    // large data cannot be read completely by ezhttp_get_data because of amxmodx's maximum array size limitation, 
    // so you can save the whole response to a file
    ezhttp_save_data_to_file(request_id, fmt("addons/amxmodx/response_%d.json", request_id))
}

// --------------------------------------------------------------------

public ftp_upload()
{
    ezhttp_ftp_upload(
        .user = "user", 
        .password = "password", 
        .host = "127.0.0.1", 
        .remote_file = "wads/cstrike_1.wad", 
        .local_file = "cstrike.wad", 
        .on_complete = "ftp_upload_complete"
    )

    ezhttp_ftp_upload2(
        .uri = "ftp://user:password@127.0.0.1/wads/cstrike_2.wad", 
        .local_file = "cstrike.wad", 
        .on_complete = "ftp_upload_complete",
        .security = EZH_SECURE_EXPLICIT
    )
}

public ftp_upload_complete(EzHttpRequest:request_id)
{
    new EzHttpErrorCode:error_code = ezhttp_get_error_code(request_id)
    new uploaded_kb = ezhttp_get_downloaded_bytes(request_id) / 1024
    new Float:elapsed_sec = ezhttp_get_elapsed(request_id)

    server_print("FTP upload complete. Error: %d. Uploaded: %d kb. Elapsed: %f sec", error_code, uploaded_kb, elapsed_sec)
}
```

## Features

* Non blocking mode
* Custom headers
* Url encoded parameters
* Url encoded POST values
* Basic authentication
* Connection and request timeout specification
* Cookie support
* Proxy support
* OpenSSL and WinSSL support for HTTPS requests
* FTP/FTPES download and upload support
* Built-in JSON support
* HTTP keep-alive support

## Advanced features

### Behavior at the end of the map
By default, when you change map, all requests are interrupted. The callback functions are called with error code EZH_REQUEST_CANCELLED.
An alternative is to specify the option ```ezhttp_option_set_plugin_end_behaviour(options_id, EZH_FORGET_REQUEST)```, in which case the request will not be interrupted at the end of the map (but the callback will not be called).

### Request queue
The module uses up to 6 threads to execute requests, so there is no guarantee that requests will be executed in the order in which they were sent.
If you need to execute requests sequentially, you can create a queue with ```new EzHttpQueue:queue_id = ezhttp_create_queue()``` and then set the ```ezhttp_option_set_queue(options_id, queue_id)``` option for all requests that need to be executed within that queue.

## Building

Building AmxxEasyHttp requires CMake 3.18+ and GCC or MSVC compiler with C++17 support. Tested compilers are:

* GCC 9.4.0
* MSVC 2019

Building the library is done using CMake. You can run the CMake GUI to configure the library or use the command line:

```
mkdir Release
cd Release
cmake .. -DCMAKE_BUILD_TYPE=Release
make easy_http
```
