#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <string.h>
using namespace cv;
using namespace std;

int main() {
    QRCodeDetector qrDecoder = QRCodeDetector();
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
            cout << "QR Code detected: " << stoi(data) << endl;
            //cout << "QR Code detected: " << data << endl;
            vector<Point> points;
            qrDecoder.detect(frame, points);
            if (points.size() == 4) {
                for (int i = 0; i < 4; i++) {
                    line(frame, points[i], points[(i+1)%4], Scalar(0, 255, 0), 3);
                }
            }
        }

        imshow("QR Code Detection", frame);

        if (waitKey(30) == 'q') {
            break;
        }
    }

    cap.release();
    destroyAllWindows();
    return 0;
}

