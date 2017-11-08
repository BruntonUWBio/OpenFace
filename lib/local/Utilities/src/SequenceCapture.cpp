///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017, Tadas Baltrusaitis, all rights reserved.
//
// ACADEMIC OR NON-PROFIT ORGANIZATION NONCOMMERCIAL RESEARCH USE ONLY
//
// BY USING OR DOWNLOADING THE SOFTWARE, YOU ARE AGREEING TO THE TERMS OF THIS LICENSE AGREEMENT.  
// IF YOU DO NOT AGREE WITH THESE TERMS, YOU MAY NOT USE OR DOWNLOAD THE SOFTWARE.
//
// License can be found in OpenFace-license.txt
//
//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite at least one of the following works:
//
//       OpenFace: an open source facial behavior analysis toolkit
//       Tadas Baltru�aitis, Peter Robinson, and Louis-Philippe Morency
//       in IEEE Winter Conference on Applications of Computer Vision, 2016  
//
//       Rendering of Eyes for Eye-Shape Registration and Gaze Estimation
//       Erroll Wood, Tadas Baltru�aitis, Xucong Zhang, Yusuke Sugano, Peter Robinson, and Andreas Bulling 
//       in IEEE International. Conference on Computer Vision (ICCV),  2015 
//
//       Cross-dataset learning and person-speci?c normalisation for automatic Action Unit detection
//       Tadas Baltru�aitis, Marwa Mahmoud, and Peter Robinson 
//       in Facial Expression Recognition and Analysis Challenge, 
//       IEEE International Conference on Automatic Face and Gesture Recognition, 2015 
//
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       Tadas Baltru�aitis, Peter Robinson, and Louis-Philippe Morency. 
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////

#include "SequenceCapture.h"

#include <iostream>

// Boost includes
#include <filesystem.hpp>
#include <filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

using namespace Utilities;

bool SequenceCapture::Open(std::vector<std::string> arguments)
{

	// Consuming the input arguments
	bool* valid = new bool[arguments.size()];

	for (size_t i = 0; i < arguments.size(); ++i)
	{
		valid[i] = true;
	}

	std::string input_root = "";

	std::string separator = std::string(1, boost::filesystem::path::preferred_separator);

	// First check if there is a root argument (so that videos and input directories could be defined more easily)
	for (size_t i = 0; i < arguments.size(); ++i)
	{
		if (arguments[i].compare("-root") == 0)
		{
			input_root = arguments[i + 1] + separator;
			i++;
		}
		if (arguments[i].compare("-inroot") == 0)
		{
			input_root = arguments[i + 1] + separator;
			i++;
		}
	}

	std::string input_video_file;
	std::string input_sequence_directory;
	int device = -1;

	bool file_found = false;

	for (size_t i = 0; i < arguments.size(); ++i)
	{
		if (!file_found && arguments[i].compare("-f") == 0)
		{
			input_video_file = (input_root + arguments[i + 1]);
			valid[i] = false;
			valid[i + 1] = false;
			i++;
			file_found = true;
		}
		else if (!file_found && arguments[i].compare("-fdir") == 0)
		{
			input_sequence_directory = (input_root + arguments[i + 1]);
			valid[i] = false;
			valid[i + 1] = false;
			i++;
			file_found = true;
		}
		else if (arguments[i].compare("-fx") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> fx;
			i++;
		}
		else if (arguments[i].compare("-fy") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> fy;
			i++;
		}
		else if (arguments[i].compare("-cx") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> cx;
			i++;
		}
		else if (arguments[i].compare("-cy") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> cy;
			i++;
		}
		else if (arguments[i].compare("-device") == 0)
		{
			std::stringstream data(arguments[i + 1]);
			data >> device;
			valid[i] = false;
			valid[i + 1] = false;
			i++;
		}
	}

	for (int i = arguments.size() - 1; i >= 0; --i)
	{
		if (!valid[i])
		{
			arguments.erase(arguments.begin() + i);
		}
	}

	// Based on what was read in open the sequence TODO
	if (device != -1)
	{
		return OpenWebcam(device, 640, 480, fx, fy, cx, cy);
	}
	if (!input_video_file.empty())
	{
		return OpenVideoFile(input_video_file, fx, fy, cx, cy);
	}
	if (!input_sequence_directory.empty())
	{
		return OpenImageSequence(input_sequence_directory, fx, fy, cx, cy);
	}

}

bool SequenceCapture::OpenWebcam(int device, int image_width, int image_height, float fx, float fy, float cx, float cy)
{

	if (device < 0)
	{
		std::cout << "Specify a valid device" << std::endl;
		return false;
	}

	latest_frame = cv::Mat();
	latest_gray_frame = cv::Mat();

	capture.open(device);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, image_width);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, image_height);

	is_webcam = true;
	is_image_seq = false;

	vid_length = 0;
	frame_num = 0;

	this->frame_width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	this->frame_height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);

	if (!capture.isOpened())
	{
		std::cout << "Failed to open the webcam" << std::endl;
		return false;
	}
	if (frame_width != image_width || frame_height != image_height)
	{
		std::cout << "Failed to open the webcam with desired resolution" << std::endl;
		std::cout << "Defaulting to " << frame_width << "x" << frame_height << std::endl;
	}

	this->fps = capture.get(CV_CAP_PROP_FPS);

	// If optical centers are not defined just use center of image
	if (cx == -1)
	{
		cx = frame_width / 2.0f;
		cy = frame_height / 2.0f;
	}
	// Use a rough guess-timate of focal length
	if (fx == -1)
	{
		fx = 500 * (frame_width / 640.0);
		fy = 500 * (frame_height / 480.0);

		fx = (fx + fy) / 2.0;
		fy = fx;
	}

	return true;

}

