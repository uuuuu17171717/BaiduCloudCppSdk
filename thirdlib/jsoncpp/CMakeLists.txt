project(JSON_CPP)

find_package(Qt5Core REQUIRED)

add_definitions(-DJSON_CPP_LIBRARY)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QT_INCLUDES ${Qt5Core_INCLUDE_DIRS})

include_directories(
        ./include/
        ${QT_INCLUDES}
)

set(_SOURCES
        "src/lib_json/json_reader.cpp"
        "src/lib_json/json_value.cpp"
        "src/lib_json/json_writer.cpp"
        )

set(_HEADERS
        "include/json/allocator.h"
        "include/json/assertions.h"
        "include/json/config.h"
        "include/json/forwards.h"
        "include/json/json.h"
        "include/json/json_features.h"
        "include/json/reader.h"
        "include/json/value.h"
        "include/json/version.h"
        "include/json/writer.h"
        "src/lib_json/json_tool.h"
        "src/lib_json/json_valueiterator.inl"
        )

# add lib
add_library(${PROJECT_NAME} ${_SOURCES} ${_HEADERS})

# link lib
target_link_libraries(${PROJECT_NAME} Qt5::Core)