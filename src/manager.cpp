#include "manager.hpp"
#include "globals.h"
#include <algorithm>
#include <map>
#include <vector>
#include <time.h>
#include <ctime>
#define CST (+7)
using std::vector;
using namespace cv;
static Logger gLogger;
int total_up,total_down,bus_up,truck_up,bus_down,truck_down;

std::string TimeStr()
{
    time_t current_time;
    struct tm* ptime;
    time(&current_time);
    int ptime_year;
  
    ptime = gmtime(&current_time); 
    ptime_year = 1900+(ptime->tm_year); 	    
    std::string lbl_hour = cv::format("%d",(ptime->tm_hour + CST)); 
    std::string lbl_min = cv::format("%d",(ptime->tm_min));
    std::string lbl_sec = cv::format("%d",(ptime->tm_sec));
    
    std::string lbl_mday = cv::format("%d",(ptime->tm_mday));
    std::string lbl_mon = cv::format("%d",(ptime->tm_mon));
    std::string lbl_year = cv::format("%d",ptime_year);
    
    std::string time_wib= lbl_mday+":"+lbl_mon+":"+lbl_year+" "+lbl_hour+":"+lbl_min+":"+lbl_sec;;
    //+"-"+lbl_hour+":"+lbl_min+":"+lbl_sec;
    cout << time_wib << endl;
    
   return time_wib;
}

Trtyolosort::Trtyolosort(char *yolo_engine_path, char *sort_engine_path)
{
    sort_engine_path_ = sort_engine_path;
    yolo_engine_path_ = yolo_engine_path;
    trt_engine = yolov5_trt_create(yolo_engine_path_);
    printf("create yolov5-trt , instance = %p\n", trt_engine);
    DS = new DeepSort(sort_engine_path_, 128, 256, 0, &gLogger);
}

cv::Scalar idToColor(int obj_id)
{
    int const colors[6][3] = {{0, 1, 1}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}};
    int const offset = obj_id * 123457 % 6;
    int const color_scale = 200 + (obj_id * 123457) % 50;
    cv::Scalar color(colors[offset][0], colors[offset][1], colors[offset][2]);
    color *= color_scale;
    return color;
}

float average(std::vector<float> &vi)
{

    double sum = 0;

    // iterate over all elements
    for (float p : vi)
    {
        sum = sum + p;
    }

    return (sum / vi.size());
}

float Trtyolosort::calculateSpeed(int id, int fps, int start_frame, int end_frame)
{
    float duration = abs(start_frame - end_frame) / (float)fps;
    // float duration = abs(start_frame - end_frame) / 5;
    float distance_meter = Trtyolosort::line_distance;
    float velocity = (distance_meter / duration) * 3.6;
    return velocity;
}

