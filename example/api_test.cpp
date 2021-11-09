#include "bcesdk/bos/client.h"
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <assert.h>
#include "api_test.h"

using namespace baidu::bos::cppsdk;
template <class T, int SizeOfArray>
void delete_array_items(T * (&array)[SizeOfArray]) {
    for (auto & item : array) {
        delete item;
    }
}

inline unsigned int rand32() {
    return (unsigned int)((int)rand() << 16) | rand();
}

class HandlerBase {
public:
    virtual ~HandlerBase() {}
    virtual std::string get_name() {
        return typeid(*this).name();
    }
};

class IUploadHandler : public HandlerBase {
public:
    virtual int upload(Client &client, const std::string &bucket,
        const std::string &object,
        const std::string &file_path) = 0;
};

class IDownloadHandler : public HandlerBase {
public:
    virtual int download(Client &client, const std::string &bucket,
        const std::string &object,
        const std::string &save_path) = 0;
};

class ICopyHandler : public HandlerBase {
public:
    virtual int copy(Client &client, const std::string &src_bucket,
        const std::string &src_object,
        const std::string &dst_bucket,
        const std::string &dst_object) = 0;
};

class put_object1 :public IUploadHandler {
    virtual int upload(Client &client, const std::string &bucket,
        const std::string &object, const std::string &file_path) {
        FileInputStream file(file_path);
        PutObjectRequest request(bucket, object, &file);
        PutObjectResponse result;
        return client.put_object(request, &result);
    }
};

class put_object2 :public IUploadHandler {
    virtual int upload(Client &client, const std::string &bucket,
        const std::string &object, const std::string &file_path) {
        std::fstream f;
        f.open(file_path, std::ios_base::in | std::ios_base::binary);
        if (false == f.is_open()) {
            assert(!"open file fail");
            return -1;
        }
        f.seekg(0, std::ios::end);
        size_t size = (size_t)f.tellg();
        std::string put_data;
        put_data.resize(size);
        f.seekg(0, std::ios::beg);
        f.read((char *)put_data.data(), size);
        f.close();
        return client.put_object(bucket, object, put_data);
    }
};

class upload_file1 :public IUploadHandler {
    virtual int upload(Client &client, const std::string &bucket,
        const std::string &object, const std::string &file_path) {
        FileInputStream file(file_path);
        return client.upload_file(bucket, object, file);
    }
};

class upload_file2 :public IUploadHandler {
    virtual int upload(Client &client, const std::string &bucket,
        const std::string &object, const std::string &file_path) {
        return client.upload_file(bucket, object, file_path);
    }
};

class parallel_upload1 :public IUploadHandler {
    virtual int upload(Client &client, const std::string &bucket,
        const std::string &object, const std::string &file_path) {
        FileInputStream file(file_path);
        return client.parallel_upload(bucket, object, file);
    }
};

class multi_upload :public IUploadHandler {
    virtual int upload(Client &client, const std::string &bucket,
        const std::string &object, const std::string &file_path) {
        std::fstream f(file_path, std::ios_base::in | std::ios_base::binary);
        InitMultiUploadRequest init_request(bucket, object);
        InitMultiUploadResponse init_response;
        int ret = client.init_multipart_upload(init_request, &init_response);
        if (ret != 0) {
            printf("init_multipart_upload fail, ret:%d\n", ret);
            return ret;
        }
        f.seekg(0, std::ios::end);
        long long size = f.tellg();
        f.seekg(0, std::ios::beg);
        const int part_size = 1024 * 1024 * 1;

        CompleteMultipartUploadRequest complete_request(bucket, object,
            init_response.upload_id());

        int part_count = (int)(size / part_size);
        std::string upload_part_data;
        upload_part_data.resize(part_size);
        for (int i = 0; i < part_count; i++) {
            f.read((char *)upload_part_data.data(), part_size);
            UploadPartRequest upload_part_request(bucket, object, upload_part_data,
                i + 1, init_response.upload_id());
            UploadPartResponse upload_part_response;
            int ret = client.upload_part(upload_part_request, &upload_part_response);
            if (ret != 0) {
                printf("upload_part fail, ret:%d\n", ret);
                return ret;
            }
            complete_request.add_part(i + 1, upload_part_response.etag());
        }
        int residual = size % part_size;
        if (residual != 0) {
            upload_part_data.resize(residual);
            f.read((char *)upload_part_data.data(), residual);
            UploadPartRequest upload_part_request(bucket, object, upload_part_data,
                part_count + 1, init_response.upload_id());
            UploadPartResponse upload_part_response;
            int ret = client.upload_part(upload_part_request, &upload_part_response);
            if (ret != 0) {
                printf("upload_part fail, ret:%d\n", ret);
                return ret;
            }
            complete_request.add_part(part_count + 1, upload_part_response.etag());
        }

        CompleteMultipartUploadResponse complete_response;
        ret = client.complete_multipart_upload(complete_request, &complete_response);
        if (ret != 0) {
            printf("complete_multipart_upload fail, ret:%d\n", ret);
            return ret;
        }
        return ret;
    }
};


