#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

namespace img {
	void backproject(cv::Mat_<cv::Vec3b>& frame, 
					 cv::Mat_<unsigned char>& output,
					 float gain1, 
					 float gain2, 
					 cv::MatND& target_hist, 
					 cv::MatND& background_hist);

	int findBall(cv::Mat_<unsigned char>& backprojection, cv::Point& centroid, std::vector<std::vector<cv::Point>>& contours);
	void doHistogram(cv::Mat_<cv::Vec3b>& frame, bool done_hist, cv::MatND& target_hist, cv::MatND& background_hist);
	void calcTargetHist(cv::Mat_<cv::Vec3b>& frame, bool done_hist, cv::MatND& target_hist);
	void calcBackgroundHist(cv::Mat_<cv::Vec3b>& frame, bool done_hist, cv::MatND& background_hist);
}
