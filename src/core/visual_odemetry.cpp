#include <vector>
#include <Eigen/Dense>
#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "visual_odemetry.hpp"

using namespace std;
using namespace cv;

VisualOdemetry::VisualOdemetry(Mat& img_initial_frame)
{
	/* intrinsic matrix */
	double fx, fy, cx, cy;
	fx = 656.24987;
	fy = 656.10660;
	cx = 327.36105;
	cy = 240.03464;

	intrinsic_matrix = (cv::Mat_<double>(3,3) << fx,  0, cx,
	                    0, fy, cy,
	                    0,  0,  1);

	/* exract features from initial frame */
	feature_detector.extract(img_initial_frame, last_features);

	/* set intial frame's relative position to be zero */
	for(int i = 0; i < 3; i++) {
		last_features.points_3d.push_back(cv::Point3f(0, 0, 0));
	}

	/* debug */
	img_initial_frame.copyTo(last_frame_img);
}

void VisualOdemetry::pose_estimation_pnp(Eigen::Matrix4f& T,
                VOFeatures& ref_frame_features,
                vector<cv::Point3f>& reference_points_3d,
                VOFeatures& curr_frame_features,
                vector<DMatch>& feature_matches)
{
	vector<Point3f> obj_points_3d;
	vector<Point2f> img_points_2d;

	for(cv::DMatch m:feature_matches) {
		obj_points_3d.push_back(reference_points_3d[m.queryIdx]);
		img_points_2d.push_back(curr_frame_features.keypoints[m.trainIdx].pt);
	}

	Mat rvec, tvec, inliers;
	solvePnPRansac(obj_points_3d, img_points_2d, intrinsic_matrix, Mat(), rvec, tvec,
	               false, 100, 4.0, 0.99, inliers);

	//int inlier_cnt = inliers.rows;
	//printf("pnp inliers count = %d\n", inlier_cnt);

	T << rvec.at<float>(0, 0), rvec.at<float>(0, 1), rvec.at<float>(0, 2), tvec.at<float>(0, 0),
             rvec.at<float>(1, 0), rvec.at<float>(1, 1), rvec.at<float>(1, 2), tvec.at<float>(1, 0),
             rvec.at<float>(2, 0), rvec.at<float>(2, 1), rvec.at<float>(2, 2), tvec.at<float>(2, 0),
                                0,                    0,                    0,                    1;
}

void VisualOdemetry::estimate(cv::Mat& new_img)
{
	Eigen::Matrix4f T_last_to_now; //transformation matrix

	feature_detector.extract(last_frame_img, last_features);

	VOFeatures new_features;
	feature_detector.extract(new_img, new_features);

	feature_detector.match(feature_matches, last_features, new_features);

	//pose_estimation_pnp(T_last_to_now, last_features, last_features.points_3d, new_features, feature_matches);

	feature_detector.plot_matched_features(last_frame_img, new_img, last_features, new_features, feature_matches);

	new_img.copyTo(last_frame_img);
	last_features = new_features;

	//printf("estimated position = %lf, %lf, %lf\n", T(0, 3), T(1, 3), T(2, 3));
}