class get_object1 :public IDownloadHandler {
    virtual int download(Client &client, const std::string &bucket,
        const std::string &object, const std::string &save_path) {
        FileOutputStream file_output_stream(save_path);
        GetObjectRequest request(bucket, object);
        GetObjectResponse response(&file_output_stream);
        return client.get_object(request, &response);
    }
};

class get_object2 :public IDownloadHandler {
    virtual int download(Client &client, const std::string &bucket,
        const std::string &object, const std::string &save_path) {
        std::string data;
        int ret = client.get_object(bucket, object, &data);
        std::fstream f;
        f.open(save_path, std::ios_base::out | std::ios_base::binary);
        if (false == f.is_open()) {
            assert(!"open file fail");
            return -1;
        }
        f.write((const char *)data.data(), data.size());
        f.close();
        return ret;
    }
};


class download_file1 :public IDownloadHandler {
    virtual int download(Client &client, const std::string &bucket,
        const std::string &object, const std::string &save_path) {
        FileOutputStream file_output_stream(save_path);
        HeadObjectRequest request(bucket, object);
        HeadObjectResponse result;
        int ret = client.head_object(request, &result);
        if (ret != 0) {
            printf("head_object fail, ret:%d\n", ret);
            return ret;
        }
        return client.download_file(bucket, object, file_output_stream, 0, result.meta().content_length());
    }
};

class download_file2 :public IDownloadHandler {
    virtual int download(Client &client, const std::string &bucket,
        const std::string &object, const std::string &save_path) {
        HeadObjectRequest request(bucket, object);
        HeadObjectResponse result;
        int ret = client.head_object(request, &result);
        if (ret != 0) {
            printf("head_object fail, ret:%d\n", ret);
            return ret;
        }
        return client.download_file(bucket, object, save_path, 0, result.meta().content_length());
    }
};

class parallel_download1 :public IDownloadHandler {
    virtual int download(Client &client, const std::string &bucket,
        const std::string &object, const std::string &save_path) {
        FileOutputStream file_output_stream(save_path);
        return client.parallel_download(bucket, object, file_output_stream);
    }
};

class copy_object1 : public ICopyHandler {
    virtual int copy(Client &client, const std::string &src_bucket,
        const std::string &src_object,
        const std::string &dst_bucket,
        const std::string &dst_object) {
        CopyObjectRequest request(dst_bucket, dst_object, src_bucket, src_object);
        request.mutable_meta()->set_storage_class("STANDARD");
        CopyObjectResponse response;
        return client.copy_object(request, &response);
    }
};

class copy_object2 : public ICopyHandler {
    virtual int copy(Client &client, const std::string &src_bucket,
        const std::string &src_object,
        const std::string &dst_bucket,
        const std::string &dst_object) {
        return client.copy_object(src_bucket, src_object, dst_bucket, dst_object, "STANDARD");
    }
};

