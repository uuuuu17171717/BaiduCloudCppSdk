cmake_minimum_required (VERSION 3.1)

project (bossdk)

set( MAIN_SRCS
	src/auth/auth.cpp
	src/bos/client.cpp
	src/bos/client_impl.cpp
	src/bos/model/copy_request.cpp
	src/bos/model/object_meta_data.cpp
	src/bos/model/upload_request.cpp
	src/bos/request/complete_multipart_upload_request.cpp
	src/bos/request/object_request.cpp
	src/bos/response/append_object_response.cpp
	src/bos/response/complete_multipart_upload_response.cpp
	src/bos/response/copy_object_response.cpp
	src/bos/response/copy_part_response.cpp
	src/bos/response/get_bucket_acl_response.cpp
	src/bos/response/get_bucket_location_response.cpp
	src/bos/response/get_object_response.cpp
	src/bos/response/init_multi_upload_response.cpp
	src/bos/response/list_buckets_response.cpp
	src/bos/response/list_multipart_uploads_response.cpp
	src/bos/response/list_objects_response.cpp
	src/bos/response/list_parts_response.cpp
    src/bos/response/get_bucket_acl_response.cpp
    src/bos/response/get_object_acl_response.cpp
	src/http/http_client.cpp
	src/http/http_request.cpp
	src/http/http_response.cpp
	src/model/bce_request.cpp
	src/model/bce_response.cpp
	src/model/json_response.cpp
	#src/util/openssl_crypto_impl.cpp
	src/util/util.cpp
	src/util/windows_port.cpp
	src/util/win_crypto_impl.cpp
	)

include_directories(
	include
	thirdlib/curl/include
	thirdlib/win_uuid/include
)

add_definitions(
	-DVERSION="1.0"
)


set(ARCHITECTURE, "")
if("${CMAKE_GENERATOR}" MATCHES "(Win64|AMD64)")
    set(ARCHITECTURE "x64")
    list( APPEND LIBS thirdlib/curl/bin/AMD64/LIBCURL.LIB )
elseif("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
    set(ARCHITECTURE "IA64")
	list( APPEND LIBS thirdlib/curl/bin/IA64/LIBCURL.LIB )
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCHITECTURE "x86")
	list( APPEND LIBS thirdlib/curl/bin/I386/LIBCURL.LIB )
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/${ARCHITECTURE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/${ARCHITECTURE}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/${ARCHITECTURE}")

file(COPY ${CMAKE_SOURCE_DIR}/include/
	DESTINATION "${CMAKE_SOURCE_DIR}/output/include")

macro(SET_OPTION option value)
  set(${option} ${value} CACHE INTERNAL "" FORCE)
endmacro()

SET_OPTION(JSONCPP_WITH_TESTS OFF)
SET_OPTION(JSONCPP_WITH_POST_BUILD_UNITTEST OFF)
add_subdirectory(thirdlib/json)

add_library(bossdk ${MAIN_SRCS} ${LIBS})
target_link_libraries (bossdk jsoncpp_lib_static)


if("${CMAKE_GENERATOR}" MATCHES "(Win64|AMD64)")
	add_custom_command(TARGET bossdk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/thirdlib/curl/bin/AMD64/LIBCURL.DLL"
            "${CMAKE_SOURCE_DIR}/output/lib/x64/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/thirdlib/curl/bin/AMD64/LIBCURL.LIB"
            "${CMAKE_SOURCE_DIR}/output/lib/x64/$<CONFIG>"
    )
elseif("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
	add_custom_command(TARGET bossdk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/thirdlib/curl/bin/IA64/LIBCURL.DLL"
            "${CMAKE_SOURCE_DIR}/output/lib/IA64/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/thirdlib/curl/bin/IA64/LIBCURL.LIB"
            "${CMAKE_SOURCE_DIR}/output/lib/IA64/$<CONFIG>"
    )
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	add_custom_command(TARGET bossdk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/thirdlib/curl/bin/I386/LIBCURL.DLL"
            "${CMAKE_SOURCE_DIR}/output/lib/x86/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/thirdlib/curl/bin/I386/LIBCURL.LIB"
            "${CMAKE_SOURCE_DIR}/output/lib/x86/$<CONFIG>"
    )
endif()
