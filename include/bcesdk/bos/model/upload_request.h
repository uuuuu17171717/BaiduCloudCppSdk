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
#ifndef BAIDU_BOS_CPPSDK_BOS_MODEL_UPLOAD_REQUEST_H
#define BAIDU_BOS_CPPSDK_BOS_MODEL_UPLOAD_REQUEST_H

#include "bcesdk/common/common.h"
#include "bcesdk/common/stream.h"
#include "bcesdk/http/http_request.h"
#include "bcesdk/common/memory_stream.h"

BEGIN_CPPSDK_NAMESPACE

class UploadRequest {
public:
    UploadRequest() : _stream(NULL), _is_stream_owner(false), _calc_md5_on(false) {}
    virtual ~UploadRequest() {
        release_stream();
    }

    InputStream *detach_stream() {
        InputStream *tmp = _stream;
        _stream = NULL;
        _is_stream_owner = false;
        return tmp;
    }
    void set_calc_md5_on(bool on) {
        _calc_md5_on = on;
    }
    void set_stream(InputStream *stream, bool is_owner) {
        release_stream();
        _stream = stream;
        _is_stream_owner = is_owner;
    }

    void release_stream() {
        if (_is_stream_owner && _stream != NULL) {
            delete _stream;
        }
        _stream = NULL;
        _is_stream_owner = false;
    }

    inline void set_stream(InputStream *stream) {
        set_stream(stream, false);
    }

    inline void set_allocated_stream(InputStream *stream) {
        set_stream(stream, true);
    }

    inline void set_data(const std::string &data) {
        set_stream(new MemoryInputStream(data.data(), data.size()), true);
    }

    int calculate_md5(std::string *digest);
    int calculate_sha256(std::string *digest);

    int fill_http_request(HttpRequest *request);

private:
    InputStream * _stream;
    bool          _is_stream_owner;
    bool          _calc_md5_on;
};

END_CPPSDK_NAMESPACE
#endif