class multi_part_copy :public ICopyHandler {
public:
    virtual int copy(Client &client, const std::string &src_bucket,
        const std::string &src_object,
        const std::string &dst_bucket,
        const std::string &dst_object) {
        InitMultiUploadRequest init_request(dst_bucket, dst_object);
        InitMultiUploadResponse init_response;
        int ret = client.init_multipart_upload(init_request, &init_response);
        if (ret != 0) {
            printf("init_multipart_upload fail, ret:%d\n", ret);
            return ret;
        }
        const int part_size = 1024 * 1024 * 1;


        HeadObjectRequest head_request(src_bucket, src_object);
        HeadObjectResponse head_result;
        ret = client.head_object(head_request, &head_result);
        if (ret != 0) {
            printf("head_object fail, ret:%d\n", ret);
            return ret;
        }

        long long size = head_result.meta().content_length();
        CompleteMultipartUploadRequest complete_request(dst_bucket, dst_object,
            init_response.upload_id());
        int part_count = (int)(size / part_size);
        std::string upload_part_data;
        upload_part_data.resize(part_size);
        for (int i = 0; i < part_count; i++) {
            CopyPartRequest copy_part_request;
            copy_part_request.set_bucket_name(dst_bucket);
            copy_part_request.set_object_name(dst_object);
            copy_part_request.set_source_bucket_name(src_bucket);
            copy_part_request.set_source_object_name(src_object);
            copy_part_request.set_range(i * part_size, (i + 1) * part_size - 1);
            copy_part_request.set_part_number(i + 1);
            copy_part_request.set_upload_id(init_response.upload_id());
            CopyPartResponse copy_part_response;
            ret = client.copy_part(copy_part_request, &copy_part_response);
            if (ret != 0) {
                printf("copy_part fail, ret:%d\n", ret);
                return ret;
            }
            complete_request.add_part(i + 1, copy_part_response.etag());
        }
        int residual = size % part_size;
        if (residual != 0) {
            CopyPartRequest copy_part_request;
            copy_part_request.set_bucket_name(dst_bucket);
            copy_part_request.set_object_name(dst_object);
            copy_part_request.set_source_bucket_name(src_bucket);
            copy_part_request.set_source_object_name(src_object);
            copy_part_request.set_range(part_count * part_size, size - 1);
            copy_part_request.set_part_number(part_count + 1);
            copy_part_request.set_upload_id(init_response.upload_id());
            CopyPartResponse copy_part_response;
            ret = client.copy_part(copy_part_request, &copy_part_response);
            complete_request.add_part(part_count + 1, copy_part_response.etag());
            if (ret != 0) {
                printf("add_part fail, ret:%d\n", ret);
                return ret;
            }
        }

        CompleteMultipartUploadResponse complete_response;
        ret = client.complete_multipart_upload(complete_request, &complete_response);
        if (ret != 0) {
            printf("complete_multipart_upload fail, ret:%d\n", ret);
            return ret;
        }
        return ret;
    }
};

class parallel_copy1 : public ICopyHandler {
    virtual int copy(Client &client, const std::string &src_bucket,
        const std::string &src_object,
        const std::string &dst_bucket,
        const std::string &dst_object) {
        return client.parallel_copy(src_bucket, src_object, dst_bucket, dst_object, "STANDARD");
    }
};

bool compare_file(const std::string & file_path1, const std::string & file_path2) {
    std::ifstream f1(file_path1, std::ios_base::binary);
    std::ifstream f2(file_path2, std::ios_base::binary);
    if (false == f1.is_open() || false == f2.is_open()) {
        assert(!"open file fail");
        return false;
    }
    f1.seekg(0, std::ios::end);
    f2.seekg(0, std::ios::end);
    long long s1 = f1.tellg();
    long long s2 = f2.tellg();

    if (s1 == 0 || s1 != s2) {
        return false;
    }
    f1.seekg(0, std::ios::beg);
    f2.seekg(0, std::ios::beg);
    const int block_size = 1024 * 4;
    std::vector<char> buffer1;
    std::vector<char> buffer2;
    buffer1.resize(block_size);
    buffer2.resize(block_size);
    for (long long i = 0; i < s1; i += block_size) {
        f1.read(&buffer1.front(), block_size);
        f2.read(&buffer2.front(), block_size);
        if (0 != memcmp(&buffer1.front(), &buffer2.front(), block_size)) {
            return false;
        }
    }
    int residual = s1 % block_size;
    if (residual != 0) {
        f1.read(&buffer1.front(), residual);
        f2.read(&buffer2.front(), residual);
        if (0 != memcmp(&buffer1.front(), &buffer2.front(), residual)) {
            return false;
        }
    }
    f1.close();
    f2.close();
    return true;
}

