#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <unistd.h>
#include <termios.h>
#include <chrono>

float h_range[] = {0,128};
float s_range[] = {0,256};
float v_range[] = {0,256};
const float* ranges[] = {h_range, s_range};//, v_range};
int channels[] = {0,1};

/**
 * 
 * Performs two histogram backproejctions. The first is using a
 * histogram of the target, the second is a histogram of the
 * background. The two resulting images are then combined 
 * based on a threshold involving gain1 and gain2, yeilding
 * a single binary image.
 *
 * \param frame input image to perform backprojections on.
 * \param output binary output image.
 * \param gain1
 * \param gain2
 * \param target_hist histogram of the target.
 * \param background_hist histogram of the background
 */
void backproject(cv::Mat_<cv::Vec3b>& frame, 
		         cv::Mat_<unsigned char>& output,
				 float gain1, 
				 float gain2, 
				 cv::MatND& target_hist, 
				 cv::MatND& background_hist) 
{
	cv::Mat_<cv::Vec3b> hsv(frame.size());
	cv::Mat_<unsigned char> target(frame.size());
	cv::Mat_<unsigned char> background(frame.size());
	cv::cvtColor(frame, hsv, CV_RGB2HSV);

	cv::calcBackProject(&hsv, 1, channels, target_hist, target, ranges, 1);
	cv::calcBackProject(&hsv, 1, channels, background_hist, background, ranges, 1);

	for(int j = 0; j < frame.rows; ++j) {
		for(int i = 0; i < frame.cols; ++i) {
			if(target(j,i) >= background(j,i)*(gain1 / 2.0f) + (gain2/2.0f)) {
				output(j, i) = 255;
			} else {
				output(j, i) = 0;
			}
		}
	}

	cv::erode(output, output, cv::Mat());
}

void doHistogram(cv::Mat_<cv::Vec3b>& frame, cv::MatND& target_hist, cv::MatND& background_hist) {
    cv::Mat_<cv::Vec3b> hsv(frame.size());
    cv::cvtColor(frame, hsv, CV_RGB2HSV);
    int hbins = 32, sbins = 32;
    int histSize[] = {hbins,sbins};

    cv::Mat_<unsigned char> mask(frame.size());

    for(int j = 0; j < mask.rows; ++j) {
        for(int i = 0; i < mask.cols; ++i) {
            float x = i - mask.cols/2;
            float y = mask.rows/2 - j;
            float d = sqrt(x*x + y*y);
            if(d > 50.0f) {
                mask(j,i) = 0;
            } else {
                mask(j,i) = 255;
            }
        }
    }

    calcHist(&hsv, 1, channels, mask, target_hist, 2, histSize, ranges, true, false);

    for(int j = 0; j < mask.rows; ++j) {
        for(int i = 0; i < mask.cols; ++i) {
            float x = i - mask.cols/2;
            float y = mask.rows/2 - j;
            float d = sqrt(x*x + y*y);
            if(d <= 60.0f) {
                mask(j,i) = 0;
            } else {
                mask(j,i) = 255;
            }
        }
    }

    calcHist(&hsv, 1, channels, mask, background_hist, 2, histSize, ranges, true, false);
}

int main(int argc, char** argv) {
	int gain1 = 3;
	int gain2 = 0;
	bool done_hist = true;
	bool disp_rgb = true;
	std::string histogramFile = "/root/histograms.yml";

	for(int i = 0; i < argc; ++i) {
		if(argv[i] == std::string("--hist")) {
			if(++i < argc) {
				histogramFile = argv[i];
			}
		}
	}
	
    cv::namedWindow("Cam Proc");
    cv::createTrackbar("Gain 1", "Cam Proc", &gain1, 10);
    cv::createTrackbar("Gain 2", "Cam Proc", &gain2, 10);

	// Get video from /dev/video0
    cv::VideoCapture cam(0);
	// Set resolution to 160x120
    cam.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

	cv::MatND target_hist;
	cv::MatND background_hist;

	cv::FileStorage histograms(histogramFile, cv::FileStorage::READ);
	if(histograms.isOpened()) {
		histograms["target"] >> target_hist;
		histograms["background"] >> background_hist;
		done_hist = true;
	} else {
		std::cout << "Couldn't open histogram data file.\n";
		done_hist = false;
	}


	// Counter and timestamp for computing framerate
    int n = 0;
    std::chrono::time_point<std::chrono::system_clock> last = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> now;

    cv::Mat_<cv::Vec3b> frame(cam.get(CV_CAP_PROP_FRAME_HEIGHT), cam.get(CV_CAP_PROP_FRAME_WIDTH));
	cv::Mat_<unsigned char> backprojection(frame.size());

    int key;
    while(key = cv::waitKey(1)) {
		// Get next frame from the camera.
		cam >> frame;
		if(done_hist) {
			backproject(frame, backprojection, gain1, gain2, target_hist, background_hist);

			if(!disp_rgb) {
				cv::cvtColor(backprojection, frame, CV_GRAY2RGB);
			}

			std::vector<std::vector<cv::Point> > contours;
			cv::findContours(backprojection, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

			int bestContour_index = -1;
			float bestContour_score = 100.0;

			for(int i = 0; i < contours.size(); ++i) {
				if(cv::contourArea(contours[i]) > bestContour_score) {
					bestContour_index = i;
				}
			}

			if(bestContour_index >= 0) {
				cv::Moments m = cv::moments(contours[bestContour_index]);
				cv::drawContours(frame, contours, bestContour_index, cv::Scalar(255,0,0), 1);
				cv::circle(frame, cv::Point(m.m10/m.m00, m.m01/m.m00), 3, cv::Scalar(255, 0, 0), CV_FILLED);
			}

		}

		cv::circle(frame, cv::Point(frame.size().width/2, frame.size().height/2), 50, cv::Scalar(0,255,0), 1);
		cv::imshow("Cam Proc", frame);

	    // Compute framerate once every second
        now = std::chrono::system_clock::now();
	    std::chrono::duration<double> diff = now - last;
        n++;
        if(diff.count() >= 1.0) {
            std::cout << "FPS: " << (n/diff.count()) << "\n";
            last = now;
            n = 0;
        }

        switch(key) {
        case 'q':
        case 'Q':
            goto done;
        case 'x':
        case 'X':
            std::cout << "X Pressed" << std::endl;
            disp_rgb = !disp_rgb;
            break;
        case 'c':
        case 'C':
            doHistogram(frame, target_hist, background_hist);
			histograms.open(histogramFile, cv::FileStorage::WRITE);
			histograms << "target" << target_hist;
			histograms << "background " << background_hist;
			histograms.release();
			done_hist = true;
            break;
        default:
	        break;
        }
    }

done:
    return 0;
}
