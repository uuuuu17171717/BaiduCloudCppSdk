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
#ifndef BAIDU_BOS_CPPSDK_BOS_REQUEST_PUT_OBJECT_REQUEST_H
#define BAIDU_BOS_CPPSDK_BOS_REQUEST_PUT_OBJECT_REQUEST_H

#include "bcesdk/bos/model/object_meta_request.h"
#include "bcesdk/bos/model/upload_request.h"

BEGIN_CPPSDK_NAMESPACE

class PutObjectRequest : public ObjectMetaRequest, public UploadRequest {
public:
    PutObjectRequest(const std::string &bucket_name, const std::string &object_name) :
            ObjectMetaRequest(bucket_name, object_name) {}
    PutObjectRequest(const std::string &bucket_name, const std::string &object_name,
            const std::string &data) : ObjectMetaRequest(bucket_name, object_name) {
        set_data(data);
    }
    PutObjectRequest(const std::string &bucket_name, const std::string &object_name,
            InputStream *stream) : ObjectMetaRequest(bucket_name, object_name) {
        set_stream(stream);
    }

    virtual ~PutObjectRequest() {}

    virtual int build_command_specific(HttpRequest *request) {
        request->set_method(HTTP_METHOD_PUT);
        append_all_meta_to(request);
        return UploadRequest::fill_http_request(request);
    }
};

END_CPPSDK_NAMESPACE
#endif

