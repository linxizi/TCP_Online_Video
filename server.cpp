#include "stdafx.h"
#include "server.h"
#define SIZE 100

bool Server::initialization(const int& port, const VideoCapture& cap) {
	m_port = port;
	m_cap = cap;

	// 初始化库（windows独有）
	//初始化套接字库
	WORD w_req = MAKEWORD(2, 2);//版本号
	WSADATA wsadata;
	int err;
	err = WSAStartup(w_req, &wsadata);
	if (err != 0) {
		cout << "初始化套接字库失败！" << endl;
		return false;
	}
	else {
		cout << "初始化套接字库成功！" << endl;
	}

	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		cout << "套接字库版本号不符！" << endl;
		WSACleanup();
		return false;
	}
	else {
		cout << "套接字库版本正确！" << endl;
	}

	return true;
}

bool Server::initialization(const int& port) {
	m_port = port;

	// 初始化库（windows独有）
	//初始化套接字库
	WORD w_req = MAKEWORD(2, 2);//版本号
	WSADATA wsadata;
	int err;
	err = WSAStartup(w_req, &wsadata);
	if (err != 0) {
		cout << "初始化套接字库失败！" << endl;
		return false;
	}
	else {
		cout << "初始化套接字库成功！" << endl;
	}

	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		cout << "套接字库版本号不符！" << endl;
		WSACleanup();
		return false;
	}
	else {
		cout << "套接字库版本正确！" << endl;
	}

	return true;
}

bool Server::build_connect() {
	//服务端地址客户端地址
	SOCKADDR_IN server_addr;
	SOCKADDR_IN accept_addr;

	//填充服务端信息
	server_addr.sin_family = AF_INET;  // 用来定义那种地址族，AF_INET：IPV4
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);  // 保存ip地址，htonl将一个无符号长整型转换为TCP/IP协议网络的大端
	// INADDR_ANY表示一个服务器上的所有网卡
	server_addr.sin_port = htons(m_port);  // 端口号

	//创建套接字
	m_server = socket(AF_INET, SOCK_STREAM, 0);  // 使用tcp进行连接

	if (::bind(m_server, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		cout << "套接字绑定失败！" << endl;
		WSACleanup();
		return false;
	}
	else {
		cout << "套接字绑定成功！" << endl;
	}

	//设置套接字为监听状态
	if (listen(m_server, SOMAXCONN) < 0) {
		cout << "设置监听状态失败！" << endl;
		WSACleanup();
		return false;
	}
	else {
		cout << "设置监听状态成功！" << endl;
	}
	cout << "服务端正在监听连接，请稍候...." << endl;
	//接受连接请求
	int len = sizeof(SOCKADDR);
	m_accept = accept(m_server, (SOCKADDR*)&accept_addr, &len);
	if (m_accept == SOCKET_ERROR) {
		cout << "连接失败！" << endl;
		WSACleanup();
		return false;
	}
	cout << "连接建立，准备接受数据" << endl;
	return true;
}

bool Server::send_data() {
	Mat frame;
	vector<uchar> data_encode;
	std::vector<int> params;  // 压缩参数
	params.resize(3, 0);
	params[0] = IMWRITE_JPEG_QUALITY; // 无损压缩
	params[1] = 30;
	char frames_cnt[10] = { 0, };
	_itoa_s(int(m_cap.get(CAP_PROP_FRAME_COUNT)), frames_cnt, 10);
	send(m_accept, frames_cnt, 10, 0);
	cout << "开始发送" << endl;
	int j = 0;
	while (m_cap.read(frame)) {

		m_file_in.push_back(frame.clone());
		imencode(".jpg", frame, data_encode, params);  // 对图像进行压缩
		int len_encoder = data_encode.size();

		_itoa_s(len_encoder, frames_cnt, 10);
		send(m_accept, frames_cnt, 10, 0);

		_itoa_s(SIZE, frames_cnt, 10);
		send(m_accept, frames_cnt, 10, 0);
		// 发送
		char send_char[SIZE] = { 0, };
		int index = 0;
		bool flag = false;
		for (int i = 0; i < len_encoder / SIZE + 1; ++i) {
			for (int k = 0; k < SIZE; ++k) {
				if (index >= data_encode.size()) {
					flag = true;
					break;
				}
				send_char[k] = data_encode[index++];
			}
			send(m_accept, send_char, SIZE, 0);
		}

		data_encode.clear();
		++j;
		cout << j << endl;  // 发送端一直在发送
	}
	cout << "发送完成";
	return true;
}