// TODO proper destructors and move constructors

bool SequenceCapture::OpenVideoFile(std::string video_file, float fx, float fy, float cx, float cy)
{
	latest_frame = cv::Mat();
	latest_gray_frame = cv::Mat();

	capture.open(video_file);

	this->fps = capture.get(CV_CAP_PROP_FPS);
	
	is_webcam = false;
	is_image_seq = false;
	
	this->frame_width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	this->frame_height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);

	vid_length = capture.get(CV_CAP_PROP_FRAME_COUNT);
	frame_num = 0;

	if (capture.isOpened())
	{
		std::cout << "Failed to open the video file at location: " << video_file << std::endl;
		return false;
	}

	// If optical centers are not defined just use center of image
	if (cx == -1)
	{
		cx = frame_width / 2.0f;
		cy = frame_height / 2.0f;
	}
	// Use a rough guess-timate of focal length
	if (fx == -1)
	{
		fx = 500 * (frame_width / 640.0);
		fy = 500 * (frame_height / 480.0);

		fx = (fx + fy) / 2.0;
		fy = fx;
	}

	return true;

}

bool SequenceCapture::OpenImageSequence(std::string directory, float fx, float fy, float cx, float cy)
{
	image_files.clear();

	boost::filesystem::path image_directory(directory);
	std::vector<boost::filesystem::path> file_in_directory;
	copy(boost::filesystem::directory_iterator(image_directory), boost::filesystem::directory_iterator(), back_inserter(file_in_directory));

	// Sort the images in the directory first
	sort(file_in_directory.begin(), file_in_directory.end());

	std::vector<std::string> curr_dir_files;

	for (std::vector<boost::filesystem::path>::const_iterator file_iterator(file_in_directory.begin()); file_iterator != file_in_directory.end(); ++file_iterator)
	{
		// Possible image extension .jpg and .png
		if (file_iterator->extension().string().compare(".jpg") == 0 || file_iterator->extension().string().compare(".png") == 0)
		{
			curr_dir_files.push_back(file_iterator->string());
		}
	}

	image_files = curr_dir_files;

	if (image_files.empty())
	{
		std::cout << "No images found in the directory: " << directory << std::endl;
		return false;
	}

	// Assume all images are same size in an image sequence
	cv::Mat tmp = cv::imread(image_files[0], -1);
	this->frame_height = tmp.size().height;
	this->frame_width = tmp.size().width;

	// If optical centers are not defined just use center of image
	if (cx == -1)
	{
		cx = frame_width / 2.0f;
		cy = frame_height / 2.0f;
	}
	// Use a rough guess-timate of focal length
	if (fx == -1)
	{
		fx = 500 * (frame_width / 640.0);
		fy = 500 * (frame_height / 480.0);

		fx = (fx + fy) / 2.0;
		fy = fx;
	}

	// No fps as we have a sequence
	this->fps = 0;

	return true;

}

cv::Mat SequenceCapture::GetNextFrame()
{
	frame_num++;

	if (is_webcam && !is_image_seq)
	{

		bool success = capture.read(latest_frame);

		if (!success)
		{
			// Indicate lack of success by returning an empty image
			latest_frame = cv::Mat();
		}
	}
	else if (is_image_seq)
	{
		if (image_files.empty())
		{
			// Indicate lack of success by returning an empty image
			latest_frame = cv::Mat();
		}

		latest_frame = cv::imread(image_files[frame_num-1], -1);
	}

	// Set the grayscale frame
	if (grayFrame == nullptr) {
		if (latestFrame->Width > 0) {
			grayFrame = gcnew OpenCVWrappers::RawImage(latestFrame->Width, latestFrame->Height, CV_8UC1);
		}
	}

	if (grayFrame != nullptr) {
		cvtColor(latestFrame->Mat, grayFrame->Mat, CV_BGR2GRAY);
	}

	return latest_frame;
}

double SequenceCapture::GetProgress()
{
	if (is_webcam)
	{
		return -1.0;
	}
	else
	{
		return (double)frame_num / (double)vid_length;
	}
}

bool SequenceCapture::IsOpened()
{
	if (is_webcam || !is_image_seq)
		return capture.isOpened();
	else
		return (image_files.size() > 0 && frame_num < image_files.size());
}

cv::Mat_<uchar> SequenceCapture::GetGrayFrame() {
	if (img_grabbed)
	{
		img_grabbed = false;
		return latest_gray_frame;
	}
	else
	{
		std::cout << "Need to call GetNextFrame(), before calling GetGrayFrame() " << std::endl;
		return cv::Mat_<uchar>();
	}
}