#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <unistd.h>
#include <termios.h>
#include <chrono>

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "ImageProc.h"
#include "PID.h"

enum TargetType {
	NONE = 0,
	RED  = 1,
	BLUE = 2,
};

enum Alliance {
	NONE = 0,
	RED  = 1,
	BLUE = 2,
};

enum HistogramsDone { 
	NONE_DONE = 0,
	RED_DONE  = 1,
	BLUE_DONE = 2,
	BOTH_DONE = 4,
};

int main(int argc, char** argv) {
	//--------------- Feedback socket -----------------
	int sock;
	struct sockaddr_in sa;

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock == -1) {
		std::cout << "Error creating socket\n";
		exit(1);
	}
	
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("10.1.72.2");
	sa.sin_port = htons(1726);


	//int gain1_cv = 50;
	int gain1_cv;
	int gain2_cv;
	float gain1;
	float gain2;
	int done_hist = NONE_DONE;
	bool disp_rgb = true;
	bool server = false;
	int curTarget = NONE;
	std::string histogramFile = "/root/target.yml";

	for(int i = 0; i < argc; ++i) {
		if(argv[i] == std::string("--target")) {
			if(++i < argc) {
				histogramFile = argv[i];
			}
		} else if(argv[i] == std::string("server")) {
			server = true;
		}
	}
	

	// Get video from /dev/video0
    cv::VideoCapture cam(0);
	// Set resolution to 160x120
    cam.set(CV_CAP_PROP_FRAME_WIDTH, 160);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, 120);
    //cam.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    //cam.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

	cv::MatND blue_hist;
	cv::MatND red_hist;
	cv::MatND background_hist;

	float tanCamAngle = 0.0;

	cv::FileStorage histograms(histogramFile, cv::FileStorage::READ);
	if(histograms.isOpened()) {
		histograms["blue_ball"] >> blue_hist;
		histograms["red_ball"] >> red_hist;
		histograms["done_hist"] >> done_hist;
		histograms["background"] >> background_hist;
		histograms["gain1"] >> gain1;
		histograms["gain2"] >> gain2;

		if(histograms["cam_angle"].isInt() ||
		   histograms["cam_angle"].isReal()) {
			float camAngle;
			histograms["cam_angle"] >> camAngle;
			tanCamAngle = tan(camAngle);
		}
	} else {
		gain1 = 2.5;
		gain2 = 0.1;

		std::cout << "Couldn't open histogram data file.\n";
		done_hist = NONE_DONE;
	}

	gain1_cv = static_cast<int>(gain1*10);
	gain2_cv = static_cast<int>(gain2);

	if(!server) {
		cv::namedWindow("Cam");
	}

	// TODO: Get this from driverstation in server mode;
	curTarget = BLUE;


	// Counter and timestamp for computing framerate
    int n = 0;
    std::chrono::time_point<std::chrono::system_clock> last = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> now;

    cv::Mat_<cv::Vec3b> frame(cam.get(CV_CAP_PROP_FRAME_HEIGHT), cam.get(CV_CAP_PROP_FRAME_WIDTH));
	cv::Mat_<unsigned char> backprojection(frame.size());
	cv::Mat_<cv::Vec3b> backout(frame.size());

	PID rotPID(0.007, 0, 0);
	PID catchPID(0.00001, 0, 0);
	float prev = 0.0;
	float prev_d = 0.0;

    int key;
    while(true) {
		int tt = curTarget;
		if(tt == NONE) {
			// Sleep for 20ms
			usleep(20*1000);
			continue;
		}

		key = cv::waitKey(1);
		// Get next frame from the camera.
		cam >> frame;
		if((done_hist&tt) == tt) {
			switch(tt) {
			case RED:
				backproject(frame, backprojection, gain1, gain2, red_hist, background_hist);
				break;
			case BLUE:
				backproject(frame, backprojection, gain1, gain2, blue_hist, background_hist);
				break;
			case NONE:
				continue;
			}

			if(!disp_rgb) {
				cv::cvtColor(backprojection, frame, CV_GRAY2RGB);
			}

			/*
			std::vector<std::vector<cv::Point> > contours;
			cv::findContours(backprojection, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

			int bestContour_index = -1;
			float bestContour_score = 500.0;

			for(int i = 0; i < contours.size(); ++i) {
				if(cv::contourArea(contours[i]) > bestContour_score) {
					bestContour_index = i;
				}
			}
			*/

			cv::Point centroid;
			int size = findBall(backprojection, centroid)

			if(size > 250) {
			}

			/*
			if(bestContour_index >= 0) {
				cv::Moments m = cv::moments(contours[bestContour_index]);
				cv::drawContours(frame, contours, bestContour_index, cv::Scalar(0,255,0), 1);
				cv::circle(frame, cv::Point(m.m10/m.m00, m.m01/m.m00), 3, cv::Scalar(0,255,0), CV_FILLED);

				float x = frame.cols/2 - m.m10/m.m00;
				float y = frame.rows/2 - m.m01/m.m00;

				now = std::chrono::system_clock::now();
				std::chrono::duration<double> _dt = now - last;
				last = now;
				float dt = _dt.count();

				float output1 = rotPID.getOutput(x, dt);
				if(output1 > 1.0) output1 = 1.0;
				if(output1 < -1.0) output1 = -1.0;

				y /= 60.0/tan(15.0);

				float tan_theta = (y + tanCamAngle) / (1 - y*tanCamAngle);
				//float tan_theta = y;
				float d_tan_theta = (tan_theta - prev) / dt;
				float dd_tan_theta = (d_tan_theta - prev_d) / dt;
				//printf("%f\n", dd_tan_theta);
				prev=tan_theta;
				prev_d=d_tan_theta;

				float output2 = catchPID.getOutput(dd_tan_theta, d_tan_theta, dt);
				//printf("%f\n",output2);
				if(output2 > 1.0) output2 = 1.0;
				if(output2 < -1.0) output2 = -1.0;

				char o1 = static_cast<char>(output1>0 ? output1*127 : output1*128 + 255);
				char o2 = static_cast<char>(output2>0 ? output2*127 : output2*128 + 255);
				char msg[] = { 0, o2, 0 };
				int bytesSent = sendto(sock, msg, sizeof msg, 0, (struct sockaddr*)&sa, sizeof sa);
			} else {
				last = std::chrono::system_clock::now();
				rotPID.reset();
				prev = 0.0;
				prev_d = 0.0;
				char msg[] = { 0, 0, 0 };
				int bytesSent = sendto(sock, msg, sizeof msg, 0, (struct sockaddr*)&sa, sizeof sa);
			}
			*/

		}

		cv::circle(frame, cv::Point(frame.size().width/2, frame.size().height/2), 25, cv::Scalar(0,255,0), 1);

		if(!server) {
			cv::imshow("Cam", frame);
			//cv::imshow("Binary", backout);
		}

	    // Compute framerate once every second
		/*
        now = std::chrono::system_clock::now();
	    std::chrono::duration<double> diff = now - last;
        n++;
        if(diff.count() >= 1.0) {
            std::cout << "FPS: " << (n/diff.count()) << "\n";
            last = now;
            n = 0;
        }
		*/

        switch(key) {
        case 'q':
        case 'Q':
            goto done;
        case 'x':
        case 'X':
            disp_rgb = !disp_rgb;
            break;
        case 'c':
        case 'C':
			switch(curTarget) {
			case RED:
				doHistogram(frame, done_hist&RED_DONE == RED_DONE, red_hist, background_hist);
				done_hist |= RED_DONE;
				break;
			case BLUE:
				doHistogram(frame, done_hist&BLUE_DONE == BLUE_DONE, blue_hist, background_hist);
				done_hist |= BLUE_DONE;
				break;
			default:
				break;
			}
            break;
		case 'r':
		case 'R':
			done_hist = NONE_DONE;
			break;
		case 't':
		case 'T':
			break;
		case 'b':
		case 'B':
			break;
		case 's':
		case 'S':
			histograms.open(histogramFile, cv::FileStorage::WRITE);
			histograms << "red_ball" << red_hist;
			histograms << "blue_ball" << blue_hist;
			histograms << "done_hist" << done_hist;
			histograms << "background " << background_hist;
			histograms << "gain1" << gain1;
			histograms << "gain2" << gain2;
			histograms.release();
			break;
        default:
	        break;
        }
    }

done:
    return 0;
}
