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
#ifndef BAIDU_BOS_CPPSDK_BOS_CLIENT_OPTIONS_H
#define BAIDU_BOS_CPPSDK_BOS_CLIENT_OPTIONS_H

#include <string>

#include "bcesdk/common/common.h"

BEGIN_CPPSDK_NAMESPACE

struct ClientOptions {
    enum auth_version_t {
        BOS_COMPATIBLE_AUTH,
        BCE_AUTH
    };

    ClientOptions() : endpoint("http://bj.bcebos.com"), sign_expire_seconds(1200),
            timeout(120), retry(2), max_parallel(10), multi_part_size(10485760), calc_md5_on(false) {}

    std::string endpoint;

    std::string user_agent;

    int sign_expire_seconds;

    // http request timeout in seconds
    int32_t timeout;
    int retry;
    int max_parallel;

    int multi_part_size;
    //this is a switch to decide whether to calculate_md5 or not
    bool calc_md5_on;
};

END_CPPSDK_NAMESPACE
#endif

