#ifndef BOS_API_TEST_H
#define BOS_API_TEST_H
#define BAIDU_BOS_CPPSDK_BOS_RESPONSE_LIST_PARTS_RESPONSE_H

#include <string>
#include "bcesdk/bos/client.h"

bool test_object_api(baidu::bos::cppsdk::Client &client, const std::string & temp_dir,
    const std::string &bucket, long long file_size = -1);
bool test_bucket_api(baidu::bos::cppsdk::Client &client);
bool test_multipart_api(baidu::bos::cppsdk::Client &client, const std::string & temp_dir,
    const std::string &bucket);
bool test_large_file(baidu::bos::cppsdk::Client &client, const std::string & temp_dir, 
    const std::string &bucket);

#endif