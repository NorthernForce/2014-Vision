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

#include "cRIO.h"
#include "ImageProc.h"
#include "Control.h"

int main(int argc, char** argv) {
	bool server = false;
	bool disp_rgb = true;
	std::string histogramFile = "/root/target.yml";
	std::string cameraFile = "/root/camera.yml";
	for(int i = 0; i < argc; ++i) {
		if(argv[i] == std::string("--target")) {
			if(++i < argc) {
				histogramFile = argv[i];
			}
		} else if(argv[i] == std::string("server")) {
			server = true;
		}
	}

	Control control;
	cRIO crio(1726);
	crio.start();

	float gain1;
	float gain2;

	bool done_red = false;
	bool done_blue = false;
	bool done_back = false;

	// Effective focal lengths of the camera
	// For unknown focal lengths initialized to 1
	float fx = 1.0;
	float fy = 1.0;
	float cx = 80;
	float cy = 60;

	// Get video from /dev/video0
    cv::VideoCapture cam(0);
	// Set resolution to 160x120
    cam.set(CV_CAP_PROP_FRAME_WIDTH, 160);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, 120);
	cam.set(CV_CAP_PROP_EXPOSURE, 200);

	cv::MatND blue_hist;
	cv::MatND red_hist;
	cv::MatND background_hist;

	cv::Mat_<double> cameraMatrix;
	cv::Mat_<double> distCoefficients;

	float tanCamAngle = 0.0;

	cv::FileStorage histograms(histogramFile, cv::FileStorage::READ);
	if(histograms.isOpened()) {
		histograms["blue_ball"] >> blue_hist;
		histograms["red_ball"] >> red_hist;
		histograms["done_red"] >> done_red;
		histograms["done_blue"] >> done_blue;
		histograms["done_background"] >> done_back;
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
	}

	cv::FileStorage camera(cameraFile, cv::FileStorage::READ);
	if(camera.isOpened()) {
		camera["Camera_Matrix"] >> cameraMatrix;
		camera["Distortion_Coefficients"] >> distCoefficients;

		fx = cameraMatrix(0,0);
		fy = cameraMatrix(1,1);

		cx = cameraMatrix(0,2);
		cy = cameraMatrix(1,2);
	}


	// Counter and timestamp for computing framerate
    int n = 0;
    std::chrono::time_point<std::chrono::system_clock> last = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> now;

    cv::Mat_<cv::Vec3b> frame(cam.get(CV_CAP_PROP_FRAME_HEIGHT), cam.get(CV_CAP_PROP_FRAME_WIDTH));
	cv::Mat_<unsigned char> backprojection(frame.size());
	cv::Mat_<cv::Vec3b> backout(frame.size());

	if(!server) {
		cv::namedWindow("Frame");
	}

    while(true) {
		Alliance alliance = BLUE;
		/*
		if(server) {
			alliance = crio.getAlliance();
		}
		*/

		cam >> frame;

		if(alliance == NONE) {
			// Sleep for 20ms
			control.reset();
			usleep(20*1000);
			continue;
		}

		// Get next frame from the camera.
		if (
			( (alliance == RED ) && done_red  ) || 
		    ( (alliance == BLUE) && done_blue ) 
		) {
			switch(alliance) {
			case RED:
				img::backproject(frame, backprojection, gain1, gain2, red_hist, background_hist);
				break;
			case BLUE:
				img::backproject(frame, backprojection, gain1, gain2, blue_hist, background_hist);
				break;
			default:
				continue;
			}

			if(!disp_rgb) {
				cv::cvtColor(backprojection, frame, CV_GRAY2RGB);
			}

			cv::Point centroid;
			std::vector<std::vector<cv::Point>> contours;
			int i = img::findBall(backprojection, centroid, contours);

			if(i >= 0) {
				now = std::chrono::system_clock::now();
				std::chrono::duration<float> _dt = now - last;
				now = last;
				float dt = _dt.count();
				int size = cv::contourArea(contours[i]);

				cv::Point2f ball;
				// Tangent of yaw to the ball
				ball.x = (float)(cx - centroid.x) / fx;
				// Tangent of elevation to the ball
				ball.y = (float)(tanCamAngle + (cy - centroid.y)/fy) / (1 - tanCamAngle*(cy - centroid.y)/fy);
				//std::cout << ball.x << " " << ball.y << std::endl;

				cv::drawContours(frame, contours, i, cv::Scalar(0,255,0), 1);
				cv::circle(frame, centroid, 3, cv::Scalar(0,255,0), CV_FILLED);

				crio.send(control.getOutput(ball,size,dt));
			}
		}

		cv::circle(frame, cv::Point(frame.size().width/2, frame.size().height/2), 25, cv::Scalar(0,255,0), 1);

		if(!server) {
			cv::imshow("Frame", frame);
		}

        switch(cv::waitKey(1)) {
        case 'q':
        case 'Q':
            goto done;
        case 'x':
        case 'X':
            disp_rgb = !disp_rgb;
            break;
		case 'b':
		case 'B':
			img::doHistogram(frame, done_blue, blue_hist, background_hist);
			done_blue = true;
			done_back = true;
		case 'r':
		case 'R':
			img::doHistogram(frame, done_red, red_hist, background_hist);
			done_red = true;
			done_back = true;
			break;
		case 't':
		case 'T':
			break;
		case 's':
		case 'S':
			histograms.open(histogramFile, cv::FileStorage::WRITE);
			histograms << "red_ball" << red_hist;
			histograms << "blue_ball" << blue_hist;
			histograms << "done_red" << done_red;
			histograms << "done_blue" << done_blue;
			histograms << "done_background" << done_back;
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
