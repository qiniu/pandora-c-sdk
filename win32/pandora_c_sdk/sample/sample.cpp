#include <iostream>

#include "pandora\client.h"

int main()
{
	// 1������client����
	s_client_params params;
	params.pipeline_host = "https://nb-pipeline.qiniuapi.com"; // pipeline�����ַ
	params.insight_host = "https://nb-insight.qiniuapi.com"; // insight�����ַ
	params.access_key = "<input your qiniu ak>"; // ��ţAK
	params.secret_key = "<input your qiniu sk>"; // ��ţSK
	params.fail_retry = 3; // ʧ�����Դ���

	// 2. ʹ��client������ʼ��client
	s_pandora_client *client = pandora_client_init(&params);
	if (!client) {
		fprintf(stderr, "client initialization failed\n");
		pandora_client_cleanup(client);
		return 1;
	}

	//3���������ݵ㼯��
	s_data_points *data = data_points_create();

	// 3.1 ��ӵ�һ�����ݵ�
	s_point_entry *pentry1 = point_entry_create(); // �������ݵ�ʵ��
	point_entry_append_string(pentry1, "f1", "abc"); // ����ֶ�f1������Ϊstring��
	point_entry_append_int32(pentry1, "f2", 123); // ����ֶ�f2������Ϊlong��
	data_points_append(data, pentry1); // �������ݵ�ʵ����ӵ�����3���������ݵ㼯��
	point_entry_destroy(pentry1); // ע�⣺�ͷŸ����ݵ�ʵ��!

	// 3.2 ��ӵڶ������ݵ�
	s_point_entry *pentry2 = point_entry_create(); // �������ݵ�ʵ��
	point_entry_append_string(pentry2, "f1", "xyz"); // ����ֶ�f1������Ϊstring��
	point_entry_append_int32(pentry2, "f2", 456); // ����ֶ�f2������Ϊlong��
	data_points_append(data, pentry2); // �������ݵ�ʵ����ӵ�����3���������ݵ㼯��
	point_entry_destroy(pentry2); // ע�⣺�ͷŸ����ݵ�ʵ��!

	// 4���������ݵ㼯��
	const char *pipeline_repo = "repo1";
	pandora_error_t status = pandora_client_write(client, pipeline_repo, data);
	if (status != PANDORAE_OK) {
		fprintf(stderr, "write failed with status: %d\n", status);
	}

	// 5���ͷ����ݵ㼯��
	data_points_destroy(data);

	// 6����־��ѯ
	s_search_params srchp;
	srchp.size = 10; // ��־��������
	srchp.from = 0; // ��־��ʼλ��
	srchp.query = "f1:abc"; // ��ѯ���ʽ
	srchp.sort = "f2:desc"; // �������
	srchp.fields = "f1,f2"; // �ֶ�ѡ��

	const char *insight_repo = "repo1"; // ��־�ֿ�����
	char *result = NULL; // ��ѯ���
	pandora_client_insight_search(client, insight_repo, &srchp, &result);
	printf("result: %s\n", result);
	free(result);

	// 7. ����clientʵ��
	pandora_client_cleanup(client);

	system("pause");

	return 0;
}