bool Server::receive_data() {
	Mat frame;
	vector<uchar> data_decode;
	std::vector<int> params;  // 压缩参数
	params.resize(3, 0);
	params[0] = IMWRITE_JPEG_QUALITY; // 无损压缩
	params[1] = 30;
	cv::namedWindow("Server", cv::WINDOW_NORMAL);
	char frams_cnt[10] = { 0, };
	// 解析总帧数
	int count = atoi(frams_cnt);
	int idx = 0;
	while (1) {
		// 解析图片字节长度
		int irecv = recv(m_accept, frams_cnt, 10, 0);
		int cnt = atoi(frams_cnt);
		data_decode.resize(cnt);//将队列大小重置为图片字节长度
		int index = 0;//表示接收数据长度计量
		count = cnt;//表示的是要从接收缓冲区接收字节的数量
		char *recv_char = new char[cnt];//新建一个字节数组 数组长度为图片字节长度
		while (count > 0)//这里只能写count > 0 如果写count >= 0 那么while循环会陷入一个死循环
		{
			//在网络通信中  recv 函数一次性接收到的字节数可能小于等于设定的SIZE大小，这时可能需要多次recv
			int iRet = recv(m_accept, recv_char, count, 0);
			for (int k = 0; k < iRet; k++)
			{
				data_decode[index++] = recv_char[k];
				if (index >= cnt) { break; }
			}
			if (!iRet) { return -1; }
			count -= iRet;//更新余下需要从接收缓冲区接收的字节数量
		}

		delete[]recv_char;
		try {
			frame = imdecode(data_decode, IMREAD_COLOR);
			imshow("Server", frame);
			waitKey(1);
		}
		catch(const char *msg)
		{
			continue;
		}
		data_decode.clear();
	}
	cout << "接受完成";
	return true;
}
bool Server::send_data_frame(Mat input) {
	Mat frame = input;
	vector<uchar> data_encode;
	std::vector<int> params;  // 压缩参数
	params.resize(3, 0);
	params[0] = IMWRITE_JPEG_QUALITY; // 无损压缩
	params[1] = 100;
	char frames_cnt[10] = { 0, };
	cout << "开始发送" << endl;

	m_file_in.push_back(frame.clone());
	imencode(".jpg", frame, data_encode, params);  // 对图像进行压缩
	int len_encoder = data_encode.size();

	_itoa_s(len_encoder, frames_cnt, 10);
	send(m_accept, frames_cnt, 10, 0);

	_itoa_s(SIZE, frames_cnt, 10);
	send(m_accept, frames_cnt, 10, 0);
	// 发送
	char send_char[SIZE] = { 0, };
	int index = 0;
	bool flag = false;
	for (int i = 0; i < len_encoder / SIZE + 1; ++i) {
		for (int k = 0; k < SIZE; ++k) {
			if (index >= data_encode.size()) {
				flag = true;
				break;
			}
			send_char[k] = data_encode[index++];
		}
		send(m_accept, send_char, SIZE, 0);
	}

	data_encode.clear();

	cout << "发送完成";
	return true;
}

bool Server::receive_data_frame(Mat& output) {
	Mat frame;
	vector<uchar> data_decode;
	std::vector<int> params;  // 压缩参数

	params.resize(3, 0);
	params[0] = IMWRITE_JPEG_QUALITY; // 无损压缩
	params[1] = 100;

	char frams_cnt[10] = { 0, };

	recv(m_accept, frams_cnt, 10, 0);
	// 解析总帧数
	int cnt = atoi(frams_cnt);
	std::cout << "frams_cnt " << frams_cnt <<" "<< cnt<< std::endl;
	recv(m_accept, frams_cnt, 10, 0);
	int size = atoi(frams_cnt);
	std::cout << "size " << size << std::endl;

	data_decode.resize(cnt);
	int index = 0;
	bool flag = true;
	char *recv_b = new char[cnt];
	std::cout << " cnt= " << cnt << std::endl;
	int iRecv = recv(m_accept, recv_b, cnt, 0);
	for (int i = 0; i < cnt; i++)
	{
		data_decode[index++] = recv_b[i];
	}
	std::cout << "data_decode" << data_decode.size() << std::endl;
	output = imdecode(data_decode, CV_LOAD_IMAGE_COLOR);
	//output = imdecode(data_decode, CV_LOAD_IMAGE_COLOR);
	std::cout << "  output.size " << output.size().width << "  "<< output.size().height  << std::endl;
	delete[]recv_b;
	data_decode.clear();
	cout << "接受完成";
	return true;
}

bool Server::free_connect() {
	m_cap.release();
	//关闭套接字
	closesocket(m_server);
	closesocket(m_accept);
	//释放DLL资源
	WSACleanup();
	return true;
}