cv::Mat Trtyolosort::showDetection(cv::Mat &img, std::vector<DetectBox> &boxes, int &fps, int &number_of_frame)
{
    cv::Mat temp = img.clone();
    // Coco Dataset
    //  string categories[] = {"bus", "big truck", "car", "small truck", "aeroplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "sofa", "pottedplant", "bed", "diningtable", "toilet", "tvmonitor", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"};
    // Custom
    string categories[] = {"Bus(L)",
                           "Bus(S)",
                           "Car",
                           "Truck(L)",
                           "Truck(M)",
                           "Truck(S)",
                           "Truck(XL)"};
    for (auto box : boxes)
    {
        // Get Rectangle Parameters
        cv::Point lt(box.x1, box.y1);
        cv::Point br(box.x2, box.y2);
        // Get Center Of Box
        float ix = (box.x1 + box.x2) / 2;
        float iy = (box.y1 + box.y2) / 2;
        // Create Rectangle
        cv::rectangle(temp, lt, br, idToColor((int)box.trackID), 1);
        // Box Label
        //  std::string lbl = cv::format("%d %s %.2f", (int)box.trackID, categories[(int)box.classID].data(), box.confidence);
        std::string lbl = cv::format("%s", categories[(int)box.classID].data());
        // Count Down Vehicle
        if ((iy < down_line_position_first) && (iy > middle_line_position_first) && (ix > 350))
        {
            velocity.erase((int)box.trackID);
            start_frames.insert({(int)box.trackID, number_of_frame});
        }
        else if ((iy > up_line_position_second) && (iy < middle_line_position_second) && (ix > 350))
        {
            if (std::find(std::begin(temp_down_list), std::end(temp_down_list), (int)box.trackID) == std::end(temp_down_list))
            {
                temp_down_list.push_back((int)box.trackID);
            }
        }
        else if ((iy < down_line_position_second) && (iy > middle_line_position_second) && (ix > 350))
        {
            if (std::find(std::begin(temp_down_list), std::end(temp_down_list), (int)box.trackID) != std::end(temp_down_list))
            {
                temp_down_list.erase(std::remove(temp_down_list.begin(), temp_down_list.end(), (int)box.trackID), temp_down_list.end());
                down_list[(int)box.classID] = down_list[(int)box.classID] + 1;
            }
            if (start_frames.find((int)box.trackID) != start_frames.end())
            {
                float speed = calculateSpeed((int)box.trackID, fps, start_frames[(int)box.trackID], number_of_frame);
                start_frames.erase((int)box.trackID);
                velocity.insert({(int)box.trackID, speed});
                if (speed > 10 && speed < 120)
                {
                    vector_moving_average_speed_down.push_back(speed);
                }
            }
        }
        // Count Up Vehicle
        if ((iy > up_line_position_second) && (iy < middle_line_position_second) && (ix < 350))
        {
            velocity.erase((int)box.trackID);
            start_frames.insert({(int)box.trackID, number_of_frame});
        }
        else if ((iy < down_line_position_first) && (iy > middle_line_position_first) && (ix < 350))
        {
            if (std::find(std::begin(temp_up_list), std::end(temp_up_list), (int)box.trackID) == std::end(temp_up_list))
            {
                temp_up_list.push_back((int)box.trackID);
            }
        }
        else if ((iy > up_line_position_first) && (iy < middle_line_position_first) && (ix < 350))
        {
            if (std::find(std::begin(temp_up_list), std::end(temp_up_list), (int)box.trackID) != std::end(temp_up_list))
            {
                temp_up_list.erase(std::remove(temp_up_list.begin(), temp_up_list.end(), (int)box.trackID), temp_up_list.end());
                up_list[(int)box.classID] = up_list[(int)box.classID] + 1;
            }
            if (start_frames.find((int)box.trackID) != start_frames.end())
            {
                float speed = calculateSpeed((int)box.trackID, fps, start_frames[(int)box.trackID], number_of_frame);
                start_frames.erase((int)box.trackID);
                velocity.insert({(int)box.trackID, speed});
                if (speed > 15 && speed < 120)
                {
                    vector_moving_average_speed_up.push_back(speed);
                }
            }
        }
        // Down Speed
        if ((iy > middle_line_position_second) && (ix > 350))
        {
            // Add Velocity to Box
            if (velocity.find((int)box.trackID) != velocity.end())
            {
                if (velocity[(int)box.trackID] > 10 && velocity[(int)box.trackID] < 120)
                {
                    std::string lbl_velocity = cv::format("%.1fkm/h", velocity[(int)box.trackID]);
                    cv::putText(temp, lbl_velocity, br, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar::all(255), 1, 8);
                }
            }
        }
        // Up Speed
        else if ((iy < middle_line_position_first) && (ix < 350))
        {
            if (velocity.find((int)box.trackID) != velocity.end())
            {
                if (velocity[(int)box.trackID] > 15 && velocity[(int)box.trackID] < 120)
                {
                    std::string lbl_velocity = cv::format("%.1fkm/h", velocity[(int)box.trackID]);
                    cv::putText(temp, lbl_velocity, br, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar::all(255), 1, 8);
                }
            }
        }
        // Speed Average
        if (vector_moving_average_speed_down.size() > 5)
        {
            filtered_moving_average_speed_down = average(vector_moving_average_speed_down);
            vector_moving_average_speed_down.erase(vector_moving_average_speed_down.begin());
        }
        if (vector_moving_average_speed_up.size() > 5)
        {
            filtered_moving_average_speed_up = average(vector_moving_average_speed_up);
            vector_moving_average_speed_up.erase(vector_moving_average_speed_up.begin());
        }
        // Add Object Class
        cv::putText(temp, lbl, lt, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, idToColor((int)box.trackID), 1, 8);
        // Add Circle in Center Box
        cv::circle(temp, cv::Point((int)ix, (int)iy), 2, cv::Scalar(0, 0, 255), -1);
    }
    if (filtered_moving_average_speed_down > 150)
    {
        filtered_moving_average_speed_down = 0;
    }
    // Draw Black Rectangle
    cv::Rect rectup1(0, 0, 800, 40);
    cv::rectangle(temp, rectup1, cv::Scalar(0, 0, 0), -1);
    cv::Rect rectup2(0, 40, 800, 30);
    cv::rectangle(temp, rectup2, cv::Scalar(0, 0, 0), -1);
    cv::Rect rectdown1(0, 500, 800, 500 );
    cv::rectangle(temp, rectdown1, cv::Scalar(0, 0, 0), -1);
    cv::Rect rectdown2(0, 570, 800, 570);
    cv::rectangle(temp, rectdown2, cv::Scalar(0, 0, 0), -1);
    // Draw Line
    cv::line(temp, cv::Point(Trtyolosort::x1_line_position_first, Trtyolosort::middle_line_position_first), cv::Point(Trtyolosort::x2_line_position_first, Trtyolosort::middle_line_position_first), cv::Scalar(0, 0, 255), 1);
    cv::line(temp, cv::Point(Trtyolosort::x1_line_position_second, Trtyolosort::middle_line_position_second), cv::Point(Trtyolosort::x2_line_position_second, Trtyolosort::middle_line_position_second), cv::Scalar(0, 0, 255), 1);
    
    //checkpoint
    //cv::Point p1(0,350 ), p2(350, 350);  //p1 -> (0 sampai 220)  p2 -> (640 panjang video, 220 titik bawahnya)
    //int thickness = 2;
    //line(temp, p1, p2, Scalar(255, 0, 0),thickness, LINE_8);	


    // Model Categories
    // 0 = Bus(L)
    // 1 = Bus(S)
    // 2 = Car
    // 3 = Truck(L)
    // 4 = Truck(M)
    // 5 = Truck(S)
    // 6 = Truck(XL)
    
    // Draw counting texts in the frame
    total_up = up_list[1] + up_list[2] + up_list[3] + up_list[4] + up_list[5] + up_list[6];
    total_down = down_list[1] + down_list[2] + down_list[3] + down_list[4] + down_list[5] + down_list[6];
    bus_up = up_list[1] + up_list[0];
    truck_up = up_list[3] + up_list[4] + up_list[5] + up_list[6];
    bus_down = down_list[1] + down_list[0];
    truck_down = down_list[3] + down_list[4] + down_list[5] + down_list[6];
    
    std::string lbl_fps = cv::format("FPS:%d ", fps);
    //car
    std::string lbl_car_up = cv::format("%d", up_list[2]);
    std::string lbl_car_down = cv::format("%d", down_list[2]);
    //bus
    std::string lbl_bus_up = cv::format("%d", bus_up );
    std::string lbl_bus_down = cv::format("%d", bus_down);
    //truck
    std::string lbl_truck_up = cv::format("%d", truck_up);
    std::string lbl_truck_down = cv::format("%d", truck_down);
    //total_kendaraan
    std::string lbl_total_up = cv::format("%d", total_up);
    std::string lbl_total_down = cv::format("%d", total_down);
    
    //std::string lbl_truck_s_down = cv::format("%d", down_list[5]);
    //std::string lbl_truck_m_down = cv::format("%d", down_list[4]);
    //std::string lbl_truck_l_down = cv::format("%d", down_list[3]);
    //std::string lbl_truck_xl_down = cv::format("%d", down_list[6]);
    //std::string lbl_speed_up = cv::format("%.1f", filtered_moving_average_speed_up);
    //std::string lbl_speed_down = cv::format("%.1f", filtered_moving_average_speed_down);

    
    cv::putText(temp, lbl_fps, cv::Point(590, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, TimeStr(), cv::Point(450, 50), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);             
    cv::putText(temp, "Total KR up: ", cv::Point(0, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_total_up, cv::Point(215, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    cv::putText(temp, "Car_up:", cv::Point(270, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_car_up, cv::Point(370, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    cv::putText(temp, "Bus_up:", cv::Point(0, 50), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_bus_up, cv::Point(215, 50), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    cv::putText(temp, "Truck_up:", cv::Point(270, 50), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_truck_up, cv::Point(400, 50), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    
    cv::putText(temp, "Total KR down: ", cv::Point(0, 530), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_total_down, cv::Point(215, 530), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    cv::putText(temp, "Car_down:", cv::Point(270, 530), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_car_down, cv::Point(400, 530), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    cv::putText(temp, "Bus_down:", cv::Point(0, 565), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_bus_down, cv::Point(215, 565), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    cv::putText(temp, "Truck_down:", cv::Point(270, 565), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    cv::putText(temp, lbl_truck_down, cv::Point(450, 565), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    
    //sricpt lama
    //cv::putText(temp, "Bus(L)", cv::Point(200, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_bus_l_up, cv::Point(250, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck", cv::Point(245, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_s_up, cv::Point(302, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck(M)", cv::Point(350, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_m_up, cv::Point(410, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck(L)", cv::Point(455, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_l_up, cv::Point(512, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck(XL)", cv::Point(545, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_xl_up, cv::Point(610, 25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255), 0.5);
   // cv::putText(temp, "Speed(km/h)", cv::Point(0, 40), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar(255, 255, 255), 1);
   // cv::putText(temp, lbl_speed_up, cv::Point(85, 40), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255), 0.5);

    //cv::putText(temp, "Car", cv::Point(0, 540), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_car_down, cv::Point(25, 540), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Bus(S)", cv::Point(70, 540), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_bus_s_down, cv::Point(112, 540), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Bus(L)", cv::Point(155, 540), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_bus_l_down, cv::Point(196, 540), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck(S)", cv::Point(245, 540), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_s_down, cv::Point(302, 540), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck(M)", cv::Point(350, 540), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_m_down, cv::Point(411, 540), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck(L)", cv::Point(455, 530), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_l_down, cv::Point(512, 530), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.4, cv::Scalar(255, 255, 255), 0.5);
    //cv::putText(temp, "Truck(XL)", cv::Point(545, 530), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    //cv::putText(temp, lbl_truck_xl_down, cv::Point(610, 530), cv::FONT_HERSHEY_SCRIPT_COMPLEX, 0.4, cv::Scalar(255, 255, 255), 0.5);
   // cv::putText(temp, "Speed(km/h)", cv::Point(0, 460), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5, cv::Scalar(255, 255, 255), 1);
   // cv::putText(temp, lbl_speed_down, cv::Point(85, 460), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255), 0.5);
    return temp;
}
cv::Mat Trtyolosort::TrtDetect(cv::Mat &frame, float &conf_thresh, std::vector<DetectBox> &det, int &number_of_frame)
{
    // yolo detect
    auto start = std::chrono::system_clock::now();
    cv::Mat temp = frame.clone();
    cv::Mat output_image;
    auto ret = yolov5_trt_detect(trt_engine, frame, conf_thresh, det);
    DS->sort(frame, det);
    auto end = std::chrono::system_clock::now();
    int delay_infer = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    int fps = 1000 / delay_infer;
    number_of_frame += 1;
    output_image = showDetection(temp, det, fps, number_of_frame);
    return output_image;
}
