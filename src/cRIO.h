#pragma once

#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <thread>

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <opencv2/core/core.hpp>

enum Alliance {
	NONE = 0,
	RED = 1,
	BLUE = 2,
};

class cRIO {
private:
	int out_sock;
	int cmd_sock;

	struct sockaddr_in out_sa;
	struct sockaddr_in cmd_sa;

	char out_buf[3];
	char cmd_buf[256];

	Alliance alliance;

	std::thread cmd_thread;

	void recv_cmd();
public:
	cRIO(int port);

	int send(float x, float y, float w);
	int send(cv::Point3f o);
	Alliance getAlliance();
	void start();
};
