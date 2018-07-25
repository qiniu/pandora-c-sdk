set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

find_package(CURL REQUIRED)
if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIR})
    link_libraries(${CURL_LIBRARIES})
else()
    message(FATAL_ERROR "Could not found CURL library")
endif()

find_package(APR REQUIRED)
if(APR_FOUND)
    include_directories(${APR_INCLUDE_DIR})
    link_libraries(${APR_LIBRARY})
    include_directories(${APRUTIL_INCLUDE_DIR})
    link_libraries(${APRUTIL_LIBRARY})
else()
    message(FATAL_ERROR "Could not found APR library")
endif()

set(LIBRARY_NAME pandora)
set(LIBRARY_STATIC_NAME pandora_static)
set(LIBRARY_SHARED_NAME pandora_shared)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

aux_source_directory(. LIB_CFILES)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(${LIBRARY_STATIC_NAME} STATIC ${LIB_CFILES} ${CRYPTO_FILES})
set_target_properties(${LIBRARY_STATIC_NAME} PROPERTIES OUTPUT_NAME ${LIBRARY_NAME}
        CLEAN_DIRECT_OUTPUT 1)
add_library(${LIBRARY_SHARED_NAME} SHARED ${LIB_CFILES} ${CRYPTO_FILES})
set_target_properties(${LIBRARY_SHARED_NAME} PROPERTIES OUTPUT_NAME ${LIBRARY_NAME}
        CLEAN_DIRECT_OUTPUT 1)

install(TARGETS ${LIBRARY_STATIC_NAME} ${LIBRARY_SHARED_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION .
        FILES_MATCHING PATTERN "*.h")