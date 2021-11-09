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
#ifndef BAIDU_BOS_CPPSDK_BOS_REQUEST_HTTP_REQUEST_H
#define BAIDU_BOS_CPPSDK_BOS_REQUEST_HTTP_REQUEST_H

#include <string>
#include <stdint.h>

#include "bcesdk/common/common.h"

struct curl_slist;

BEGIN_CPPSDK_NAMESPACE

enum HttpMethod {
    HTTP_METHOD_INVALID,
    HTTP_METHOD_PUT,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_DELETE
};

static inline const char *method_str(HttpMethod method) {
    switch (method) {
    case HTTP_METHOD_PUT:
        return "PUT";
    case HTTP_METHOD_GET:
        return "GET";
    case HTTP_METHOD_POST:
        return "POST";
    case HTTP_METHOD_HEAD:
        return "HEAD";
    case HTTP_METHOD_DELETE:
        return "DELETE";
    default:
        return "UNKNOWN";
    }
}

class InputStream;

class HttpRequest {
public:
    HttpRequest() : _header_list(NULL) {
        _method = HTTP_METHOD_INVALID;
        _in_stream = NULL;
        _timeout = 30;
    }
    ~HttpRequest();

    void set_input_stream(InputStream *in_stream)
    {
        _in_stream = in_stream;
    }

    InputStream * get_input_stream() const
    {
        return _in_stream;
    }

    void set_method(HttpMethod method) {
        _method = method;
    }

    HttpMethod method() const {
        return _method;
    }

    void append_header(const char *header_line);
    void append_header(const std::string &key, const std::string &val);
    void append_header(const std::string &key, long long val);

    void add_parameter(const std::string &key, const std::string &value) {
        _parameters[key] = value;
    }
    void add_parameter(const std::string &flag) {
        _parameters[flag] = "";
    }
    void add_parameter(const std::string &key, int64_t value);

    const StringMap &parameters() const {
        return _parameters;
    }

    const struct curl_slist *header_list() const {
        return _header_list;
    }

    int get_timeout() const
    {
        return _timeout;
    }

    void set_timeout(int timeout)
    {
        _timeout = timeout;
    }

    void set_endpoint(const std::string &endpoint) {
        size_t pos = endpoint.find("://");
        if (pos == std::string::npos) {
            _protocol = "http";
            _host = endpoint;
        } else {
            _protocol = endpoint.substr(0, pos);
            _host = endpoint.substr(pos + 3);
        }
        pos = _host.find('/');
        if (pos != std::string::npos) {
            _host = _host.substr(0, pos);
        }
    }

    const std::string &host() const {
        return _host;
    }

    void set_uri(const std::string &uri) {
        _uri = uri;
    }
    const std::string &uri() const {
        return _uri;
    }

    std::string generate_url() const;

    std::string to_string() {
        return std::string(method_str(_method)) + " " + generate_url();
    }

    // reset to state before build that can rebuild again
    void reset();
private:
    InputStream *                      _in_stream;
    std::string _endpoint;
    std::string _protocol;
    std::string _host;
    std::string _uri;
    struct curl_slist *_header_list;
    StringMap _parameters;
    HttpMethod                  _method;
    // seconds
    int                                _timeout;
};

END_CPPSDK_NAMESPACE
#endif

