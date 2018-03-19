#include "CppUTest/TestHarness.h"

#include "pandora_curl.h"

#include <curl/curl.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

TEST_GROUP(PANDORACURL){
    void setup(){
        curl_global_init(CURL_GLOBAL_ALL);
    }

    void teardown(){
        curl_global_cleanup();
    }};

TEST(PANDORACURL, connect) {
    PANDORA_Points* points = pandora_points_new();
    pandora_points_add_long(points, "ll", 999988887L);

    pandora_points_add_string(points, "str", "hello");

    pandora_points_add_float(points, "dbl", 999.99);

    pandora_points_add_boolean(points, "bool", 0);

    pandora_points_add_time(points, "tm", time(NULL));

    pandora_points_newline(points);

    pandora_points_add_long(points, "ll", 888888L);

    pandora_points_add_string(points, "str", "llo");

    pandora_points_add_float(points, "dbl", 666.66);

    pandora_points_add_boolean(points, "bool", 1);

    pandora_points_add_time(points, "tm", time(NULL));

    pandora_points_newline(points);

    PANDORA_curl_send("csdk", 
        "Pandora zIV2FhwbOS0wKj3K1TmMRWOgyDSx6uTmMwu3mIRL:-wRHpm51fM4hVOcetlDBtTMti0o=:eyJyZXNvdXJjZSI6Ii92Mi9yZXBvcy9jc2RrL2RhdGEiLCJleHBpcmVzIjoxNTI3NDY1MDYwLCJjb250ZW50TUQ1IjoiIiwiY29udGVudFR5cGUiOiJ0ZXh0L3BsYWluIiwiaGVhZGVycyI6IiIsIm1ldGhvZCI6IlBPU1QifQ==", 
        points);

    pandora_points_delete(points);
}
