#include "qr.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <string>
using namespace cv;
using namespace std;

int det;

int qr_detector(int* arg){
    QRCodeDetector qrDecoder = QRCodeDetector();


    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error opening video stream" << endl;
        return -1;
    }
    while(1){
        Mat frame;
            cap >> frame;
            if (frame.empty()) {
                cerr << "Error capturing frame" << endl;
                break;
            }

            string data = qrDecoder.detectAndDecode(frame);
            if (!data.empty()) {
                cout << "QR Code detected: " << data << endl;
                // cap.release();
                // return stoi(data);
                det = stoi(data);
            }
            *arg = det;
            if(cv::waitKey(10)=='q'){
                // break;
            }
    }
    cap.release();
    return -1;
}

