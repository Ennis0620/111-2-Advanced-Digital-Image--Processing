// midterm.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

#include <iostream>
#include <opencv2/opencv.hpp>
#pragma comment(lib,"opencv_world460d.lib")
using namespace cv;
using namespace std;

#define BG_title "背景"

VideoCapture capture; //video

Mat bg;	//影片的背景

//直方圖範圍設定
int bins = 256;
int hist_size[] = { bins };
float range[] = { 0,256 };
const float* ranges[] = { range };

//Trackbar變數設定
Mat srcImage, dstImage;
int g_nTrackbarValue;
int g_nKernelValue;
const int g_nTrackerbarMaxValue = 9;

//Trackbar callback
void on_kernelTrackbar(int, void*) {
	g_nKernelValue = g_nTrackbarValue * 2 + 1;
	blur(srcImage, dstImage, Size(g_nKernelValue, g_nKernelValue));
	imshow("均值濾波", dstImage);
}

//負片
Mat linearTransform_negative(Mat src, float a=-1.2, float b=255) {
	if (src.empty()) {
		cout << "no img";
	}
	const int nRows = src.rows;
	const int nCols = src.cols;

	Mat result_img = cv::Mat::zeros(src.size(), src.type());

	for (int i = 0; i < nRows; i++) {
		for (int j = 0; j < nCols; j++) {
			for (int channel = 0; channel < 3; channel++) {
				result_img.at<Vec3b>(i, j)[channel] = saturate_cast<uchar>( a * (src.at<Vec3b>(i, j)[channel]) + b);
			}
		}
	}
	return result_img;
}

//直方圖
void histr(Mat src) {
	
	Mat hist;
	int channels[] = { 0 };
	//直方圖
	calcHist(&src, 1, channels, Mat(), hist, 1, hist_size, ranges, true, false);
	//plot histogram as an image
	double maxVal;
	minMaxLoc(hist, 0, &maxVal, 0, 0);

	int scale = 2;
	int hist_height = 256;

	Mat hist_img = Mat::zeros(hist_height, bins * scale, CV_8UC3);

	for (int i = 0; i < bins; i++) {
		float binVal = hist.at<float>(i);
		int intensity = cvRound(binVal / maxVal * hist_height);
		rectangle(hist_img, Point(i * scale, hist_height - 1),
			Point((i + 1) * scale - 1, hist_height - intensity), CV_RGB(255, 255, 255));
	}
	imshow("直方圖", hist_img);
}

//找到灰階圖的最大最小值 將範圍scale到0~255
Mat find_minMax_scale(Mat src) {

	Point minLoc, maxLoc;
	double minVal = 0;
	double maxVal = 0;
	minMaxLoc(src, &minVal, &maxVal, &minLoc, &maxLoc);
	src = src / maxVal * 255.0;
	return src;
}


//播放影片
void play(const char *filename) {
	
	//狀態計數器 當狀態計數器%2==0時 代表要切換成原來影像
	int status_counter=0;
	//紀錄按下哪顆按鍵
	int key_keeper = -1;

	Mat frame;
	VideoCapture capture(filename);
	if (!capture.isOpened()) {
		printf("open %s failed \n", filename, CAP_ANY);
		return;
	}
	long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);
	printf("total frames = %d \n", totalFrameNumber);
	char title[30];
	sprintf_s(title, "影片:%s", filename);
	
	while (1) {

		capture >> frame;				//利用>>取得frame

		if (frame.empty()) {
			printf("End of video\n");	
			break;
		}
		//按鍵輸入
		int key = waitKey(30);
		
		if (key == 'g' || key == 'i') {
			// 按下 'g' 或 'i'
			
			// 如果按鍵有改變，重置狀態計數器
			if (key_keeper != key) {
				status_counter = 1;				
				key_keeper = key;				// 更新key_keeper
			}
			// 如果按鍵沒改變 當該按鍵按下2次後會變回原始影像
			else if (key_keeper == key) {
				status_counter++;				// 狀態計數器累加
				if (status_counter % 2 == 0) {
					status_counter = 0;
					key_keeper = -1;			// 更新key_keeper
				}
			}
			
			cout << "key_keeper按鍵: " << char(key_keeper) << endl;
		}

		// 根據目前key_keeper狀態來區分要做什麼影像處理
		Mat processed_frame;
		if (key_keeper == -1) {
			processed_frame = frame;										//原始影像
		}
		else if (key_keeper == 'g') {
			cvtColor(frame, processed_frame, COLOR_BGR2GRAY);				//對原始影像做灰階
		}
		else if (key_keeper == 'i') {
			processed_frame = linearTransform_negative(frame, -1.2, 255);	//對原始影像做負片
		}

		// 顯示影像
		imshow(title, processed_frame);
		
		if (key == 'q') {  // 小寫q ASCII:133 退出
			cout << "按鍵: " << char(key) << " 退出" << endl;
			break;
		}
	}
}

