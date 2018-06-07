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

TEST(PANDORACURL, pandora_point) {
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

    PANDORA_CURL_CODE c = PANDORA_curl_send("csdk", 
        "Pandora 0knikszBTnSnfqhcWLw1yvunjhqweRYSV2GlLbrP:C2F02gMNJ3f_xfOKCnidfTIAngk=:eyJyZXNvdXJjZSI6Ii92Mi9yZXBvcy9jc2RrL2RhdGEiLCJleHBpcmVzIjo0NjgxOTQxNDk0LCJjb250ZW50TUQ1IjoiIiwiY29udGVudFR5cGUiOiJ0ZXh0L3BsYWluIiwiaGVhZGVycyI6IiIsIm1ldGhvZCI6IlBPU1QifQ==", 
        points);

    pandora_points_delete(points);

    LONGS_EQUAL(PANDORA_CURL_OK, c);
}

TEST(PANDORACURL, pandora_file) {
    PANDORA_CURL_CODE c = PANDORA_curl_send_file("syslog", 
        "Pandora 0knikszBTnSnfqhcWLw1yvunjhqweRYSV2GlLbrP:_WQP2SeErz10br9Q5tCzyEN1qHQ=:eyJyZXNvdXJjZSI6Ii92Mi9yZXBvcy9zeXNsb2cvZGF0YSIsImV4cGlyZXMiOjQ2ODE5NDE1NDYsImNvbnRlbnRNRDUiOiIiLCJjb250ZW50VHlwZSI6InRleHQvcGxhaW4iLCJoZWFkZXJzIjoiIiwibWV0aG9kIjoiUE9TVCJ9", 
        "./x.log");

    LONGS_EQUAL(PANDORA_CURL_OK, c);
}
