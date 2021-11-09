#include "bcesdk/bos/client.h"
#include <fcntl.h>
#include <stdio.h>
#include "api_test.h"
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ole32.lib")

using namespace baidu::bos::cppsdk;

std::string bucket = "mybucket";
std::string object = "myobject";

void print_common_response(BceResponse &result) {
    printf("status:%d\n", result.status_code());
    if (result.is_ok()) {
        printf("request-id:%s\n", result.request_id().c_str());
        printf("debug-id:%s\n", result.debug_id().c_str());
    }
    if (result.is_fail()) {
        printf("error-message:%s\n", result.error().message().c_str());
    }
}

void list_buckets(Client &client) {
    ListBucketsRequest request;
    ListBucketsResponse response;
    client.list_buckets(request, &response);
}

void test_bucket_acl(Client &client) {
    GetBucketAclRequest request(bucket);
    GetBucketAclResponse response;
    client.get_bucket_acl(request, &response);
    print_common_response(response);
}

void test_bucket_location(Client &client) {
    GetBucketLocationRequest request(bucket);
    GetBucketLocationResponse response;
    client.get_bucket_location(request, &response);
    print_common_response(response);
    printf("location:%s\n", response.location().c_str());
}

void test_list_objects(Client &client) {
    ListObjectsRequest request(bucket);
    ListObjectsResponse result;
    client.list_objects(request, &result);
    print_common_response(result);
    if (result.is_fail()) {
        return;
    }
    std::vector<ObjectSummary> &summary = result.contents();
    for (size_t i = 0; i < summary.size(); ++i) {
        printf("%s %s\n", summary[i].key.c_str(), summary[i].storage_class.c_str());
    }
    for (size_t i = 0; i < result.common_prefixes().size(); ++i) {
        printf("dir %s\n", result.common_prefixes().at(i).c_str());
    }
}

void test_head_object(Client &client) {
    HeadObjectRequest request(bucket, object);
    HeadObjectResponse result;
    int ret = client.head_object(request, &result);
    printf("ret:%d\n", ret);
    print_common_response(result);
    printf("content-type: %s\n", result.meta().content_type().c_str());
    printf("content-disposition: %s\n", result.meta().content_disposition().c_str());
    printf("content-encoding: %s\n", result.meta().content_encoding().c_str());
    printf("last-modified: %ld\n", (long)result.meta().last_modified());
    printf("expires: %ld\n", (long)result.meta().expires());
    printf("storage class: %s\n", result.meta().storage_class().c_str());
    printf("etag: %s\n", result.meta().etag().c_str());
    printf("x-bce-meta-uid: %s\n", result.meta().user_meta("uid").c_str());
    printf("x-bce-next-append-offset: %llu\n", result.meta().next_append_offset());
}

void test_put_object(Client &client) {
    std::string filename = object;
    FileInputStream file(filename);
    PutObjectRequest request(bucket, object, &file);
    request.mutable_meta()->set_storage_class("STANDARD_IA");
    PutObjectResponse result;
    client.put_object(request, &result);
    print_common_response(result);
    printf("etag: %s\n", result.etag().c_str());
}

void test_append_object(Client &client) {
    std::string filename = object;

    uint64_t off = 0;
    {
        // append first
        FileInputStream file(filename);
        AppendObjectRequest request(bucket, object, &file);
        AppendObjectResponse result;

        client.append_object(request, &result);
        print_common_response(result);
        printf("etag: %s\n", result.etag().c_str());
        printf("x-bce-next-append-offset: %llu\n", result.next_append_offset());
        off = result.next_append_offset();
    }
    {
        // append again
        FileInputStream file(filename);
        AppendObjectRequest request(bucket, object, &file);
        AppendObjectResponse result;
        request.set_offset(off);

        client.append_object(request, &result);
        print_common_response(result);
        printf("etag: %s\n", result.etag().c_str());
        printf("x-bce-next-append-offset: %llu\n", result.next_append_offset());
    }
}