//開鏡頭
void cam_play() {
	//狀態計數器 當狀態計數器%2==0時 代表要切換成原來影像
	int status_counter = 0;
	//紀錄按下哪顆按鍵
	int key_keeper = -1;
	//init capture bg
	bool bg_flag = true;
	//init histr
	bool histr_flag = false;

	//讀webcam
	VideoCapture capture(0, CAP_DSHOW);	
	if (!capture.isOpened()) {
		puts("webcam not found!");
		return;
	}
	char title[30];
	sprintf_s(title, "webcam:%d", 0);

	
	
	//frame:webcam擷取
	//bgframe:按鍵c擷取背景
	//gray_frame:webcam擷取後轉灰階
	//gray_bgframe:按鍵c擷取背景後轉灰階
	//histr_frame:顯示直方圖
	Mat frame, bgframe, gray_frame, gray_bgframe, histr_frame;
	
	//當webcam有持續擷取到影像
	while (capture.read(frame)) {

		//按鍵輸入
		int key = waitKey(30);
		
		//一開始先抓一張當作預設背景
		if (bg_flag) {
			bgframe = frame;
			cvtColor(bgframe, gray_bgframe, COLOR_BGR2GRAY);				//背景做灰階
			bg_flag = false;
		}
		
		//按下c抓取背景影像
		if (key == 'c') {
			bgframe = frame;
			imshow("capture bg", bgframe);
			cvtColor(bgframe, gray_bgframe, COLOR_BGR2GRAY);				//背景做灰階
			//imshow("capture gray_bgframe", gray_bgframe);
		}
		
		//只偵測會對影像操作的按鍵
		if (key == 'b' || key == 't') {
			// 按下 'b' 或 't'

			// 如果按鍵有改變，重置狀態計數器
			if (key_keeper != key) {
				status_counter = 1;
				key_keeper = key;				// 更新key_keeper
			}
			// 如果按鍵沒改變 當該按鍵按下2次後會變回原始影像
			else if (key_keeper == key) {
				status_counter++;				// 狀態計數器累加
				if (status_counter % 2 == 0) {
					status_counter = 0;
					key_keeper = -1;			// 更新key_keeper
				}
			}

			cout << "key_keeper按鍵: " << char(key_keeper) << endl;
		}

		// 根據目前key_keeper狀態來區分要做什麼影像處理
		Mat processed_frame;
		if (key_keeper == -1) {
			processed_frame = frame;//原始影像
		}
		//灰階 + background subtraction
		else if (key_keeper == 'b') {
			cvtColor(frame, gray_frame, COLOR_BGR2GRAY);					
			absdiff(gray_frame, gray_bgframe, processed_frame);
			
			Point minLoc, maxLoc;
			double minVal = 0;
			double maxVal = 0;
			minMaxLoc(processed_frame, &minVal, &maxVal, &minLoc, &maxLoc);
			processed_frame = processed_frame / maxVal * 255.0;
			
		}
		//灰階 + background subtraction + binary thresholding
		else if (key_keeper == 't') {

			cvtColor(frame, gray_frame, COLOR_BGR2GRAY);						
			absdiff(gray_frame, gray_bgframe, processed_frame);					

			Point minLoc, maxLoc;
			double minVal = 0;
			double maxVal = 0;
			minMaxLoc(processed_frame, &minVal, &maxVal, &minLoc, &maxLoc);
			processed_frame = processed_frame / maxVal * 255.0;
			threshold(processed_frame, processed_frame, 127, 255, THRESH_BINARY); 
		}
		
		//當按下h histr_flag會變成相反的boolean數值
		if (key == 'h') {
			histr_flag = !histr_flag;
		}

		//當histr_flag == true顯示直方圖
		if (histr_flag == true) {
			//如果目前是
			if (key_keeper == -1) {
				cvtColor(processed_frame, histr_frame, COLOR_BGR2GRAY);
			}
			else {
				histr_frame = processed_frame;
			}
			histr(histr_frame);
		}

		// 顯示影像
		imshow(title, processed_frame);

		if (key == 'q') {  // 小寫q ASCII:133 退出
			cout << "按鍵: " << char(key) << " 退出" << endl;
			break;
		}

	}
		
}

