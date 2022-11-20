#include "manager.hpp"
// Add Web Dashboard
#include "MJPEGWriter.h"
#include "Scheduler.h"
#include "globals.h"
//#include <amqp.h>
//#include <amqp_tcp_socket.h>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <typeinfo>
#include <fstream>
#define CST (+7)
#define timeday (+1)
#include <iomanip>

using json = nlohmann::json;
using namespace cv;

// RTSP receive buffer list
cv::Mat frame;
cv::Mat output_frame;
int frame_width;
int frame_height;
bool isRun;
json j;
std::string jsonmessage;
unsigned int max_n_threads = 12;
int total_bus_up,total_truck_up,total_kr_up;
int total_bus_down,total_truck_down,total_kr_down;

Bosma::Scheduler s(max_n_threads);
// Counter List Params
int up_list[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int down_list[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// Moving Average Speed
float filtered_moving_average_speed_up = 0;
float filtered_moving_average_speed_down = 0;
int temp_up_list[8];
int result_up[8];
int temp_down_list[8];
int result_down[8];


//std::string getTimeStr()
//{
//    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

//    std::string s(30, '\0');
//    std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
//    s.erase(std::find(s.begin(), s.end(), '\0'), s.end());
//    return s;
//}

std::string getTimeStr()
{
    time_t current_time;
    struct tm* ptime;
    time(&current_time);
    int ptime_year;
  
    ptime = gmtime(&current_time); 
    ptime_year = 1900+(ptime->tm_year);

    std::string lbl_hour = cv::format("%2d",(ptime->tm_hour + CST)%24); 
    std::string lbl_min = cv::format("%2d",(ptime->tm_min));
    std::string lbl_sec = cv::format("%2d",(ptime->tm_sec));
   // cout << ptime->tm_hour << endl;
   
   
    std::string lbl_mday = cv::format("%2d",(ptime->tm_mday));
    std::string lbl_mon = cv::format("%2d",(ptime->tm_mon));
    std::string lbl_year = cv::format("%d",ptime_year);
 
    
    std::string time_wib= lbl_hour+":"+lbl_min+":"+lbl_sec;
    //+"-"+lbl_hour+":"+lbl_min+":"+lbl_sec;
    
    
   return time_wib;
}

string configtxt()
{

    string line;
    string id_loc;
    ifstream filein("./config_ai.txt");
    if (filein.is_open())
    {
      getline(filein,line);     
      filein.close();
    }
    else
    {
        cout<<"file not open"<<endl;
    }     
    return line;
}


void subtract(int *x, int *y, int *result, int size)
{
    for (int i = 0; i < size; ++i)
        result[i] = y[i] - x[i];
}
// thread function for video read
void readFrame(void)
{    
    string line;
    string streamUrl;
    
    line=configtxt();
    json stream1=json::parse(line);
    streamUrl = stream1.value("streamUrl", "oops");
    
    cout<<stream1<<endl;
    //string streamUrl = "rtsp://admin:hik12345@103.3.77.163:5030/Streaming/Channels/102";
    // string streamUrl = "rtsp:://103.3.77.163:4200/axis-cgi/mjpg/video.cgi?date=1&clock=1&resolution=640x480";
     VideoCapture capture(streamUrl);
    // capture.set(cv::CAP_PROP_FPS,15.0);
    // frame = capture.open(source_name);
    if (!capture.isOpened())
    {
        std::cout << "can not open" << std::endl;
        
    }
    frame_width=capture.get(cv::CAP_PROP_FRAME_WIDTH);
    frame_height=capture.get(cv::CAP_PROP_FRAME_HEIGHT); 
    while (isRun)
    {
        capture >> frame;
    }
    capture.release();
}

// Thread function for video write
void writeFrame(void)
{
    // Time sleep 5 seconds
    usleep(5000000);
    VideoWriter writer;
    // Save Stream to Video File
    std::string output_name = "/yolov5-deepsort-tensorrt/result.avi";
    writer.open(output_name, VideoWriter::fourcc('M', 'J', 'P', 'G'), 10, Size(frame_width, frame_height), true);
    int i = 0;
    while (isRun)
    {
        writer.write(output_frame);
        usleep(50000);
    }
    writer.release();
}



void resetminutes(int up_list_message[], int down_list_message[], float up_speed_message, float down_speed_message)
{
    // create an empty structure (null)
    json j;
    string line;
    string id_loc;
    
    line=configtxt();
    json stream1=json::parse(line);
    id_loc = stream1.value("id_loc", "oops");
    
    cout<<stream1<<endl;
    
    //jumlah kendaraan up
    total_bus_up = up_list_message[1]+up_list_message[0];
    total_truck_up= up_list_message[6]+up_list_message[5]+up_list_message[4]+up_list_message[3];
    total_kr_up= up_list_message[2]+ total_bus_up + total_truck_up;
    //jumlah kendaraan down
    total_bus_down = down_list_message[1]+down_list_message[0];
    total_truck_down= down_list_message[6]+down_list_message[5]+down_list_message[4]+down_list_message[3];
    total_kr_down = up_list_message[2]+ total_bus_down + total_truck_down;
    
    //json
    j["id"] = id_loc;
    j["total_kr_up"]= total_kr_up;
    j["total_kr_down"]= total_kr_down;
    j["time"] = getTimeStr();
    j["car_up"] = up_list_message[2];
    j["Bus_up"] = total_bus_up;
    j["truck_up"] = total_truck_up;
    j["Bus_down"] = total_bus_down;
    j["truck_down"] = total_truck_down;

    const auto s = j.dump(); // serialize to std::string
    jsonmessage = s;
    cout << jsonmessage <<endl;
    //cout<<typeid(jsonmessage).name()<<endl;
    string vehiclesList[1] = {jsonmessage};
    

    //cek direktori
    //cout<<get_current_dir_name()<<endl;
    //write to txt.
    int arraySize = *(&vehiclesList + 1) - vehiclesList;
    try {
    cout << "\nWriting  array contents to file...";
    //open file for writing
    ofstream fw("./temp.txt", std::ofstream::out);
    cout << "\nDone!"<<endl;
    //check if file was successfully opened for writing
    if (fw.is_open())
    {
      for (int i = 0; i < arraySize; i++) {
        fw << vehiclesList[i] << "\n";
      }
      fw.close();
    }
    else cout << "Problem with opening file";
  }
   catch (const char* msg) {
    cerr << msg << endl;
  }    
}

// Production Main FUnction
int main()
{
    // Publis Web Dashboard to port 85
    MJPEGWriter test(85);
    // Deepsort Engine Location
    char *sort_engine = "/yolov5-deepsort-tensorrt/resources/deepsort.engine";
    // YoloV5 Engine Location
    char *yolo_engine = "/yolov5-deepsort-tensorrt/resources/yolov5s-080122.engine";
    // Confidence Level YoloV5
    float conf_thre = 0.5;

    isRun = true;

   // Cron Calculate Counter and Send to AMQP
     s.cron("* * * * *", []()
          {
     subtract(temp_up_list, up_list, result_up, 8);
     subtract(temp_down_list, down_list, result_down, 8);
     resetminutes(result_up, result_down,filtered_moving_average_speed_up,filtered_moving_average_speed_down);
     memset(up_list,0,sizeof(up_list));
     memset(down_list,0,sizeof(down_list));	
     std::copy(up_list, up_list + 8, temp_up_list);
     std::copy(down_list, down_list + 8, temp_down_list);
 });
  

    // Cron Reset Parameters Every Day
    //s.cron("0 17 * * *", []()
    //       {
    //    memset(up_list, 0, sizeof(up_list));
    //    memset(down_list, 0, sizeof(down_list));
    //    std::copy(up_list, up_list + 8, temp_up_list);
    //    std::copy(down_list, down_list + 8, temp_down_list); });

    // Copy Array
    std::copy(up_list, up_list + 8, temp_up_list);
    std::copy(down_list, down_list + 8, temp_down_list);

    // Thread
    thread t1(readFrame);

    // Comment Because using Cron
    // thread t2(sendAMQP);

    // Uncomment if want save to file
    // thread t3(writeFrame);

    Trtyolosort yosort(yolo_engine, sort_engine);
    std::vector<DetectBox> det;
    auto start_draw_time = std::chrono::system_clock::now();
    clock_t start_draw, end_draw;
    start_draw = clock();
    int number_of_frame = 0;

    // Running WebService
    test.start();

    // Main Function for Object Detection
    while (isRun)
    {
        auto start = std::chrono::system_clock::now();
        output_frame = yosort.TrtDetect(frame, conf_thre, det, number_of_frame);
        auto end = std::chrono::system_clock::now();
        int delay_infer = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        // std::cout << "FPS:" << 1000 / delay_infer << std::endl;
        test.write(output_frame);
        char c = (char)waitKey(1);
        if (c == 27)
        {
            isRun = false;
        }
    }

    // Stop Webservice
    test.stop();
    isRun = false;
    t1.join();
    // t2.join();
    // t3.join();
    return 0;
}