#ifndef _MANAGER_H
#define _MANAGER_H

#include "deepsort.h"
#include "logging.h"
#include "time.h"
#include <assert.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "deepsort.h"
#include "yolov5_lib.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

using std::vector;
using namespace cv;
//static Logger gLogger;
class Trtyolosort
{
  public:
    // init
    Trtyolosort(char *yolo_engine_path, char *sort_engine_path);

    // detect and show
    cv::Mat TrtDetect(cv::Mat &frame, float &conf_thresh, std::vector<DetectBox> &det, int &number_of_frame);
    cv::Mat showDetection(cv::Mat &img, std::vector<DetectBox> &boxes, int &fps, int &number_of_frame);
    float calculateSpeed(int id, int fps, int start_frame, int end_frame);

    // Temp Variable Params
    std::vector<int> temp_up_list;
    std::vector<int> temp_down_list;

    // Intersect Params
    map<int, float> intersect_x;
    map<int, float> intersect_y;

    // Speed Params
    map<int, int> section;
    map<int, int> start_frames;
    map<int, int> end_frames;
    map<int, float> velocity;
    map<int, int> direction;

    // X in Line Position
    float x1_line_position_first = 0;
    float x2_line_position_first = 700; //resolusi width
    float x1_line_position_second = 2;
    float x2_line_position_second = 700;

    // Line distance to calculate speed
    float line_distance = 35;

    // Y in Line Position , kalo bisa middle line position dan firstnya beda 30 aja.
    float middle_line_position_first = 250 ; //fisrt line
    float up_line_position_first = middle_line_position_first - 15;
    float down_line_position_first = middle_line_position_first + 15;
    float middle_line_position_second = 300; //second line
    float up_line_position_second = middle_line_position_second - 15;
    float down_line_position_second = middle_line_position_second + 15;

    std::vector<float>  vector_moving_average_speed_up;
    std::vector<float> vector_moving_average_speed_down;

  private:
    // Engine Path
    char *yolo_engine_path_ = NULL;
    char *sort_engine_path_ = NULL;
    void *trt_engine = NULL;
    
    // Deepsort parms
    DeepSort *DS;
    std::vector<DetectBox> t;
};
#endif // _MANAGER_H
