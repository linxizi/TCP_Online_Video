// tcp_video_serv.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "winsock2.h"
#include<cstdlib>
#include "opencv2\opencv.hpp"
#include "opencv2\imgproc\imgproc.hpp"
#pragma comment(lib,"ws2_32.lib")//���ÿ��ļ�
#pragma comment(lib,"opencv_world340.lib")//���ÿ��ļ�


char recvBuf[1000];
char sendBuf[100];
SOCKET sockConn;
/**
* ��һ���µ��߳������������
*/

DWORD WINAPI Fun(LPVOID lpParamter)
{
	std::vector<unsigned char>decode;
	//cv::namedWindow("image", cv::WINDOW_NORMAL);
	while (true) {
		memset(recvBuf, 0, sizeof(recvBuf));
		int pos = 0;
		int total_len = 0;
		while (true)
		{
			int len = recv(sockConn, recvBuf, sizeof(recvBuf), 0);
			//std::cout << " len " << len << std::endl;
			total_len += len;
			while (pos < total_len) { decode.push_back(recvBuf[pos++]); }
			if (len < 1000) { /*std::cout << " total_len " << total_len << std::endl;*/  break; }
		}
		//std::cout << "decode.size() " << decode.size() << std::endl;
		if (decode.size() ==  100) { decode.clear();  continue; }
		cv::Mat image = cv::imdecode(decode, CV_LOAD_IMAGE_COLOR);//ͼ�����
		//std::cout << "decode.size() " << decode.size() << "   "<< image.size << "  "<< image.channels() <<std::endl;
		cv::Vec3b intensity = image.at<cv::Vec3b>(100, 100);
		uchar blue = intensity.val[0];
		uchar green = intensity.val[1];
		uchar red = intensity.val[2];
		printf("B: %d G: %d R: %d \n", blue, green, red);
		/*cv::imshow("image", image);
		cv::waitKey(1);*/
		decode.clear();
		//std::cout << " len " << len << std::endl;
		//if (len < 1000) { continue; }
		//int pos = 0; 
		//while (pos < len) { decode.push_back(recvBuf[pos++]); }
		//cv::Mat image = cv::imdecode(decode, CV_LOAD_IMAGE_COLOR);//ͼ�����
		//cv::imshow("image", image);
		//cv::waitKey(1);
		//decode.clear();
	}
	closesocket(sockConn);
}

int main()
{
	WSADATA wsaData;
	int port = 8889;//�˿ں�
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("��ʼ��ʧ��");
		return 0;
	}

	//�������ڼ������׽���,������˵��׽���
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port); //1024���ϵĶ˿ں�
	/**
	* INADDR_ANY����ָ����ַΪ0.0.0.0�ĵ�ַ�������ַ��ʵ�ϱ�ʾ��ȷ����ַ�������е�ַ�����������ַ���� һ����˵���ڸ���ϵͳ�о������Ϊ0ֵ��
	*/
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	int retVal = bind(sockSrv, (LPSOCKADDR)&addrSrv, sizeof(SOCKADDR_IN));
	if (retVal == SOCKET_ERROR) {
		printf("����ʧ��:%d\n", WSAGetLastError());
		return 0;
	}

	if (listen(sockSrv, 10) == SOCKET_ERROR) {
		printf("����ʧ��:%d", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (1)
	{
		//�ȴ��ͻ�������
		sockConn = accept(sockSrv, (SOCKADDR *)&addrClient, &len);
		if (sockConn == SOCKET_ERROR) {
			printf("�ȴ�����ʧ��:%d", WSAGetLastError());
			break;
		}

		printf("�ͻ��˵�IP��:[%s]\n", inet_ntoa(addrClient.sin_addr));

		//��������
		char sendbuf[] = "��ã����Ƿ���ˣ�����һ�������";
		int iSend = send(sockConn, sendbuf, sizeof(sendbuf), 0);
		if (iSend == SOCKET_ERROR) {
			printf("����ʧ��");
			break;
		}

		HANDLE hThread = CreateThread(NULL, 0, Fun, NULL, 0, NULL);
		CloseHandle(hThread);

	}

	closesocket(sockSrv);
	WSACleanup();
	system("pause");
	return 0;
}
