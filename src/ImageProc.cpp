#include "ImageProc.h"

namespace img {

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
	//cv::Mat_<cv::Vec3b> back1(frame.size());
	//cv::Mat_<cv::Vec3b> back2(frame.size());
	cv::Mat_<unsigned char> target(frame.size());
	cv::Mat_<unsigned char> background(frame.size());
	cv::cvtColor(frame, hsv, CV_RGB2HSV);

	cv::calcBackProject(&hsv, 1, channels, target_hist, target, ranges, 1);
	cv::calcBackProject(&hsv, 1, channels, background_hist, background, ranges, 1);

	//cv::cvtColor(target, back1, CV_GRAY2RGB);
	//cv::cvtColor(background, back2, CV_GRAY2RGB);
	//cv::imshow("Backprojection -- Target", back1);
	//cv::imshow("Backprojection -- Background", back2);

	for(int j = 0; j < frame.rows; ++j) {
		for(int i = 0; i < frame.cols; ++i) {
			if(target(j,i) >= background(j,i)*gain1 + gain2) {
				output(j, i) = 255;
			} else {
				output(j, i) = 0;
			}
		}
	}

	//cv::erode(output, output, cv::Mat());
}

void doHistogram(cv::Mat_<cv::Vec3b>& frame, bool done_hist, cv::MatND& target_hist, cv::MatND& background_hist) {
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
            if(d > 25.0f) {
                mask(j,i) = 0;
            } else {
                mask(j,i) = 255;
            }
        }
    }

    calcHist(&hsv, 1, channels, mask, target_hist, 2, histSize, ranges, true, done_hist);

    for(int j = 0; j < mask.rows; ++j) {
        for(int i = 0; i < mask.cols; ++i) {
            float x = i - mask.cols/2;
            float y = mask.rows/2 - j;
            float d = sqrt(x*x + y*y);
            if(d <= 30.0f) {
                mask(j,i) = 0;
            } else {
                mask(j,i) = 255;
            }
        }
    }

    calcHist(&hsv, 1, channels, mask, background_hist, 2, histSize, ranges, true, done_hist);

    cv::normalize(target_hist, target_hist, 0, 255, cv::NORM_MINMAX);
    cv::normalize(background_hist, background_hist, 0, 255, cv::NORM_MINMAX);
}

int findBall(cv::Mat_<unsigned char>& backprojection, cv::Point& centroid, std::vector<std::vector<cv::Point>>& contours) {
	//std::vector<std::vector<cv::Point> > contours;
	cv::findContours(backprojection, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	int bestContour_index = -1;
	float bestContour_score = 0.0;

	for(int i = 0; i < contours.size(); ++i) {
		if(cv::contourArea(contours[i]) > bestContour_score) {
			bestContour_index = i;
		}
	}

	if(bestContour_index >= 0) {
		cv::Moments m = cv::moments(contours[bestContour_index]);
		//cv::drawContours(frame, contours, bestContour_index, cv::Scalar(0,255,0), 1);
		//cv::circle(frame, cv::Point(m.m10/m.m00, m.m01/m.m00), 3, cv::Scalar(0,255,0), CV_FILLED);

		centroid.x = m.m10/m.m00;
		centroid.y = m.m01/m.m00;

		//centroid = cv::Point(x,y);
		//ball = contours[bestContour_index];
		return bestContour_index;;
	}

	return -1;
}

};
