// tcp_video_client.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "stdafx.h"
#include "opencv2\opencv.hpp"
#include "opencv2\imgproc\imgproc.hpp"
#include<WinSock2.h>
#include<iostream>
#include<mutex>
#include<thread>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"opencv_world340.lib")
std::mutex myMutex;
std::queue<cv::Mat> queueInput;//�洢ͼ��Ķ���

void get_online_video()
{
	//��������������������ַ
	std::string  url = "rtsp://admin:abc.1234@192.168.0.64:554/h264/ch1/sub/av_stream";
	cv::VideoCapture cap(url);
	cv::Mat frame;//�����֡��ͼ�����
	while (1)
	{
		cap >> frame;
		myMutex.lock();
		if (queueInput.size() > 3) {
			queueInput.pop();
		}
		else {
			queueInput.push(frame);
		}
		myMutex.unlock();
	}
}

int send_online_video()
{
	WORD w_req = MAKEWORD(2, 2);//�汾��
	WSADATA wsadata;
	int err;
	err = WSAStartup(w_req, &wsadata);
	if (err != 0) {
		std::cout << "��ʼ���׽��ֿ�ʧ�ܣ�" << std::endl;
		return false;
	}
	else {
		std::cout << "��ʼ���׽��ֿ�ɹ���" << std::endl;
	}

	//���汾��
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		std::cout << "�׽��ֿ�汾�Ų�����" << std::endl;
		WSACleanup();
		return false;
	}
	else {
		std::cout << "�׽��ֿ�汾��ȷ��" << std::endl;
	}

	SOCKADDR_IN server_addr;
	SOCKADDR_IN accept_addr;

	//���������Ϣ
	server_addr.sin_family = AF_INET;  // �����������ֵ�ַ�壬AF_INET��IPV4
	std::string m_ip = "192.168.0.111";
	server_addr.sin_addr.S_un.S_addr = inet_addr(m_ip.c_str());  // ����ip��ַ��htonl��һ���޷��ų�����ת��ΪTCP/IPЭ������Ĵ��
	// INADDR_ANY��ʾһ���������ϵ���������
	server_addr.sin_port = htons(7777);  // �˿ں�

	//�����׽���
	SOCKET m_server = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(m_server, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		std::cout << "����������ʧ�ܣ�" << std::endl;
		WSACleanup();
		return false;
	}
	else {
		std::cout << "���������ӳɹ���" << std::endl;
	}

	cv::Mat frame;
	std::vector<uchar> data_encode;//��������紫�����ݽ���������
	std::vector<int> params;  // ѹ������
	params.resize(3, 0);
	params[0] = cv::IMWRITE_JPEG_QUALITY; // ����ѹ��
	params[1] = 30;//ѹ������������ ��ֵԽ�� ѹ�����ͼ������Խ��
	char frames_cnt[10] = { 0, };
	std::cout << "��ʼ����" << std::endl;
	int j = 0;
	while (1) {
		/* ������ö��߳� �Ӷ����д�ȡ���� ��Ҫ�Ƿ�ֹ���߳̽���������Ƶ�ٶ�̫�����µ�����ӵ��*/
		myMutex.lock();
		if (queueInput.empty()) {//���������û������ ˵������Ϊ�� ��ʱӦ�õȴ����������������������
			myMutex.unlock();//�ͷ���
			Sleep(3);//˯��������  �����ø�������
			continue;
		}
		else {
			frame = queueInput.front();//�Ӷ�����ȡ��ͼ�����
			queueInput.pop();
			myMutex.unlock();//�ͷ���
		}
		imencode(".jpg", frame, data_encode, params);  // ��ͼ�����ѹ��
		int len_encoder = data_encode.size();//��ȡͼ��������ֽڳ��� �������ͨ��TCP����ʱ  ���ն�֪���˴δ�����ֽڴ�С
		_itoa_s(len_encoder, frames_cnt, 10);// 
		send(m_server, frames_cnt, 10, 0);//��ͼ���ֽڳ��� ���д���
		// ����
		int index = 0;//��־ʵʱ����ͼ���ֽڵĳ��� ����������жϻ��ж����ֽ���δ���յ�
		char *send_b = new char[data_encode.size()];// ����һ���ֽ����� ������СΪͼ���ֽڳ��ȵ��ַ�����ռ�
		//�����ǽ�data_encode�׵�ַ�ҳ���ΪͼƬ�ֽڳ��� ͨ���ڴ濽�����Ƶ�send_b�����У�����ڲ���ѭ������Ԫ�ظ�ֵ���ٶȿ�������10��
		memcpy(send_b, &data_encode[0], data_encode.size());
		int iSend = send(m_server, send_b, data_encode.size(), 0);//��ͼ���ֽ����ݴ��䵽��������
		delete[]send_b;//���ٶ���
		data_encode.clear();//���������  ������һ�ν���ͼ��������
		++j;
	}
	std::cout << "�������";
	closesocket(m_server);//�رշ��Ͷ��׽���
	WSACleanup();//�ͷų�ʼ��Ws2_32.dll���������Դ��
}

int main()
{
	std::thread Get(get_online_video);
	std::thread Send(send_online_video);
	Get.join();
	Send.join();
	return 0;
}