#include "qr.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <string>
using namespace cv;
using namespace std;

int qr_detector(){
    QRCodeDetector qrDecoder = QRCodeDetector::QRCodeDetector();

    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error opening video stream" << endl;
        return -1;
    }

    Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) {
            cerr << "Error capturing frame" << endl;
            break;
        }

        string data = qrDecoder.detectAndDecode(frame);
        if (!data.empty()) {
            cout << "QR Code detected: " << data << endl;
            return stoi(data);
        }
    }
    cap.release();
}
