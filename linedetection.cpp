#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main(){

	VideoCapture cap(0);
	
	if (!cap.isOpened()){
		cout << "camera not detected" << endl;
		return -1;
	}
	while(true){
		Mat frame;
		cap >> frame;

		Mat gray, blurred, thresh, edges, mask;
		cvtColor(frame, gray, COLOR_BGR2GRAY);
		GaussianBlur(gray, blurred, Size(5,5), 0);
		// threshold(blurred, thresh, 100, 255, THRESH_BINARY_INV);

		// vector<Vec4i> lines;
		// HoughLinesP(thresh, lines, 1, CV_PI/180, 30, 50,5);

		// for (size_t i=0; i < lines.size(); i++){
		// 	Vec4i l = lines[i];
		// 	line(frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255,0), 2, LINE_AA);			
		// }

		Canny(gray, edges, 50, 150);

		bitwise_not(edges, mask);


		imshow("mask", mask);

		if (waitKey(1) == 'q'){
			break;
		}
	}
	cap.release();
	destroyAllWindows();
	cout << "EOP" << endl;
	return 0;
}