//整合webcam和播放影片
//當filename==webcam時 開鏡頭
void merge_play(const char* filename) {

	cout << "檔名:" << filename << endl;
	char title[30];

	//狀態計數器 當狀態計數器%2==0時 代表要切換成原來影像
	int status_counter = 0;
	//紀錄按下哪顆按鍵
	int key_keeper = -1;
	//init capture bg
	bool bg_flag = true;
	//init histr
	bool histr_flag = false;
	//init Trackebar
	bool bar_flag = false;

	//frame:webcam擷取
	//bgframe:按鍵c擷取背景
	//gray_frame:webcam擷取後轉灰階
	//gray_bgframe:按鍵c擷取背景後轉灰階
	//histr_frame:顯示直方圖
	Mat frame, bgframe, gray_frame, gray_bgframe, histr_frame;


	//讀webcam
	if (filename == "webcam") {
		capture.open(0, CAP_DSHOW);
		if (!capture.isOpened()) {
			puts("webcam not found!");
			return;
		}
		sprintf_s(title, "webcam");
	}
	//讀影片
	else{
		capture.open(filename);
		if (!capture.isOpened()) {
			printf("open %s failed \n", filename, CAP_ANY);
			return;
		}
		sprintf_s(title, "影片:%s", filename);
		//影片的預設背景
		bgframe = bg;
		cvtColor(bgframe, gray_bgframe, COLOR_BGR2GRAY);
	}
	namedWindow(title, WINDOW_AUTOSIZE);
	//當有持續擷取到影像
	while (capture.read(frame)) {
		capture >> frame;				//利用>>取得frame
		
		if (frame.empty()) {
			printf("End of video\n");
			break;
		}
		//按鍵輸入
		int key = waitKey(30);

		if (filename == "webcam") {
			//一開始先抓一張當作預設背景
			if (bg_flag) {
				bgframe = frame;
				cvtColor(bgframe, gray_bgframe, COLOR_BGR2GRAY);				//背景做灰階
				bg_flag = false;
			}
			//按下c抓取背景影像
			if (key == 'c') {
				bgframe = frame;
				imshow("capture bg", bgframe);
				cvtColor(bgframe, gray_bgframe, COLOR_BGR2GRAY);				//背景做灰階
				//imshow("capture gray_bgframe", gray_bgframe);
			}
		}

		if (key == 'g' || key == 'i' || key == 'b' || key == 't' ) {
			// 按下 'g' 'i' 'b' 't' 

			// 如果按鍵有改變，重置狀態計數器
			if (key_keeper != key) {
				status_counter = 1;
				key_keeper = key;				// 更新key_keeper
			}
			// 如果按鍵沒改變 當該按鍵按下2次後會變回原始影像
			else if (key_keeper == key) {
				status_counter++;				// 狀態計數器累加
				if (status_counter % 2 == 0) {
					status_counter = 0;
					key_keeper = -1;			// 更新key_keeper
				}
			}
			
			cout << "key_keeper按鍵: " << char(key_keeper) << endl;
		}

		// 根據目前key_keeper狀態來區分要做什麼影像處理
		Mat processed_frame;
		

		//原始影像
		if (key_keeper == -1) {
			processed_frame = frame;										
		}
		//對原始影像做灰階
		else if (key_keeper == 'g') {
			cvtColor(frame, processed_frame, COLOR_BGR2GRAY);				
		}
		//對原始影像做負片
		else if (key_keeper == 'i') {
			processed_frame = linearTransform_negative(frame, -1.2, 255);	
		}
		//灰階 + background subtraction
		else if (key_keeper == 'b') {
			cvtColor(frame, gray_frame, COLOR_BGR2GRAY);
			absdiff(gray_frame, gray_bgframe, processed_frame);
			processed_frame = find_minMax_scale(processed_frame);
		}
		//灰階 + background subtraction + binary thresholding
		else if (key_keeper == 't') {

			cvtColor(frame, gray_frame, COLOR_BGR2GRAY);
			absdiff(gray_frame, gray_bgframe, processed_frame);
			processed_frame = find_minMax_scale(processed_frame);
			threshold(processed_frame, processed_frame, 35, 255, THRESH_BINARY);
		}
		
		
		if (key == 'a') {
			bar_flag = !bar_flag;
		}
		//當bar_flag == true 使用濾波blur
		if (bar_flag == true) {

			srcImage = processed_frame;
			dstImage = frame;
			on_kernelTrackbar(g_nKernelValue, 0);
			
		}
		

		//當按下h histr_flag會變成相反的boolean數值
		if (key == 'h') {
			histr_flag = !histr_flag;
		}
		//當histr_flag == true顯示直方圖
		if (histr_flag == true) {
			//如果目前是
			if (key_keeper == -1) {
				cvtColor(processed_frame, histr_frame, COLOR_BGR2GRAY);
			}
			else {
				histr_frame = processed_frame;
			}
			histr(histr_frame);
		}

		// 顯示影像
		imshow(title, processed_frame);

		if (key == 'q') {  // 小寫q ASCII:133 退出
			cout << "按鍵: " << char(key) << " 退出" << endl;
			break;
		}
	}

}

