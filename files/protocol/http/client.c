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

#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include "lib/libubox/blobmsg.h"

#include "uhttpd.h"
#include "client.h"
#include "file.h"
#include "utils.h"
//#include "uh_ssl.h"
#include "log/log.h"
#include "../resetful/parse_json.h"


static const char *const http_versions[] = {
    [UH_HTTP_VER_09] = "HTTP/0.9",
    [UH_HTTP_VER_10] = "HTTP/1.0",
    [UH_HTTP_VER_11] = "HTTP/1.1"
};

static const char *const http_methods[] = {
    [UH_HTTP_METHOD_GET] = "GET",
    [UH_HTTP_METHOD_POST] = "POST",
    [UH_HTTP_METHOD_HEAD] = "HEAD",
    [UH_HTTP_METHOD_PUT] =  "PUT",
    [UH_HTTP_METHOD_DELETE] = "DELETE"
};

static inline void client_send(struct uh_client *cl, const void *data, int len)
{
    ustream_write(cl->us, data, len, true);
}

static void client_send_header(struct uh_client *cl, int code, const char *summary, int64_t length/* change by ycs */)
{
    cl->printf(cl, "%s %03i %s\r\n", http_versions[cl->request.version], code, summary);
    cl->printf(cl, "Server: Libuhttpd %s\r\n", UHTTPD_VERSION_STRING);
    cl->printf(cl, "Connection: keep-alive\r\n");
    if (length < 0) {
        cl->printf(cl, "Transfer-Encoding: chunked\r\n");
    } else {
        cl->printf(cl, "Content-Length: %lld\r\n", length);
    }

    cl->response_length = length;
}

static inline void client_append_header(struct uh_client *cl, const char *name, const char *value)
{
    cl->printf(cl, "%s: %s\r\n", name, value);
}

static inline void client_header_end(struct uh_client *cl)
{
    cl->printf(cl, "\r\n");
}

static inline void client_redirect(struct uh_client *cl, int code, const char *fmt, ...)
{
    va_list arg;
    const char *summary = ((code == 301) ? "Moved Permanently" : "Found");

    assert((code == 301 || code == 302) && fmt);

    cl->send_header(cl, code, summary, 0);
    cl->printf(cl, "Location: ");
    va_start(arg, fmt);
    cl->vprintf(cl, fmt, arg);
    va_end(arg);

    cl->printf(cl, "\r\n\r\n");
    cl->request_done(cl);
}

static void client_send_error(struct uh_client *cl, int code, const char *summary, const char *fmt, ...)
{
    va_list arg;

    cl->send_header(cl, code, summary, -1);
    cl->printf(cl, "Content-Type: text/html\r\n\r\n");

    cl->chunk_printf(cl, "<h1>%s</h1>", summary);

    if (fmt) {
        va_start(arg, fmt);
        cl->chunk_vprintf(cl, fmt, arg);
        va_end(arg);
    }

    cl->request_done(cl);
}

static void client_send_error_json(struct uh_client *cl, int err_code, const char *message)
{
    va_list arg;
    int len = 0;
    char *pjson;
    pjson = assemble_json_response(err_code, message);
    len = strlen(pjson);
    cl->send_header(cl, 200, "OK", len);
    cl->printf(cl, "Content-Type: text/json\r\n\r\n");
    cl->printf(cl, "%s", pjson);
    cl->request_done(cl);
    if(pjson){
        free(pjson);
        pjson = NULL;
    }
}


static void client_send_json(struct uh_client *cl, int err_code, const char *message, const char *content)
{
    int len = 0;
    char *buffer;
    buffer = assemble_json_data_response(err_code, message, content);
    len = strlen(buffer);
    cl->send_header(cl, 200, "OK", len);
    cl->printf(cl, "Content-Type: text/json\r\n\r\n");
    cl->printf(cl, "%s", buffer);
    cl->request_done(cl);
    free(buffer);
}

