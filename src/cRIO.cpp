#include "cRIO.h"

cRIO::cRIO(int port) :
	alliance(NONE)
{
	out_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(out_sock == -1) {
		std::cout << "Error creating socket\n";
		exit(1);
	}
	
	memset(&out_sa, 0, sizeof out_sa);
	out_sa.sin_family = AF_INET;
	out_sa.sin_addr.s_addr = inet_addr("10.1.72.2");
	out_sa.sin_port = htons(1726);

	cmd_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(cmd_sock == -1) {
		std::cout << "Error creating socket\n";
		exit(1);
	}
	
	memset(&cmd_sa, 0, sizeof cmd_sa);
	cmd_sa.sin_family = AF_INET;
	cmd_sa.sin_addr.s_addr = inet_addr("10.1.72.2");
	cmd_sa.sin_port = htons(1726);
}

void cRIO::recv_cmd() {
	while(true) {
		printf("Trying to connect...\n");
		if(connect(cmd_sock, (struct sockaddr*)&cmd_sa, sizeof cmd_sa) != -1) {
			printf("Connected\n");
			while(true) {
				int bytes = recv(cmd_sock, cmd_buf, sizeof cmd_buf, 0);
				if(bytes == -1) break;
				alliance = (Alliance)cmd_buf[bytes-1];
				switch(alliance) {
					case RED:
						//printf("Start RED\n");
						break;
					case BLUE:
						//printf("Start BLUE\n");
						break;
					case NONE:
					default:
						//printf("Stop\n");
						break;
				}
			}
		} else {
			sleep(1);
			printf("Failed to connect, retrying...\n");
		} 
	}
}

int cRIO::send(float x, float y, float w) {
	if(x > 1.0)  x =  1.0;
	if(x < -1.0) x = -1.0;
	if(y > 1.0)  y =  1.0;
	if(y < -1.0) y = -1.0;
	if(w > 1.0)  w =  1.0;
	if(w < -1.0) w = -1.0;
	out_buf[0] = static_cast<unsigned char>(x>=0 ? x*127 : x*128 + 256);
	out_buf[1] = static_cast<unsigned char>(y>=0 ? y*127 : y*128 + 256);
	out_buf[2] = static_cast<unsigned char>(w>=0 ? w*127 : w*128 + 256);
	return sendto(out_sock, out_buf, sizeof out_buf, 0, (struct sockaddr*)&out_sa, sizeof out_sa);
}

int cRIO::send(cv::Point3f o) { return send(o.x, o.y, o.z); }

Alliance cRIO::getAlliance() {
	return alliance;
}

void cRIO::start() {
	cmd_thread = std::thread(&cRIO::recv_cmd, this);
}
