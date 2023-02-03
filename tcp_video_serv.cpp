// tcp_video_serv.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "winsock2.h"
#include<cstdlib>
#include "opencv2\opencv.hpp"
#include "opencv2\imgproc\imgproc.hpp"
#pragma comment(lib,"ws2_32.lib")//引用库文件
#pragma comment(lib,"opencv_world340.lib")//引用库文件


char recvBuf[1000];
char sendBuf[100];
SOCKET sockConn;
/**
* 在一个新的线程里面接收数据
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
		cv::Mat image = cv::imdecode(decode, CV_LOAD_IMAGE_COLOR);//图像解码
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
		//cv::Mat image = cv::imdecode(decode, CV_LOAD_IMAGE_COLOR);//图像解码
		//cv::imshow("image", image);
		//cv::waitKey(1);
		//decode.clear();
	}
	closesocket(sockConn);
}

int main()
{
	WSADATA wsaData;
	int port = 8889;//端口号
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("初始化失败");
		return 0;
	}

	//创建用于监听的套接字,即服务端的套接字
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port); //1024以上的端口号
	/**
	* INADDR_ANY就是指定地址为0.0.0.0的地址，这个地址事实上表示不确定地址，或“所有地址”、“任意地址”。 一般来说，在各个系统中均定义成为0值。
	*/
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	int retVal = bind(sockSrv, (LPSOCKADDR)&addrSrv, sizeof(SOCKADDR_IN));
	if (retVal == SOCKET_ERROR) {
		printf("连接失败:%d\n", WSAGetLastError());
		return 0;
	}

	if (listen(sockSrv, 10) == SOCKET_ERROR) {
		printf("监听失败:%d", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (1)
	{
		//等待客户请求到来
		sockConn = accept(sockSrv, (SOCKADDR *)&addrClient, &len);
		if (sockConn == SOCKET_ERROR) {
			printf("等待请求失败:%d", WSAGetLastError());
			break;
		}

		printf("客户端的IP是:[%s]\n", inet_ntoa(addrClient.sin_addr));

		//发送数据
		char sendbuf[] = "你好，我是服务端，咱们一起聊天吧";
		int iSend = send(sockConn, sendbuf, sizeof(sendbuf), 0);
		if (iSend == SOCKET_ERROR) {
			printf("发送失败");
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