static int clinet_srv_send_request(struct uh_client *cl)
{
    #define HTTP_HEADER_LEN 256
    char *send_buffer;
    int j = 0;
    int ret = -1;
    enum http_method method = cl->srv_request.method;
    char *path = cl->srv_request.path;
    char *host = cl->srv_request.host;
    char *data= cl->srv_request.data;
    int data_len = cl->srv_request.content_length;
    /* reset result code */
    cl->srv_request.result_code = -1;
    char *http_headers = (char*)malloc(HTTP_HEADER_LEN);
    if(http_headers == NULL)
        return -1;
    switch(method){
        case UH_HTTP_METHOD_GET:
            break;
        case UH_HTTP_METHOD_POST:
            j = sprintf(http_headers,"%s", http_methods[method]);
            j += sprintf(http_headers+j," %s", path);
            j += sprintf(http_headers+j," HTTP/1.1\r\n");
            j += sprintf(http_headers+j,"Host: %s\r\n", host);
            j += sprintf(http_headers+j,"Connection: keep-alive\r\n");
            j += sprintf(http_headers+j,"Content-Length: %d\r\n", data_len);
            j += sprintf(http_headers+j,"Content-Type: application/json\r\n");
            if(j > HTTP_HEADER_LEN){
                printf_err("the head is %d is more than buffer len %d\n", j, HTTP_HEADER_LEN);
                return -1;
            }
            if(HTTP_HEADER_LEN - j < data_len){
                http_headers=(char*)realloc(http_headers, HTTP_HEADER_LEN+data_len);
                if(http_headers == NULL){
                    goto exit;
                }
            }
            printf_note("http_headers:%s\n", http_headers);
            sprintf(http_headers+j, "\r\n%s", data);
            cl->printf(cl, http_headers);
            break;
        default:
            break;
    }
exit:
    free(http_headers);
    http_headers = NULL;
    return ret;
}

