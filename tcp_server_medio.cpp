#include "stdafx.h"
#include "server.h"
#pragma comment(lib,"opencv_world340.lib")//引用库文件

int recv_online_video() {
	Server ser;
	int port = 7777;
	ser.initialization(port);
	ser.build_connect();
	ser.receive_data();
	ser.free_connect();
	return 0;
}

int recv_pic()
{
	Server ser;
	int port = 7777;
	ser.initialization(port);
	ser.build_connect();
	Mat output;
	ser.receive_data_frame(output);
	imshow("s", output);
	waitKey(0);

	ser.free_connect();
	return 0;
}

int main() {
	recv_online_video();
	//recv_pic();
	return 0;
}


