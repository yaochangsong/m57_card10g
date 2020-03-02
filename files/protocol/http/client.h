/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <netinet/in.h>
#include "lib/libubox/kvlist.h"
#include "lib/libubox/ustream.h"
//#include "config.h"

#if (UHTTPD_SSL_SUPPORT)
#include <libubox/ustream-ssl.h>
#endif

#define UHTTPD_CONNECTION_TIMEOUT 60

#define UH_POST_DATA_BUF_SIZE   1024
#define UH_POST_MAX_POST_SIZE   4096

enum request_status {
    UH_REQUEST_DONE,
    UH_REQUEST_CONTINUE
};

enum http_method {
    UH_HTTP_METHOD_GET,
    UH_HTTP_METHOD_POST,
    UH_HTTP_METHOD_HEAD,
    UH_HTTP_METHOD_PUT,
    UH_HTTP_METHOD_DELETE,
};

enum http_version {
    UH_HTTP_VER_09,
    UH_HTTP_VER_10,
    UH_HTTP_VER_11
};

enum client_state {
    CLIENT_STATE_INIT,
    CLIENT_STATE_HEADER,
    CLIENT_STATE_DATA,
    CLIENT_STATE_DONE,
    CLIENT_STATE_CLOSE
};

struct http_request {
    enum http_method method;
    enum http_version version;
    int content_length;
    struct kvlist url;
    struct kvlist var;
    struct kvlist header;
    struct kvlist resetful_var;
};

struct http_srv_request {
    enum http_method method;
    enum http_version version;
    char *path;
    char *host;
    char *data;
    int content_length;
    int  result_code;  /* 0: false, 1: ok  -1: wait */
};


struct uh_client;

#include "protocol/resetful/request_file.h"

struct dispatch {
    int (*post_data)(struct uh_client *cl, const char *data, int len);
    void (*post_done)(struct uh_client *cl);
    void (*write_cb)(struct uh_client *cl);
    void (*free)(struct uh_client *cl);

    struct {
        int fd;
        char filename[64];
        char path[FILE_PATH_MAX_LEN];/* add by ycs */
    } file;
    int cmd; /* add by ycs */
    int post_len;
    char *body;
};

struct uh_client {
    struct list_head list;
    struct uh_server *srv;
    struct ustream *us;
    struct ustream_fd sfd;
#if (UHTTPD_SSL_SUPPORT)
    struct ustream_ssl ssl;
#endif
    struct uloop_timeout timeout;
    enum client_state state;
    struct http_request request;
    struct http_srv_request srv_request;    /* add by ycs */
    struct sockaddr_in peer_addr;
    struct dispatch dispatch;
    bool connection_close;
    int response_length;

    void (*free)(struct uh_client *cl);
    void (*send_error)(struct uh_client *cl, int code, const char *summary, const char *fmt, ...);
    void (*send_error_json)(struct uh_client *cl, int err_code, const char *summary);
    void (*send_json)(struct uh_client *cl, int err_code, const char *message, const char *content);
    void (*send_header)(struct uh_client *cl, int code, const char *summary, int64_t length);
    void (*append_header)(struct uh_client *cl, const char *name, const char *value);
    void (*header_end)(struct uh_client *cl);
    void (*redirect)(struct uh_client *cl, int code, const char *fmt, ...);
    void (*request_done)(struct uh_client *cl);
    int  (*srv_send_post)(char *path, char *data);
    void (*parse_resetful_var)(struct uh_client *cl, char *str);
    
    void (*send)(struct uh_client *cl, const void *data, int len);
    void (*printf)(struct uh_client *cl, const char *format, ...);
    void (*vprintf)(struct uh_client *cl, const char *format, va_list arg);

    void (*chunk_send)(struct uh_client *cl, const void *data, int len);
    void (*chunk_printf)(struct uh_client *cl, const char *format, ...);
    void (*chunk_vprintf)(struct uh_client *cl, const char *format, va_list arg);

    const char *(*get_method)(struct uh_client *cl);
    const char *(*get_version)(struct uh_client *cl);
    const char *(*get_peer_addr)(struct uh_client *cl);
    int (*get_peer_port)(struct uh_client *cl);
    const char *(*get_url)(struct uh_client *cl);
    const char *(*get_path)(struct uh_client *cl);
    const char *(*get_query)(struct uh_client *cl);
    const char *(*get_var)(struct uh_client *cl, const char *name);
    const char *(*get_restful_var)(struct uh_client *cl, const char *name);
    const char *(*get_header)(struct uh_client *cl, const char *name);
    const char *(*get_body)(struct uh_client *cl, int *len);
};

void uh_client_read_cb(struct uh_client *cl);
void uh_client_notify_state(struct uh_client *cl);
void uh_accept_client(struct uh_server *srv, bool ssl);

#endif