void gen_file(const std::string & file_path, long long file_size) {
    std::ofstream f;
    f.open(file_path, std::ios_base::binary);
    std::vector<int> buffer;
    buffer.reserve(1024);
    for (long long j = 0; j < 1024; j++) {
        buffer.push_back(rand32());
    }
    size_t block_size = buffer.size() * sizeof(buffer.front());
    long long count = file_size / block_size;
    for (long long j = 0; j < count; j++) {
        f.write((const char *)&buffer.front(), block_size);
        std::random_shuffle(buffer.begin(), buffer.end());
    }
    if (file_size % (1024 * 4) != 0) {
        f.write((const char *)&buffer.front(), file_size % (1024 * 4));
    }
    f.close();
}


bool test_object_api_internal(baidu::bos::cppsdk::Client &client, const std::string & temp_dir, const std::string &bucket, long long file_size,
    IUploadHandler * upload_handler, IDownloadHandler * download_handler, ICopyHandler * copy_handler) {
    std::string src_file = "cpp_sdk_test_upload_file.dat";
    std::string dst_copy_file = "cpp_sdk_test_copy_file.dat";
    std::string download_file = "cpp_sdk_test_download_file.dat";

    std::string file_op_put_path(temp_dir + "/" + src_file);
    std::string file_op_get_path(temp_dir + "/" + download_file);

    // -1 == random size
    if (file_size == -1) {
        file_size = rand32() % (3 * 1024 * 1024) + 1;
    }
    printf("test file size:%lld\n", file_size);

    gen_file(file_op_put_path, file_size);
    bool final_result = false;
    do {
        {   //put
            int ret = upload_handler->upload(client, bucket, src_file, file_op_put_path);
            printf("%s::upload ret:%d\n", upload_handler->get_name().c_str(), ret);
            if (ret != 0) {
                break;
            }
        }

        {  //head
            HeadObjectRequest request(bucket, src_file);
            HeadObjectResponse result;
            int ret = client.head_object(request, &result);
            printf("head_object ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
            if (result.meta().content_length() != file_size) {
                printf("head_file got:%lld, want:%lld", file_size, result.meta().content_length());
                break;
            }
        }

        {  //copy
            int ret = copy_handler->copy(client, bucket, src_file, bucket, dst_copy_file);
            printf("%s::copy ret:%d\n", copy_handler->get_name().c_str(), ret);
            if (ret != 0) {
                break;
            }
        }


        { //list_objects
            ListObjectsRequest request(bucket);
            request.set_marker("cpp_sdk_test_");
            ListObjectsResponse response;
            int ret = client.list_objects(request, &response);
            printf("list_objects ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
            int match_count = 0;
            for (auto & object_item : response.contents()) {
                if (object_item.key == src_file || object_item.key == dst_copy_file) {
                    match_count++;
                }
            }
            if (match_count != 2) {
                printf("list object not expected");
                break;
            }
        }


        {  //get
            int ret = download_handler->download(client, bucket, dst_copy_file, file_op_get_path);
            printf("%s::download ret:%d\n", download_handler->get_name().c_str(), ret);
            if (ret != 0) {
                break;
            }
        }

        { //delete
            DeleteObjectRequest request(bucket, src_file);
            DeleteObjectResponse response;
            int ret = client.delete_object(request, &response);
            printf("delete_object object:%s, ret:%d\n", src_file.c_str(), ret);
            int ret2 = client.delete_object(bucket, dst_copy_file);
            printf("delete_object object:%s, ret:%d\n", dst_copy_file.c_str(), ret2);
        }

        if (false == compare_file(file_op_put_path, file_op_get_path)) {
            printf("compare_file fail\n");
            break;
        }
        printf("object basic operation :ok\n");
        printf("----------------------------\n");
        final_result = true;
    } while (0);
    remove(file_op_get_path.c_str());
    remove(file_op_put_path.c_str());
    return final_result;
}



bool test_object_api(baidu::bos::cppsdk::Client &client, const std::string & temp_dir, const std::string &bucket, long long file_size) {
    IUploadHandler * upload_handlers[] = {
         new put_object1()
        , new put_object2()
        , new upload_file1()
        , new upload_file2()
        , new parallel_upload1()
        , new multi_upload()
    };

    IDownloadHandler * download_handlers[] = {
        new get_object1()
        , new get_object2()
        , new download_file1()
        , new download_file2()
        , new parallel_download1()
    };

    ICopyHandler * copy_handlers[] = {
         new copy_object1()
        , new copy_object2()
        , new parallel_copy1()
        , new multi_part_copy()
    };

    for (auto & upload_handler : upload_handlers) {
        for (auto & download_handler : download_handlers) {
            for (auto & copy_handler : copy_handlers) {
                bool final_result = test_object_api_internal(client, temp_dir, bucket, file_size,
                    upload_handler, download_handler, copy_handler);
                if (false == final_result) {
                    delete_array_items(upload_handlers);
                    delete_array_items(download_handlers);
                    delete_array_items(copy_handlers);
                    return false;
                }
            }
        }
    }
    printf("test_object_api : ok\n");
    printf("----------------------------\n");
    delete_array_items(upload_handlers);
    delete_array_items(download_handlers);
    delete_array_items(copy_handlers);
    return true;
}

bool test_bucket_api(baidu::bos::cppsdk::Client &client) {
    std::string new_bucket_name;
    std::stringstream ss;
    ss << "cpp-sdk-test-bucket-" << time(NULL) << "-" << rand() << "-" << rand();
    std::string bucket = ss.str();
    bool final_result = false;
    do {
        { //put_bucket
            PutBucketRequest request(bucket);
            PutBucketResponse response;
            int ret = client.put_bucket(request, &response);
            printf("put_bucket ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
        }

        { //get_bucket_location
            GetBucketLocationRequest request(bucket);
            GetBucketLocationResponse response;
            int ret = client.get_bucket_location(request, &response);
            printf("get_bucket_location ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
            std::cout << "bucket location :" << response.location() << std::endl;
        }

        { //head_bucket
            ListBucketsRequest request;
            ListBucketsResponse response;
            int ret = client.list_buckets(request, &response);
            printf("list_buckets ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
            bool new_bucket_exists = false;
            std::cout << "list bucket : " << std::endl;
            for (auto & bucket_item : response.buckets()) {
                if (bucket_item.name == bucket) {
                    new_bucket_exists = true;
                }
                std::cout << "- bucket :" << bucket_item.name
                    << ", " << "create date:" << bucket_item.creation_date
                    << "," << "location:" << bucket_item.location << std::endl;
            }
            if (false == new_bucket_exists) {
                printf("new bucket create fail ?\n");
                break;
            }
        }

        const std::string test_bucket_acl = "public-read";
        { //put_bucket_acl
            PutBucketAclRequest request(bucket, test_bucket_acl);
            PutBucketAclResponse response;
            int ret = client.put_bucket_acl(request, &response);
            printf("put_bucket_acl ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
        }

        { //get_bucket_acl
            GetBucketAclRequest request(bucket);
            GetBucketAclResponse response;
            int ret = client.get_bucket_acl(request, &response);
            printf("get_bucket_acl ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
            bool permission_match = false;
            for (auto & grant : response.access_control_list()) {
                for (size_t i = 0; i < grant.grantee.size(); ++i) {
                    if (grant.grantee[i].id == "*") {
                        if (grant.permission[i] == "READ") {
                            permission_match = true;
                            break;
                        }
                    }
                }
            }
            if (false == permission_match) {
                printf("put_bucket_acl or get_bucket_acl error \n");
                break;
            }
        }

        { //delete_bucket
            DeleteBucketRequest request(bucket);
            DeleteBucketResponse response;
            int ret = client.delete_bucket(request, &response);
            printf("delete_bucket ret:%d\n", ret);
            if (ret != 0) {
                break;
            }
        }
        final_result = true;
    } while (0);
    return final_result;
}

bool test_multipart_api(baidu::bos::cppsdk::Client &client, const std::string & temp_dir, const std::string &bucket) {
    std::string object = "cpp_sdk_test_multipart.dat";
    std::string file_op_put_path(temp_dir + "/" + object);

    int file_size = rand32() % (10 * 1024 * 1024) + 1;
    printf("test_multipart_api file size:%d\n", file_size);

    gen_file(file_op_put_path, file_size);
    std::fstream f(file_op_put_path, std::ios_base::in | std::ios_base::binary);
    InitMultiUploadRequest init_request(bucket, object);
    InitMultiUploadResponse init_response;
    int ret = client.init_multipart_upload(init_request, &init_response);
    printf("init_multipart_upload, upload id:%s, ret:%d\n", init_response.upload_id().c_str(), ret);
    if (ret != 0) {
        return false;
    }
    f.seekg(0, std::ios::end);
    long long size = f.tellg();
    f.seekg(0, std::ios::beg);

    const int part_size = 1024 * 1024 * 1;
    int part_count = (int)(size / part_size);
    std::vector<std::string> etags;
    etags.reserve(part_count + 1);
    std::string upload_part_data;
    upload_part_data.resize(part_size);
    for (int i = 0; i < part_count; i++) {
        f.read((char *)upload_part_data.data(), part_size);
        UploadPartRequest upload_part_request(bucket, object, upload_part_data,
            i + 1, init_response.upload_id());
        UploadPartResponse upload_part_response;
        int ret = client.upload_part(upload_part_request, &upload_part_response);
        etags.push_back(upload_part_response.etag());
        printf("upload_part part:%d, ret:%d\n", i + 1,  ret);
        if (ret != 0) {
            return false;
        }
   
    }
    int residual = size % part_size;
    if (residual != 0) {
        upload_part_data.resize(residual);
        f.read((char *)upload_part_data.data(), residual);
        UploadPartRequest upload_part_request(bucket, object, upload_part_data,
            part_count + 1, init_response.upload_id());
        UploadPartResponse upload_part_response;
        int ret = client.upload_part(upload_part_request, &upload_part_response);
        etags.push_back(upload_part_response.etag());
        printf("upload_part last part:%d, ret:%d\n", part_count + 1, ret);
        if (ret != 0) {
            return false;
        }
    }

    {   //list_parts
        ListPartsRequest list_request(bucket, object, init_response.upload_id());
        ListPartsResponse list_reponse;

        int ret = client.list_parts(list_request, &list_reponse);
        printf("list_parts ret:%d\n", ret);
        if (ret != 0) {
            return false;
        }
        if (list_reponse.parts().size() != etags.size()) {
            printf("list_parts part size not expected, want:%d, got:%d", (int)etags.size(), (int)list_reponse.parts().size());
            return false;
        }
        auto & parts = list_reponse.parts();
        for (size_t i = 0; i < etags.size(); i++) {
            if (parts[i].etag != etags[i]) {
                printf("etag not match,  want:[%s] , got:[%s]", etags[i].c_str(), parts[i].etag.c_str());
                return false;
            }
        }
    }

    {   //list_multipart_uploads
        ListMultipartUploadsRequest list_upload_request(bucket);
        ListMultipartUploadsResponse list_upload_reponse;
        int ret = client.list_multipart_uploads(list_upload_request, &list_upload_reponse);
        printf("list_multipart_uploads ret:%d\n", ret);
        if (ret != 0) {
            return false;
        }
        bool match_currnet_upload = false;
        for (auto & upload_sumary : list_upload_reponse.uploads()) {
            if (upload_sumary.upload_id == init_response.upload_id()) {
                match_currnet_upload = true;
            }
        }
        if (false == match_currnet_upload) {
            printf("list_multipart_uploads error, upload id:[%s] not exists\n", 
                init_response.upload_id().c_str());
        }
    }

    {   //abort     
        AbortMultipartUploadRequest abort_request(bucket, object, init_response.upload_id());
        AbortMultipartUploadResponse abort_response;
        int ret = client.abort_multipart_upload(abort_request, &abort_response);
        printf("abort_multipart_upload ret:%d\n", ret);
        if (ret != 0) {
            return false;
        }
    }
    printf("test_multipart_api : ok\n");
    printf("----------------------------\n");
    return true;
}

bool test_large_file(baidu::bos::cppsdk::Client &client, const std::string & temp_dir, const std::string &bucket) {
    const long long file_size = 5LL * 1024 * 1024 * 1024 + rand() % 1024; //5G + 
    parallel_upload1 uploader;
    parallel_download1 downloader;
    parallel_copy1 copyer;
    bool final_result = test_object_api_internal(client, temp_dir, bucket, file_size,
        &uploader, &downloader, &copyer);
    if (false == final_result) {
        return false;
    }
    printf("test_large_file : ok\n");
    printf("----------------------------\n");
    return true;
}
