// tcp_video_client.cpp : 定义控制台应用程序的入口点。
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
std::queue<cv::Mat> queueInput;//存储图像的队列

void get_online_video()
{
	//海康威视子码流拉流地址
	std::string  url = "rtsp://admin:abc.1234@192.168.0.64:554/h264/ch1/sub/av_stream";
	cv::VideoCapture cap(url);
	cv::Mat frame;//保存抽帧的图像矩阵
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
	WORD w_req = MAKEWORD(2, 2);//版本号
	WSADATA wsadata;
	int err;
	err = WSAStartup(w_req, &wsadata);
	if (err != 0) {
		std::cout << "初始化套接字库失败！" << std::endl;
		return false;
	}
	else {
		std::cout << "初始化套接字库成功！" << std::endl;
	}

	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		std::cout << "套接字库版本号不符！" << std::endl;
		WSACleanup();
		return false;
	}
	else {
		std::cout << "套接字库版本正确！" << std::endl;
	}

	SOCKADDR_IN server_addr;
	SOCKADDR_IN accept_addr;

	//填充服务端信息
	server_addr.sin_family = AF_INET;  // 用来定义那种地址族，AF_INET：IPV4
	std::string m_ip = "192.168.0.111";
	server_addr.sin_addr.S_un.S_addr = inet_addr(m_ip.c_str());  // 保存ip地址，htonl将一个无符号长整型转换为TCP/IP协议网络的大端
	// INADDR_ANY表示一个服务器上的所有网卡
	server_addr.sin_port = htons(7777);  // 端口号

	//创建套接字
	SOCKET m_server = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(m_server, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		std::cout << "服务器连接失败！" << std::endl;
		WSACleanup();
		return false;
	}
	else {
		std::cout << "服务器连接成功！" << std::endl;
	}

	cv::Mat frame;
	std::vector<uchar> data_encode;//保存从网络传输数据解码后的数据
	std::vector<int> params;  // 压缩参数
	params.resize(3, 0);
	params[0] = cv::IMWRITE_JPEG_QUALITY; // 无损压缩
	params[1] = 30;//压缩的质量参数 该值越大 压缩后的图像质量越好
	char frames_cnt[10] = { 0, };
	std::cout << "开始发送" << std::endl;
	int j = 0;
	while (1) {
		/* 这里采用多线程 从队列中存取数据 主要是防止单线程解码网络视频速度太慢导致的网络拥塞*/
		myMutex.lock();
		if (queueInput.empty()) {//如果队列中没有数据 说明队列为空 此时应该等待生产者向队列中输入数据
			myMutex.unlock();//释放锁
			Sleep(3);//睡眠三秒钟  把锁让给生产者
			continue;
		}
		else {
			frame = queueInput.front();//从队列中取出图像矩阵
			queueInput.pop();
			myMutex.unlock();//释放锁
		}
		imencode(".jpg", frame, data_encode, params);  // 对图像进行压缩
		int len_encoder = data_encode.size();//获取图像编码后的字节长度 方便后续通过TCP传输时  接收端知道此次传输的字节大小
		_itoa_s(len_encoder, frames_cnt, 10);// 
		send(m_server, frames_cnt, 10, 0);//将图像字节长度 进行传输
		// 发送
		int index = 0;//标志实时接收图像字节的长度 方便程序中判断还有多少字节尚未接收到
		char *send_b = new char[data_encode.size()];// 创建一个字节数组 开启大小为图像字节长度的字符数组空间
		//这里是将data_encode首地址且长度为图片字节长度 通过内存拷贝复制到send_b数组中，相比于采用循环单个元素赋值，速度快了至少10倍
		memcpy(send_b, &data_encode[0], data_encode.size());
		int iSend = send(m_server, send_b, data_encode.size(), 0);//将图像字节数据传输到服务器端
		delete[]send_b;//销毁对象
		data_encode.clear();//将队列清空  方便下一次进行图像矩阵接收
		++j;
	}
	std::cout << "发送完成";
	closesocket(m_server);//关闭发送端套接字
	WSACleanup();//释放初始化Ws2_32.dll所分配的资源。
}

int main()
{
	std::thread Get(get_online_video);
	std::thread Send(send_online_video);
	Get.join();
	Send.join();
	return 0;
}