void test_abort(Client &client) {
    ListMultipartUploadsRequest request(bucket);
    ListMultipartUploadsResponse result;
    client.list_multipart_uploads(request, &result);
    print_common_response(result);
    if (result.is_fail()) {
        return;
    }
    std::vector<MultipartUploadSummary> &uploads = result.uploads();
    for (size_t i = 0; i < uploads.size(); ++i) {
        printf("key: %s\n", uploads[i].key.c_str());
        AbortMultipartUploadRequest abort_req(bucket, uploads[i].key, uploads[i].upload_id);
        AbortMultipartUploadResponse abort_res;
        client.abort_multipart_upload(abort_req, &abort_res);
        printf("abort status::%d\n", abort_res.status_code());
    }
}

void test_copy(Client &client) {
    std::string dst_bucket = bucket;
    std::string dst_object = object + ".bak";
    CopyObjectRequest request(dst_bucket, dst_object, bucket, object);
    request.mutable_meta()->set_storage_class("STANDARD");
    CopyObjectResponse response;
    client.copy_object(request, &response);
    print_common_response(response);
    if (response.is_fail()) {
        return;
    }
    printf("etag: %s\n", response.etag().c_str());
    printf("last_modified: %ld\n", (long)response.last_modified());
}

void test_delete_object(Client &client) {
    DeleteObjectRequest request(bucket, object);
    DeleteObjectResponse response;
    client.delete_object(request, &response);
    print_common_response(response);
}

void test_download(Client &client) {
    std::string filename = object;
    FileOutputStream file(filename);
    GetObjectRequest request(bucket, object);
    GetObjectResponse response(&file);
    client.get_object(request, &response);
    print_common_response(response);
}

void test_multiple_download(Client &client) {
    std::string filename = object;
    FileOutputStream file(filename);
    int ret = client.parallel_download(bucket, object, file);
    printf("ret:%d\n", ret);
}

void test_multiple_upload(Client &client) {
    std::string filename = object;
    FileInputStream file(filename);
    ObjectMetaData meta;
    meta.set_storage_class("STANDARD_IA");
    int ret = client.parallel_upload(bucket, object, file, &meta);
    printf("ret:%d\n", ret);
}

void test_parallel_copy(Client &client) {
    std::string dst_bucket = bucket;
    std::string dst_object = object + ".bak";
    int ret = client.parallel_copy(bucket, object, dst_bucket, dst_object);
    printf("ret:%d\n", ret);
}

int main() {
    FILE *logfp = fopen("sdk.log", "w");
    sdk_set_log_stream(logfp);
    sdk_set_log_level(SDK_LOG_DEBUG);

    ClientOptions option;
    option.max_parallel = 10;
    option.endpoint = "http://bj.bcebos.com";

    std::string ak;
    std::string sk;
    Client client(ak, sk, option);

    //test_put_object(client);
    //test_copy(client);
    //test_delete_object(client);
    //test_list_objects(client);
    //test_append_object(client);
    //test_head_object(client);
    //test_abort(client);
    //test_multiple_download(client);
    //test_download(client);
    //test_multiple_upload(client);
    //test_bucket_acl(client);
    //list_buckets(client);
    //test_parallel_copy(client);
    //test_bucket_location(client);

    //test more bos api
    //srand((unsigned int)time(NULL));
    //std::string temp_dir = "/tmp";
    //std::string  test_bucket = "your-test-bucket";
    //if (false == test_object_api(client, temp_dir, test_bucket)) {
    //    printf("test_object_api fail!");
    //    return -1;
    //}
    //if (false == test_bucket_api(client)) {
    //    printf("test_bucket_api fail!");
    //    return -1;
    //}
    //if (false == test_multipart_api(client, temp_dir, test_bucket)) {
    //    printf("test_multipart_api fail!");
    //    return -1;
    //}
    //if (false == test_large_file(client, temp_dir, test_bucket) ){
    //    printf("test_large_file fail!");
    //    return -1;
    //}
    return 0;
}
