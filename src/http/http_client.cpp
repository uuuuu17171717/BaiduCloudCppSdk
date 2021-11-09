/**
 * Copyright 2014 (c) Baidu, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */
#include <iostream>
#include <curl/curl.h>
#include <stdlib.h>
#include <errno.h>
#include "bcesdk/common/common.h"
#include "bcesdk/common/stream.h"
#include "bcesdk/common/memory_stream.h"
#include "bcesdk/util/util.h"
#include "bcesdk/http/http_client.h"
#ifndef _WIN32

#include "bcesdk/http/unix_curl_global.h"
#else
#include <Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include "bcesdk/http/win_curl_global.h"
#endif



BEGIN_CPPSDK_NAMESPACE

CURLSH * thread_local_share() {
    static CurlGlobal g_curl_global(CURL_GLOBAL_ALL);
    return g_curl_global.thread_local_share();
}


static int curl_debug(CURL *handle, curl_infotype type, char *data, size_t size, void *userp) {
    const char *text;
    (void) handle; /* prevent compiler warning */
    (void) userp;

    switch (type) {
        case CURLINFO_TEXT:
            text = "== Info";
            if (size >= 1) {
                size -= 1;
            }
            break;
        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            size -= 2;
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            size -= 2;
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            return 0;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            return 0;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            return 0;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            return 0;
        default: /* in case a new one is introduced to shock us */
            return 0;
    }
    std::string info(data, size);
    LOG(DEBUG) << "curl:" << handle << " " << text << ", " << info;
    return 0;
}

static int curl_multi_select(CURLM *curl_m) {
    fd_set fd_read;
    fd_set fd_write;
    fd_set fd_except;
    int max_fd = -1;

    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_except);

    long timeo_ms = -1;
    curl_multi_timeout(curl_m, &timeo_ms);
    if (timeo_ms < 0) {
        timeo_ms = 1000;
    }
    struct timeval tv;
    tv.tv_sec = timeo_ms / 1000;
    tv.tv_usec = (timeo_ms % 1000) * 1000;

    int ret = curl_multi_fdset(curl_m, &fd_read, &fd_write, &fd_except, &max_fd);
    if (ret != 0) {
        LOG(WARN) << "curl multi fdset failed: " << ret;
        return -1;
    }
    int n = ::select(max_fd + 1, &fd_read, &fd_write, &fd_except, &tv);
    if (n < 0) {
        LOG(WARN) << "select failed, errno: " << errno;
        return -1;
    }
    return 0;
}

void *HttpClient::prepare_curl(HttpRequest &request, HttpResponse *response, MyData* data) {
    CURL *curl_handle = curl_easy_init();
    if (curl_handle == NULL) {
        LOG(WARN) << "curl_handle is empty";
        return NULL;
    }
    return prepare_curl(curl_handle, request, response,data);
}

void *HttpClient::prepare_curl(void *curl_handle, HttpRequest &request, HttpResponse *response, MyData* data) {
    std::string url = request.generate_url();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    
    if (data == NULL){
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    }
    else{
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, data->callback);
        curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, data->userData);
    }
    
    // prevent core dump when used in multi-thread application 
    // for the case the libcurl is not built with c-ares
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_stream);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, response);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, recv_header_line);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, request.get_timeout());
    curl_easy_setopt(curl_handle, CURLOPT_SHARE, thread_local_share());
    curl_easy_setopt(curl_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    if (LogUtil::should_log(SDK_LOG_DEBUG)) {
        curl_easy_setopt(curl_handle, CURLOPT_DEBUGFUNCTION, curl_debug);
        curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    }

    InputStream *req_stream = request.get_input_stream();
    int64_t body_size = 0;
    if (req_stream != NULL) {
        body_size = req_stream->get_size();
    }

    switch (request.method()) {
    case HTTP_METHOD_PUT:
        curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1);
        curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_stream);
        curl_easy_setopt(curl_handle, CURLOPT_READDATA, req_stream);
        curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, body_size);
        break;
    case HTTP_METHOD_DELETE:
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    case HTTP_METHOD_HEAD:
        curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);
        break;
    case HTTP_METHOD_POST:
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, body_size);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_stream);
        curl_easy_setopt(curl_handle, CURLOPT_READDATA, req_stream);
        break;
    default:
        // http GET
        break;
    }
    request.append_header("Expect:");
    
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, request.header_list());
    return curl_handle;
}

int HttpClient::execute(HttpRequest &request, HttpResponse *response, MyData* data) {
    int64_t start_time = TimeUtil::now_ms();
    CURL *curl = prepare_curl(request, response, data);
    if (curl == NULL) {
        return RET_INIT_CURL_FAIL;
    }
    CURLcode ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    int64_t cost = TimeUtil::now_ms() - start_time;
    if (ret != 0) {
        LOG(WARN) << "curl:" << curl << " request failed, returns:(" << ret << ")"
            << curl_easy_strerror(ret) << ' ' << request.to_string() << " cost:" << cost;
        return RET_CLIENT_ERROR;
    }
    LOG(INFO) << "curl:" << curl << " request finished, " << request.to_string()
        << " status:" << response->status_code() << " cost:" << cost << "ms";
    return 0;
}