int clinet_srv_send_post(char *path, char *data)
{
    struct uh_client *cl_list, *list_tmp;
    char host[64] = {0};
    if(get_uh_srv() == NULL)
        return -1;
    printf_note("path:%s\n", path);
    list_for_each_entry_safe(cl_list, list_tmp, &get_uh_srv()->clients, list){
        printf_note("send post on list:%s, port=%d\n",  cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
        snprintf(host, sizeof(host), "%s:%d", cl_list->get_peer_addr(cl_list), cl_list->get_peer_port(cl_list));
        printf_note("host:%s\n", host);
        cl_list->srv_request.method = UH_HTTP_METHOD_POST;
        cl_list->srv_request.host = &host[0];
        cl_list->srv_request.path = path;
        cl_list->srv_request.data = data;
        cl_list->srv_request.content_length = strlen(data);
        printf_note("srv host:%s\n", cl_list->srv_request.host);
        clinet_srv_send_request(cl_list);
    }
    return 0;
}

static inline const char *client_get_version(struct uh_client *cl)
{
    return http_versions[cl->request.version];
}

static inline const char *client_get_method(struct uh_client *cl)
{
    return http_methods[cl->request.method];
}

static inline const char *client_get_peer_addr(struct uh_client *cl)
{
    return inet_ntoa(cl->peer_addr.sin_addr);
}

static inline int client_get_peer_port(struct uh_client *cl)
{
    return ntohs(cl->peer_addr.sin_port);
}

static inline const char *client_get_url(struct uh_client *cl)
{
    return kvlist_get(&cl->request.url, "url");
}

static inline const char *client_get_path(struct uh_client *cl)
{
    return kvlist_get(&cl->request.url, "path");
}

static inline const char *client_get_query(struct uh_client *cl)
{
    return kvlist_get(&cl->request.url, "query");
}

const char *client_get_var(struct uh_client *cl, const char *name)
{
    return kvlist_get(&cl->request.var, name);
}

const char *client_get_restful_var(struct uh_client *cl, const char *name)
{
    return kvlist_get(&cl->request.resetful_var, name);
}


static inline const char *client_get_header(struct uh_client *cl, const char *name)
{
    return kvlist_get(&cl->request.header, name);
}

static inline const char *client_get_body(struct uh_client *cl, int *len)
{
    *len = cl->dispatch.post_len;
    return cl->dispatch.body;
}

static int post_post_data(struct uh_client *cl, const char *data, int len)
{
    struct dispatch *d = &cl->dispatch;
    d->post_len += len;
    printf_warn("d->post_len=%d\n", d->post_len);

    if (d->post_len > UH_POST_MAX_POST_SIZE)
        goto err;

    if (d->post_len > UH_POST_DATA_BUF_SIZE) {
        d->body = realloc(d->body, UH_POST_MAX_POST_SIZE);
        if (!d->body) {
            cl->send_error(cl, 500, "Internal Server Error", "No memory");
            return 0;
        }
    }

    memcpy(d->body, data, len);
    return len;
err:
    cl->send_error(cl, 413, "Request Entity Too Large", NULL);
    return 0;
}

static void post_post_done(struct uh_client *cl)
{
    char *path = kvlist_get(&cl->request.url, "path");

    if (cl->srv->on_request(cl) == UH_REQUEST_DONE)
        return;
    printf_warn("path=%s\n", path);
    //if (handle_file_request(cl, path))
    //    return;
    if(cl->dispatch.cmd == 0){
        if (handle_file_request(cl, path))
            return;
    }else{
         if(http_dispatch_requset_handle(cl, path)){
            return;
         }
    }

    if (cl->srv->on_error404) {
        cl->srv->on_error404(cl);
        return;
    }
    cl->send_error_json(cl, 404, "Not Found");
    //cl->send_error(cl, 404, "Not Found", "The requested PATH %s was not found on this server.", path);
}

static void post_data_free(struct uh_client *cl)
{
    struct dispatch *d = &cl->dispatch;
    free(d->body);
}

static void uh_handle_request(struct uh_client *cl)
{
    char *path = kvlist_get(&cl->request.url, "path");

    if (cl->srv->on_request) {
        struct dispatch *d = &cl->dispatch;

        switch (cl->request.method) {
        case UH_HTTP_METHOD_GET:
        case UH_HTTP_METHOD_DELETE:
        case UH_HTTP_METHOD_PUT:
            if (cl->srv->on_request(cl) == UH_REQUEST_DONE)
                return;
            break;
        case UH_HTTP_METHOD_POST:
            d->post_data = post_post_data;
            d->post_done = post_post_done;
            d->free = post_data_free;
            d->body = calloc(1, UH_POST_DATA_BUF_SIZE);
            if (!d->body)
                cl->send_error_json(cl, 500, "Internal Server Error,No memory");
                //cl->send_error(cl, 500, "Internal Server Error", "No memory");
            return;
        default:
            cl->send_error_json(cl, 400, "Bad Request,Invalid Request");
            //cl->send_error(cl, 400, "Bad Request", "Invalid Request");
            return;
        }
    }
    if(cl->dispatch.cmd == 0){
         printf_info("handle_file_request blk act=%d\n", cl->dispatch.cmd);
        if (handle_file_request(cl, path))
            return;
    }else{
         printf_info("path = %s, blk act=%d\n", path, cl->dispatch.cmd);
         if(http_dispatch_requset_handle(cl, path)){
            return;
         }
    }
   
    if (cl->srv->on_error404) {
        cl->srv->on_error404(cl);
        return;
    }
    cl->send_error(cl, 404, "Not Found", "The requested PATH %s was not found on this server.", path);
}

static inline void connection_close(struct uh_client *cl)
{
    cl->us->eof = true;
    cl->state = CLIENT_STATE_CLOSE;
    ustream_state_change(cl->us);
}

static inline void keepalive_cb(struct uloop_timeout *timeout)
{
    struct uh_client *cl = container_of(timeout, struct uh_client, timeout);
    printf_warn("keepalive_cb###\n");

    connection_close(cl);
}

static void dispatch_done(struct uh_client *cl)
{
    if (cl->dispatch.free)
        cl->dispatch.free(cl);

    memset(&cl->dispatch, 0, sizeof(struct dispatch));
}

static inline int hdr_get_len(struct kvlist *kv, const void *data)
{
    return strlen(data) + 1;
}

static void client_request_done(struct uh_client *cl)
{
    if (cl->response_length < 0)
        cl->printf(cl, "0\r\n\r\n");

    dispatch_done(cl);

    if (cl->connection_close) {
        connection_close(cl);
        return;
    }

    cl->state = CLIENT_STATE_INIT;

    memset(&cl->request, 0, sizeof(cl->request));
    memset(&cl->dispatch, 0, sizeof(cl->dispatch));
    memset(&cl->srv_request, 0, sizeof(cl->srv_request));
    kvlist_init(&cl->request.url, hdr_get_len);
    kvlist_init(&cl->request.var, hdr_get_len);
    kvlist_init(&cl->request.header, hdr_get_len);
    kvlist_init(&cl->request.resetful_var, hdr_get_len);
    uloop_timeout_set(&cl->timeout, UHTTPD_CONNECTION_TIMEOUT * 1000);
}

static void client_free(struct uh_client *cl)
{
    printf_warn("free######\n");
    if (cl) {
        dispatch_done(cl);
        uloop_timeout_cancel(&cl->timeout);
        if (cl->srv->ssl){
           // uh_ssl_client_detach(cl);
        }
            
        ustream_free(&cl->sfd.stream);
        shutdown(cl->sfd.fd.fd, SHUT_RDWR);
        close(cl->sfd.fd.fd);
        list_del(&cl->list);
        kvlist_free(&cl->request.url);
        kvlist_free(&cl->request.var);
        kvlist_free(&cl->request.header);
        cl->srv->nclients--;

        if (cl->srv->on_client_free)
            cl->srv->on_client_free(cl);

        free(cl);
    }
}

static void parse_resetful_var(struct uh_client *cl, char *str)
{
    struct kvlist *kv = &cl->request.resetful_var;
    char *k, *v;

    k = str;
    v = strchr(k, ',');
    if (v)
        *v++ = 0;

    if (*k && v)
        kvlist_set(kv, k, v);
}

static void parse_var(struct uh_client *cl, char *query)
{
    struct kvlist *kv = &cl->request.var;
    char *k, *v;

    while (query && *query) {
        k = query;
        query = strchr(query, '&');
        if (query)
            *query++ = 0;

        v = strchr(k, '=');
        if (v)
            *v++ = 0;

        if (*k && v)
            kvlist_set(kv, k, v);
    }
}


static int client_parse_request(struct uh_client *cl, char *data)
{
    struct http_request *req = &cl->request;
    char *type, *url, *version, *p;
    int h_method, h_version;
    static char path[PATH_MAX];

    type = strtok(data, " ");
    url = strtok(NULL, " ");
    version = strtok(NULL, " ");
    if (!type || !url || !version)
        return CLIENT_STATE_DONE;

    h_method = find_idx(http_methods, ARRAY_SIZE(http_methods), type);
    h_version = find_idx(http_versions, ARRAY_SIZE(http_versions), version);
    if (h_method < 0 || h_version < 0) {
        req->version = UH_HTTP_VER_10;
        return CLIENT_STATE_DONE;
    }

    kvlist_set(&req->url, "url", url);

    p = strchr(url, '?');
    if (p) {
        *p = 0;
        if (p[1]) {
            kvlist_set(&req->url, "query", p + 1);
            parse_var(cl, p + 1);
        }
    }

    if (uh_urldecode(path, sizeof(path) - 1, url, strlen(url)) < 0)
        return CLIENT_STATE_DONE;

    kvlist_set(&req->url, "path", path);
    
    req->method = h_method;
    req->version = h_version;
    if (req->version < UH_HTTP_VER_11)
        cl->connection_close = true;

    return CLIENT_STATE_HEADER;
}

static int client_srv_parse_response(struct uh_client *cl, const char *data)
{
    /* HTTP/1.1 200 OK */
    char *code, *status, *version, *p;
    char *data_copy= NULL, *data_copy_tmp = NULL; 
    int h_method, h_version;
    
    data_copy_tmp = data_copy = strdup(data);
    version = strtok(data_copy, " ");
    code = strtok(NULL, " ");
    status = strtok(NULL, " ");
    if (!code || !status || !version)
        return -1;
    h_version = find_idx(http_versions, ARRAY_SIZE(http_versions), version);
    if (h_version < 0) {
        return -1;
    }
    p = strstr(code, '200');
    if(p){
        cl->srv_request.result_code = 1;
        printf_note("client: %s:%d response ok: %s\n", status);
    }else{
        cl->srv_request.result_code = 0;
        printf_note("client: %s:%d response false: %s\n", status);
    }
    
    if(data_copy_tmp)
        free(data_copy_tmp);
    data_copy_tmp = NULL;
    
    return 0;
}

static bool client_init_cb(struct uh_client *cl, char *buf, int len)
{
    char *newline;

    newline = strstr(buf, "\r\n");
    if (!newline)
        return false;

    if (newline == buf) {
        ustream_consume(cl->us, 2);
        return true;
    }
    *newline = 0;    
    if(client_srv_parse_response(cl, buf) == 0){
        /* if data is response, return */
        ustream_consume(cl->us, newline + 2 - buf);
        cl->state = CLIENT_STATE_HEADER;
        return true;
    }
    cl->state = client_parse_request(cl, buf);
    ustream_consume(cl->us, newline + 2 - buf);
    if (cl->state == CLIENT_STATE_DONE)
        cl->send_error_json(cl, 400, "Bad Request");
        //cl->send_error(cl, 400, "Bad Request", NULL);

    return true;
}

static void client_poll_post_data(struct uh_client *cl)
{
    struct dispatch *d = &cl->dispatch;
    struct http_request *r = &cl->request;
    char *buf;
    int len;
    printf_info("cl->state=%d,%d\n", cl->state, CLIENT_STATE_DONE);

    if (cl->state == CLIENT_STATE_DONE)
        return;

    while (1) {
        int cur_len;

        buf = ustream_get_read_buf(cl->us, &len);
        if (!buf || !len)
            break;

        if (!d->post_data)
            return;

        cur_len = min(r->content_length, len);
        if (cur_len) {
            if (d->post_data)
                cur_len = d->post_data(cl, buf, cur_len);

            r->content_length -= cur_len;
            ustream_consume(cl->us, cur_len);
            continue;
        }
    }
    if (!r->content_length && cl->state != CLIENT_STATE_DONE) {
        if (cl->dispatch.post_done)
            cl->dispatch.post_done(cl);
#if 0
        if(cl->connection_close != true){
            cl->state = CLIENT_STATE_INIT;
        }else{
            cl->state = CLIENT_STATE_DONE;
        }
#endif
    }

}

static inline bool client_data_cb(struct uh_client *cl, char *buf, int len)
{
    client_poll_post_data(cl);
    return false;
}

static void client_parse_header(struct uh_client *cl, char *data)
{
    struct http_request *req = &cl->request;
    char *err;
    char *name;
    char *val;

    if (!*data) {
        uloop_timeout_cancel(&cl->timeout);
        cl->state = CLIENT_STATE_DATA;
        uh_handle_request(cl);
        return;
    }

    val = uh_split_header(data);
    if (!val) {
        cl->state = CLIENT_STATE_DONE;
        return;
    }

    for (name = data; *name; name++)
        if (isupper(*name))
            *name = tolower(*name);

    if (!strcmp(data, "content-length")) {
        req->content_length = strtoul(val, &err, 0);
        if (err && *err) {
            cl->send_error(cl, 400, "Bad Request", "Invalid Content-Length");
            return;
        }
    } else if (!strcmp(data, "transfer-encoding") && !strcmp(val, "chunked")) {
        cl->send_error(cl, 501, "Not Implemented", "Chunked body is not implemented");
        return;
    } else if (!strcmp(data, "connection") && !strcasecmp(val, "close")) {
        cl->connection_close = true;
    }

    kvlist_set(&req->header, data, val);

    cl->state = CLIENT_STATE_HEADER;
}

static bool client_header_cb(struct uh_client *cl, char *buf, int len)
{
    char *newline;
    int line_len;

    newline = strstr(buf, "\r\n");
    if (!newline)
        return false;

    *newline = 0;
    client_parse_header(cl, buf);
    line_len = newline + 2 - buf;
    ustream_consume(cl->us, line_len);
    printf_debug("cl->state=%d,%d\n", cl->state, CLIENT_STATE_DATA);
    if (cl->state == CLIENT_STATE_DATA)
        return client_data_cb(cl, newline + 2, len - line_len);


    return true;
}

typedef bool (*read_cb_t)(struct uh_client *cl, char *buf, int len);
static read_cb_t read_cbs[] = {
    [CLIENT_STATE_INIT] = client_init_cb,
    [CLIENT_STATE_HEADER] = client_header_cb,
    [CLIENT_STATE_DATA] = client_data_cb,
};

void uh_client_read_cb(struct uh_client *cl)
{
    struct ustream *us = cl->us;
    
    char *str;
    int len;
    
    do {
        str = ustream_get_read_buf(us, &len);
        if (!str || !len)
            return;

        if (cl->state >= ARRAY_SIZE(read_cbs) || !read_cbs[cl->state])
            return;

        if (!read_cbs[cl->state](cl, str, len)) {
            if (len == us->r.buffer_len && cl->state != CLIENT_STATE_DATA)
                cl->send_error(cl, 413, "Request Entity Too Large", NULL);
            break;
        }
    } while(1);
}

static inline void client_ustream_read_cb(struct ustream *s, int bytes)
{
    struct uh_client *cl = container_of(s, struct uh_client, sfd.stream);
    uh_client_read_cb(cl);
}

static void client_ustream_write_cb(struct ustream *s, int bytes)
{
    struct uh_client *cl = container_of(s, struct uh_client, sfd.stream);

    if (cl->dispatch.write_cb)
        cl->dispatch.write_cb(cl);
}

void uh_client_notify_state(struct uh_client *cl)
{
    struct ustream *us = cl->us;
    if (!us->write_error) {
        if (cl->state == CLIENT_STATE_DATA)
            return;

        if (!us->eof || us->w.data_bytes)
            return;
    }

    client_free(cl);
}

static inline void client_notify_state(struct ustream *s)
{
    struct uh_client *cl = container_of(s, struct uh_client, sfd.stream);
    printf_warn("notify######\n");

    uh_client_notify_state(cl);
}

void uh_accept_client(struct uh_server *srv, bool ssl)
{
    struct uh_client *cl;
    unsigned int sl;
    int sfd;
    struct sockaddr_in addr;

    sl = sizeof(addr);
    sfd = accept(srv->fd.fd, (struct sockaddr *)&addr, &sl);
    if (sfd < 0) {
        uh_log_err("accept");
        return;
    }

    cl = calloc(1, sizeof(struct uh_client));
    if (!cl) {
        uh_log_err("calloc");
        goto err;
    }

    memcpy(&cl->peer_addr, &addr, sizeof(addr));

    cl->us = &cl->sfd.stream;
    if (ssl) {
        //uh_ssl_client_attach(cl);
    } else {
        cl->us->notify_read = client_ustream_read_cb;
        cl->us->notify_write = client_ustream_write_cb;
        cl->us->notify_state = client_notify_state;
    }

    cl->us->string_data = true;
    ustream_fd_init(&cl->sfd, sfd);

    cl->timeout.cb = keepalive_cb;
    uloop_timeout_set(&cl->timeout, UHTTPD_CONNECTION_TIMEOUT * 1000);

    list_add(&cl->list, &srv->clients);
    kvlist_init(&cl->request.url, hdr_get_len);
    kvlist_init(&cl->request.var, hdr_get_len);
    kvlist_init(&cl->request.header, hdr_get_len);
    kvlist_init(&cl->request.resetful_var, hdr_get_len);
    
    cl->srv = srv;
    cl->srv->nclients++;

    cl->free = client_free;
    cl->send_error = client_send_error;
    cl->send_error_json =  client_send_error_json;
    cl->send_json = client_send_json;
    cl->send_header = client_send_header;
    cl->append_header = client_append_header;
    cl->header_end = client_header_end;
    cl->redirect = client_redirect;
    cl->request_done = client_request_done;
    cl->srv_send_post = clinet_srv_send_post;
    cl->parse_resetful_var = parse_resetful_var;

    cl->send = client_send;
    cl->printf = uh_printf;
    cl->vprintf = uh_vprintf;
    cl->chunk_send = uh_chunk_send;
    cl->chunk_printf = uh_chunk_printf;
    cl->chunk_vprintf = uh_chunk_vprintf;

    cl->get_version = client_get_version;
    cl->get_method = client_get_method;
    cl->get_peer_addr = client_get_peer_addr;
    cl->get_peer_port = client_get_peer_port;
    cl->get_url = client_get_url;
    cl->get_restful_var = client_get_restful_var;
    cl->get_path = client_get_path;
    cl->get_query = client_get_query;
    cl->get_var = client_get_var;
    cl->get_header = client_get_header;
    cl->get_body = client_get_body;

    if (srv->on_accept)
        srv->on_accept(cl);

    return;
err:
    close(sfd);
}

