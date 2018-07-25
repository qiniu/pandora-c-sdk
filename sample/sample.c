#include "pandora/pandora.h"

int main(void)
{
    // 1、设置client参数
    s_client_params params;
    params.host = "https://nb-pipeline.qiniuapi.com"; // Pandora服务地址
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

    // 3、（可选）设置缓存策略，默认为NO_CACHE，即不开启缓存功能。
    // 此处设置的缓存策略为按照缓存文件大小(1MB)，当缓存文件大小超过1MB时，将触发缓存文件的上传，上次成功后清理缓存文件。
    // 其中，最后一个参数表示缓存文件存放的文件夹路径。
    //
    // 按照时间的缓存策略示例（超过15分钟（900秒）将触发缓存文件的上传）：
    // pandora_client_set_cache_policy(client, CACHE_BY_TIME, 900, "./cache")
    //
    // 提示：如果不开启缓存功能，不需要调用pandora_client_set_cache_policy函数。
    pandora_error_t status = pandora_client_set_cache_policy(client, CACHE_BY_SIZE, 1*1024*1024, "./cache");
    if (status != PANDORAE_OK) {
        fprintf(stderr, "failed to set cache policy: %d\n", status);
        pandora_client_cleanup(client);
        return 1;
    }

    // 4、（可选）刷新上一次程序运行期间产生且未上传的所有缓存文件，其中，myrepo1为Pandora仓库的名称，./cache为缓存文件夹。
    //
    // 提示：如果不开启缓存功能，不需要调用pandora_client_write_cached函数。
    const char *repo = "myrepo1";
    pandora_client_write_cached(client, repo, "./cache");
    if (status != PANDORAE_OK) {
        fprintf(stderr, "could not write cached data points: %d\n", status);
        pandora_client_cleanup(client);
        return 1;
    }

    // 5、创建数据点集合，一共发送10w个次打点请求，每个请求包含两个数据点，发送过程中会生成缓存文件。
    s_data_points *data = data_points_create();
    for (int i = 0; i < 100000; ++i) {
        // 5.1 增加第一个数据点
        s_point_entry *pentry1 = point_entry_create(); // 创建数据点实例
        point_entry_append_string(pentry1, "f1", "abc"); // 添加字段f1（类型为string）
        point_entry_append_int32(pentry1, "f2", 123); // 添加字段f2（类型为long）
        data_points_append(data, pentry1); // 将该数据点实例添加到步骤5创建的数据点集合
        point_entry_destroy(pentry1); // 注意：释放该数据点实例!!!

        // 5.2 增加第二个数据点
        s_point_entry *pentry2 = point_entry_create(); // 创建数据点实例
        point_entry_append_string(pentry2, "f1", "xyz"); // 添加字段f1（类型为string）
        point_entry_append_int32(pentry2, "f2", 456); // 添加字段f2（类型为long）
        data_points_append(data, pentry2); // 将该数据点实例添加到步骤5创建的数据点集合
        point_entry_destroy(pentry2); // 注意：释放该数据点实例!!!

        // 5.3、发送数据点集合（可能被写入缓存文件）
        status = pandora_client_write(client, repo, data);
        if (status != PANDORAE_OK) {
            fprintf(stderr, "write failed with status: %d\n", status);
        }

        // 5.4、清空数据点集合，重新用于新的打点请求
        data_points_clear(data);
    }

    // 6、释放数据点集合
    data_points_destroy(data);

    // 7. 清理client实例
    pandora_client_cleanup(client);

    return 0;
}

