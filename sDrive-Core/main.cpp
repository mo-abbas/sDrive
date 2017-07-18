#define _SCL_SECURE_NO_WARNINGS
#define BUILD_DLL 1

#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Classifier.h"
#include "ObjectExtractor.h"

namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

using namespace std;
using namespace cv;

#define VIDEO_FORMAT VideoWriter::fourcc('M', 'P', '4', 'V')

extern "C" __declspec(dllexport) bool __cdecl processVideos(char* directory)
{
	boost::system::error_code ec;

	do
		fs::remove_all("Output", ec);
	while (ec != 0);

	pt::ptree config;

	if (fs::exists("config.ini"))
		pt::ini_parser::read_ini("config.ini", config);
	else
		return false;
	
	fs::path dir(directory);

	vector<VideoCapture> videos_input;
	vector<pair<VideoWriter, Direction>> videos_disp_ouput;
	vector<pair<VideoWriter, Direction>> videos_road_ouput;
	vector<pair<VideoWriter, Direction>> videos_box_ouput;
	vector<pair<VideoWriter, Direction>> videos_seg_ouput;

	static const string directions[] = {"front", "right", "back", "left"};

	VideoCapture front_left((dir / fs::path("front_left.mp4")).string());
	VideoCapture front_right((dir / fs::path("front_right.mp4")).string());

	if (front_left.isOpened() && front_right.isOpened())
	{
		videos_input.push_back(front_left);
		videos_input.push_back(front_right);

		videos_disp_ouput.push_back({ VideoWriter(), FRONT });
		videos_road_ouput.push_back({ VideoWriter(), FRONT });
		videos_box_ouput.push_back({ VideoWriter(), FRONT });
		videos_seg_ouput.push_back({ VideoWriter(), FRONT });
	}

	VideoCapture right_left((dir / fs::path("right_left.mp4")).string());
	VideoCapture right_right((dir / fs::path("right_right.mp4")).string());

	if (right_left.isOpened() && right_right.isOpened())
	{
		videos_input.push_back(right_left);
		videos_input.push_back(right_right);

		videos_disp_ouput.push_back({ VideoWriter(), RIGHT });
		videos_road_ouput.push_back({ VideoWriter(), RIGHT });
		videos_box_ouput.push_back({ VideoWriter(), RIGHT });
		videos_seg_ouput.push_back({ VideoWriter(), RIGHT });
	}

	VideoCapture back_left((dir / fs::path("back_left.mp4")).string());
	VideoCapture back_right((dir / fs::path("back_right.mp4")).string());

	if (back_left.isOpened() && back_right.isOpened())
	{
		videos_input.push_back(back_left);
		videos_input.push_back(back_right);

		videos_disp_ouput.push_back({ VideoWriter(), BACK });
		videos_road_ouput.push_back({ VideoWriter(), BACK });
		videos_box_ouput.push_back({ VideoWriter(), BACK });
		videos_seg_ouput.push_back({ VideoWriter(), BACK });
	}

	VideoCapture left_left((dir / fs::path("left_left.mp4")).string());
	VideoCapture left_right((dir / fs::path("left_right.mp4")).string());

	if (left_left.isOpened() && left_right.isOpened())
	{
		videos_input.push_back(left_left);
		videos_input.push_back(left_right);

		videos_disp_ouput.push_back({ VideoWriter(), LEFT });
		videos_road_ouput.push_back({ VideoWriter(), LEFT });
		videos_box_ouput.push_back({ VideoWriter(), LEFT });
		videos_seg_ouput.push_back({ VideoWriter(), LEFT });
	}

	if (videos_input.size() == 0)
		return false;

	const double VIDEO_FPS = videos_input[0].get(CAP_PROP_FPS);
	const double VIDEO_WIDTH = videos_input[0].get(CAP_PROP_FRAME_WIDTH);
	const double VIDEO_HEIGHT = videos_input[0].get(CAP_PROP_FRAME_HEIGHT);

	string stereo_model_file = config.get<string>("models.stereo_model_file");
	string stereo_trained_file = config.get<string>("models.stereo_trained_file");
	string road_model_file = config.get<string>("models.road_model_file");
	string road_trained_file = config.get<string>("models.road_trained_file");

	if (!fs::exists(stereo_model_file) || !fs::exists(stereo_trained_file) ||
		!fs::exists(road_model_file) || !fs::exists(road_trained_file))
		return false;

	Classifier stereoClassifier(stereo_model_file, stereo_trained_file);
	Classifier roadClassifier(road_model_file, road_trained_file);

	fs::create_directory("Output");
	fs::create_directory("Output\\Disparity");
	fs::create_directory("Output\\Road");
	fs::create_directory("Output\\Boxes");
	fs::create_directory("Output\\Segments");
	
	pt::ptree results;
	results.put("input.width", to_string(VIDEO_WIDTH));
	results.put("input.height", to_string(VIDEO_HEIGHT));
	results.put("input.fps", to_string(VIDEO_FPS));

	const float FOVx = 90.0f;
	float BASELINE = config.get<float>("settings.baseline");
	const float PIXEL_SIZE = 1.0f;	
	
	for (int frame = 1;; frame++)
	{
		vector<Mat> disparityVector;
		vector<Road> roadVector;

		for (int i = 0; i < videos_input.size(); i += 2)
		{
			Mat i_left, i_right;

			if (!videos_input[i].read(i_left) || !videos_input[i + 1].read(i_right))
				return true;			
			
			vector<Mat> input;

			input.push_back(i_left);
			input.push_back(i_right);

			Mat disp_output = stereoClassifier.Predict(input);
			disp_output.convertTo(disp_output, CV_8U);
			
			if (frame == 1)
				videos_disp_ouput[i / 2].first.open("Output\\Disparity\\disp_" + directions[videos_disp_ouput[i / 2].second] + ".mp4", VIDEO_FORMAT, VIDEO_FPS, cv::Size(VIDEO_WIDTH, VIDEO_HEIGHT), false);
			
			if (!videos_disp_ouput[i / 2].first.isOpened())
				return false;

			videos_disp_ouput[i / 2].first << disp_output;

			Mat i_road;
			i_left.convertTo(i_road, CV_32FC3);
			i_road = i_road - Scalar(103.939, 116.779, 123.68);

			input.clear();
			input.push_back(i_road);
			Mat road_ouput = roadClassifier.Predict(input);
			road_ouput = 255 - (road_ouput * 255);
			road_ouput.convertTo(road_ouput, CV_8U);	

			if (frame == 1)
				videos_road_ouput[i / 2].first.open("Output\\Road\\road_" + directions[videos_road_ouput[i / 2].second] + ".mp4", VIDEO_FORMAT, VIDEO_FPS, cv::Size(VIDEO_WIDTH, VIDEO_HEIGHT), true);
				
			if (!videos_road_ouput[i / 2].first.isOpened())
				return false;

			vector<Mat> road_merge;		
			Mat road_3C, dst;

			road_merge.push_back(Mat::zeros(road_ouput.rows, road_ouput.cols, CV_8U));
			road_merge.push_back(road_ouput);
			road_merge.push_back(Mat::zeros(road_ouput.rows, road_ouput.cols, CV_8U));
			merge(road_merge, road_3C);
			addWeighted(i_left, 0.7, road_3C, 0.3, 0.0, dst);

			videos_road_ouput[i / 2].first << dst;
			
			disp_output.convertTo(disp_output, CV_32F);
			disparityVector.push_back(disp_output.clone());

			roadVector.push_back(Road(road_ouput.clone(), videos_road_ouput[i / 2].second));			
		}

		float FOCAL_LENGTH = disparityVector[0].cols / 2;

		Vec3f cameraLocation(-BASELINE / 2, 0, -BASELINE / 2);

		PointCloud pointCloud(disparityVector, FOVx, BASELINE, FOCAL_LENGTH, PIXEL_SIZE, cameraLocation);
		OffRoadClipper clipper(roadVector, pointCloud);
		//clipper.AdjustFromPrevious(clipper.leftCoefficients, clipper.rightCoefficients);
		clipper.Clip();

		pointCloud.GetValuesFromClipper(clipper);

		ObjectExtractor oe(pointCloud);

		// center[0] is the X-axis and center[1] is the Z-axis (the axis going into the screen)
		vector<Vec2f> carsCenter = oe.GetCars();

		// coeff[0] = coeff(x^0)
		// coeff[1] = coeff(x)
		// coeff[2] = coeff(x^2)
		// ...
		// currently we use the first two only
		pair<Mat, Mat> roadCoefficients = oe.GetRoadBorders();

		vector<Mat> boxes = oe.VisualizeBoxes();
		vector<Mat> segments = oe.VisualizeSegmentation();

		for (int i = 0; i < videos_input.size(); i += 2)
		{
			if (frame == 1)
				videos_box_ouput[i / 2].first.open("Output\\Boxes\\box_" + directions[videos_box_ouput[i / 2].second] + ".mp4", VIDEO_FORMAT, VIDEO_FPS, cv::Size(VIDEO_WIDTH, VIDEO_HEIGHT), true);

			if (!videos_box_ouput[i / 2].first.isOpened())
				return false;

			videos_box_ouput[i / 2].first << boxes[i / 2];

			if (frame == 1)
				videos_seg_ouput[i / 2].first.open("Output\\Segments\\seg_" + directions[videos_seg_ouput[i / 2].second] + ".mp4", VIDEO_FORMAT, VIDEO_FPS, cv::Size(VIDEO_WIDTH, VIDEO_HEIGHT), true);

			if (!videos_seg_ouput[i / 2].first.isOpened())
				return false;

			videos_seg_ouput[i / 2].first << segments[i / 2];
		}

		string key = "frames." + to_string(frame) + ".cars";

		pt::ptree cars_node;
		for (int i = 0; i < carsCenter.size(); i++)
		{
			pt::ptree car_node;
			car_node.put("", to_string(carsCenter[i][0]) + "," + to_string(carsCenter[i][1]));
			cars_node.push_back(make_pair("", car_node));
		}
		results.add_child(key, cars_node);

		key = "frames." + to_string(frame) + ".road.left";
		string road_left_coeffs = "";

		for (int i = 0; i < roadCoefficients.first.rows; i++)
			road_left_coeffs += to_string(roadCoefficients.first.at<float>(i, 0)) + ",";

		road_left_coeffs.pop_back();	//remove last comma
		results.put(key, road_left_coeffs);

		key = "frames." + to_string(frame) + ".road.right";
		string road_right_coeffs = "";

		for (int i = 0; i < roadCoefficients.second.rows; i++)
			road_right_coeffs += to_string(roadCoefficients.second.at<float>(i, 0)) + ",";

		road_right_coeffs.pop_back();	//remove last comma
		results.put(key, road_right_coeffs);

		pt::write_json("Output\\results.json", results);
	}

	return true;
}

//int main()
//{
//	processVideos("D:\\sDrive\\sDrive-GUI\\build\\Input");
//	cin.ignore();
//	return 0;
//}