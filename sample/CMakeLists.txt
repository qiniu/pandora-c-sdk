include_directories(${PROJECT_SOURCE_DIR}/include)

if(APPLE)
    link_libraries(${PROJECT_BINARY_DIR}/lib/libpandora.dylib)
elseif(UNIX)
    link_libraries(${PROJECT_BINARY_DIR}/lib/libpandora.so)
endif()

add_executable(sample sample.c)