int main()
{	
	
	bg = imread("flymanBG.jpg");
	//imshow(BG_title,bg);
	//play("flyman512x512.avi");
	//cam_play();
	

	char kernalName[20];
	sprintf_s(kernalName, "Max:%d", g_nTrackerbarMaxValue);
	//g_nTrackbarValue = 1;
	namedWindow("均值濾波", WINDOW_AUTOSIZE);
	createTrackbar(kernalName, "均值濾波", &g_nTrackbarValue, g_nTrackerbarMaxValue, on_kernelTrackbar);

	//merge_play("flyman512x512.avi");
	merge_play("webcam");
	
	return 0;
}



// 執行程式: Ctrl + F5 或 [偵錯] > [啟動但不偵錯] 功能表
// 偵錯程式: F5 或 [偵錯] > [啟動偵錯] 功能表

// 開始使用的提示: 
//   1. 使用 [方案總管] 視窗，新增/管理檔案
//   2. 使用 [Team Explorer] 視窗，連線到原始檔控制
//   3. 使用 [輸出] 視窗，參閱組建輸出與其他訊息
//   4. 使用 [錯誤清單] 視窗，檢視錯誤
//   5. 前往 [專案] > [新增項目]，建立新的程式碼檔案，或是前往 [專案] > [新增現有項目]，將現有程式碼檔案新增至專案
//   6. 之後要再次開啟此專案時，請前往 [檔案] > [開啟] > [專案]，然後選取 .sln 檔案