size_t HttpClient::write_stream(void *ptr, size_t size, size_t nmemb, void *user_data) {
    HttpResponse* response = (HttpResponse *) user_data;
    return response->write_body(ptr, size * nmemb);
}

size_t HttpClient::read_stream(char *buffer, size_t size, size_t nmemb, void *stream) {
    if (stream == NULL) {
        return 0;
    }
    InputStream *reader = (InputStream *) stream;
    size_t ret = (size_t)reader->read(buffer, size * nmemb);
   // LOG(DEBUG) << "read_stream,ret:" << ret;
    return ret;
}

// The header callback will be called once for each header
//     and only complete header lines are passed on to the callback
size_t HttpClient::recv_header_line(void* ptr, size_t size, size_t nmemb, void* user_data) {
    HttpResponse* response = (HttpResponse *) user_data;
    if (response->write_header(std::string((char *)ptr, size * nmemb - 2))) {
        return 0;
    }
    return size * nmemb;
}

//int HttpClient::progress_callback(void* client_p, curl_off_t dl_total, curl_off_t dl_now, curl_off_t ul_total, curl_off_t ul_now)
//{
//    LOG(DEBUG) << "progress_callback,dl_total:" << dl_total<<",dl_now:"<< dl_now<<",ul_total:"<< ul_total << ",ul_now:"<< ul_now;
//    return 0;
//}

HttpReactor::HttpReactor() {
    _mcurl = curl_multi_init();
    _detach_context_callback = NULL;
}
HttpReactor::~HttpReactor() {
    if (_mcurl != NULL) {
        for (HandleContextMap::iterator it = _handle_ctx.begin(); it != _handle_ctx.end();) {
            if (it->second != NULL) {
                curl_multi_remove_handle(_mcurl, it->first);
                if (_detach_context_callback != NULL) {
                    _detach_context_callback(it->second);
                }
            }
            curl_easy_cleanup(it->first);
            _handle_ctx.erase(it++);
        }
        curl_multi_cleanup(_mcurl);
    }
}

int HttpReactor::execute(HttpRequestContext *ctx) {
    ctx->timestamp_ms = TimeUtil::now_ms();
    CURL *curl = NULL;
    if (_idle_handles.empty()) {
        curl = _client.prepare_curl(*ctx->request, ctx->response);
        if (curl == NULL) {
            LOG(ERROR) << "init curl failed";
            return RET_CLIENT_ERROR;
        }
    } else {
        curl = _idle_handles.front();
        _client.prepare_curl(curl, *ctx->request, ctx->response);
        _idle_handles.pop_front();
    }
    _handle_ctx[curl] = ctx;
    int ret = curl_multi_add_handle(_mcurl, curl);
    if (ret != 0) {
        LOG(ERROR) << "multi curl add handle failed, ret:(" << ret << ')'
            << curl_easy_strerror((CURLcode) ret);
    }
    return ret;
}

HttpRequestContext *HttpReactor::perform() {
    int running_handles = 0;
    while (curl_multi_perform(_mcurl, &running_handles) == CURLM_CALL_MULTI_PERFORM) {
    }
    int msgs_left;
    CURLMsg *msg = curl_multi_info_read(_mcurl, &msgs_left);
    if (msg == NULL) {
        return NULL;
    }
    if (msg->msg != CURLMSG_DONE) {
        LOG(ERROR) << "unexpected curl msg type: " << msg->msg;
        return NULL;
    }
    CURL *curl = msg->easy_handle;
    int rc = msg->data.result;
    if (rc != 0) {
        LOG(WARN) << "curl " << curl << " failed, code: (" << rc << ')'
            << curl_easy_strerror((CURLcode) rc);
    }
    // will destroy msg
    curl_multi_remove_handle(_mcurl, curl);
    HandleContextMap::iterator it = _handle_ctx.find(curl);
    if (it == _handle_ctx.end()) {
        LOG(ERROR) << "unexpected curl handle finished: " << curl;
        curl_easy_cleanup(curl);
        return 0;
    }
    curl_easy_reset(curl);
    _idle_handles.push_back(curl);
    HttpRequestContext *ctx = it->second;
    it->second = NULL;
    ctx->rc = rc;
    int64_t cost = TimeUtil::now_ms() - ctx->timestamp_ms;
    if (rc == 0) {
        LOG(INFO) << "curl:" << curl << " request finished, " << ctx->request->to_string()
            << " status:" << ctx->response->status_code() << " cost:" << cost << "ms";
    }
    return ctx;
}

void HttpReactor::wait() {
    curl_multi_select(_mcurl);
}

END_CPPSDK_NAMESPACE

