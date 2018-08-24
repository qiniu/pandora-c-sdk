# pandora-c-sdk

## 安装依赖库

### libcur
- wget https://curl.haxx.se/download/curl-7.61.0.tar.gz
- tar zxvf curl-7.61.0.tar.gz
- cd curl-7.61.0
- ./configure --prefix=/usr/local
- make && make install

## 安装pandora-c-sdk
- git clone https://github.com/qiniu/pandora-c-sdk.git
- cd pandora-c-sdk
- cmake .
- make && make install

## 使用pandora-c-sdk（可参考示例代码sample/sample.c）
- 引用头文件<pandora/client.h>
- 编译时加上-lpandora选项

## 示例代码
```
    // 1、设置client参数
    s_client_params params;
    params.pipeline_host = "https://nb-pipeline.qiniuapi.com"; // pipeline服务地址
    params.insight_host = "https://nb-insight.qiniuapi.com"; // insight服务地址
    params.access_key = "<--input your ak-->"; // 七牛AK
    params.secret_key = "<--input your sk-->"; // 七牛SK
    params.fail_retry = 3; // 失败重试次数

    // 2. 使用client参数初始化client
    s_pandora_client *client = pandora_client_init(&params);
    if (!client) {
        fprintf(stderr, "client initialization failed\n");
        pandora_client_cleanup(client);
        return 1;
    }

    //3、创建数据点集合
    s_data_points *data = data_points_create();

    // 3.1 添加第一个数据点
    s_point_entry *pentry1 = point_entry_create(); // 创建数据点实例
    point_entry_append_string(pentry1, "f1", "abc"); // 添加字段f1（类型为string）
    point_entry_append_int32(pentry1, "f2", 123); // 添加字段f2（类型为long）
    data_points_append(data, pentry1); // 将该数据点实例添加到步骤3创建的数据点集合
    point_entry_destroy(pentry1); // 注意：释放该数据点实例!

    // 3.2 添加第二个数据点
    s_point_entry *pentry2 = point_entry_create(); // 创建数据点实例
    point_entry_append_string(pentry2, "f1", "xyz"); // 添加字段f1（类型为string）
    point_entry_append_int32(pentry2, "f2", 456); // 添加字段f2（类型为long）
    data_points_append(data, pentry2); // 将该数据点实例添加到步骤3创建的数据点集合
    point_entry_destroy(pentry2); // 注意：释放该数据点实例!

    // 4、发送数据点集合
    const char *pipeline_repo = "repo1";
    pandora_error_t status = pandora_client_write(client, pipeline_repo, data);
    if (status != PANDORAE_OK) {
        fprintf(stderr, "write failed with status: %d\n", status);
    }

    // 5、释放数据点集合
    data_points_destroy(data);

    // 6、日志查询
    s_search_params srchp;
    srchp.size = 10; // 日志返回数量
    srchp.from = 0; // 日志起始位置
    srchp.query = "f1:abc"; // 查询表达式
    srchp.sort = "f2:desc"; // 结果排序
    srchp.fields = "f1,f2"; // 字段选择

    const char *insight_repo = "repo1"; // 日志仓库名称
    char *result = NULL; // 查询结果
    pandora_client_insight_search(client, insight_repo, &srchp, &result);
    printf("result: %s\n", result);
    free(result);

    // 7. 清理client实例
    pandora_client_cleanup(client);
```

## 示例代码运行
- cd sample
- 填写sample.c文件中的ak、sk以及repo名称
- make
- ./sample

## 注意事项
- client的创建、释放
```
s_pandora_client *client = pandora_client_init(&params)

...

pandora_client_cleanup(client);
```

- 数据点集合的创建、释放

```
s_data_points *data = data_points_create();

... 添加若干数据点并发送

data_points_destroy(data);
```

- 数据点的创建、释放
```
// 1、创建一个数据点
s_point_entry *pentry1 = point_entry_create();

// 2、添加若干字段
point_entry_append_string(pentry1, "f1", "abc");
point_entry_append_int32(pentry1, "f2", 123);

// 3、将该数据点加入到数据集
data_points_append(data, pentry1);

// 4、加入后释放该数据点
data_points_destroy(data);
```
