#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <map>
#include <list>
#include <thread>
#include <iostream>
#include <fstream>
#include <mutex>

using namespace cv;
using namespace std;

struct elipParam {
	float a;
	float b;
	float cx;
	float cy;
};


//结构体保存一幅图像中保存的5个圆环的信息
struct Circle_msg
{
	RotatedRect box_standard;
	RotatedRect box1;
	RotatedRect box2;
	RotatedRect box3;
	RotatedRect box4;
	RotatedRect box5;
	bool flag;
};

struct circlParam {
	int erodeSize;
	int dilateSize;
	int erodeType;
	int dilateType;
	Size blurSz;

	double cannythreshold1;
	double cannythreshold2;
	int cannyapertureSz;
	int findCounterMode;
	int findCounterMethod;

	int tooLessPixels;
	int mianjiThreshold;
	float HeigWidthRio;

	float boxCashHeigWidthRio;

};

struct defctParam {

	float imgshiftxThrd;
	float imgshiftyThrd;

	float diffImgGain;

	int mediaBlurSz;

	int roiXmarg;
	int roiYmarg;
	int roiHmarg;
	int roiWmarg;

	int erodeSize;

	int thresholdMin;
	int thresholdMax;
	int thresholdType;

	int secdMediaBlurSz;

	int findCounterMode;
	int findCounterMethod;

	float ratioThrdMax;
	float ratioThrdMin;

	float areaThrdMax;
	float areaThrdMin;

	int gunRectOffsetMarg;
	int gunRectSzMarg;

	float meandiffThrd;

	float elipThrd;

	float posShiftThrd;
};

class logger {
public:
	logger();
	void doLog(string msg);
	void setLogPath(string camIp);
	string getLogPath();
	int getImgId() {
		return camImgId;
	}
	void setImgId() {
		camImgId++;
	}

private:
	string logPath;
	string logFile;
	int camImgId;
};


class cameraDevice {
public:
	cameraDevice();  // in this construct, set default param
	void calcGun(Mat src, char* camIp);
	void Circle_recognize(Mat img, Circle_msg& box_result);
	void drawGunPos(Mat* src, int* cnt, int* goal);
	void setCirclParam(); // read from txt file
	struct circlParam getCirclParam();
	void setDefctParam();
	struct defctParam getDefctParam();
	pair<int, int> getScore();
	vector<Point> getCurrentPos();
	void setGlobalParam();
	void setLogger(string camIp);
	void updateImgAndGunPos(Mat& firstImg, vector<Point2f>& allguns, Circle_msg& firstImgMsg, float imgshiftx, float imgshifty, Circle_msg& box_msg3);
	Mat getDiffImg(Mat& thirdImg, Mat& firstImg, Circle_msg& box_msg3, int offsetx, int offsety, int roiw, int roih);
	void getBullet(Mat& thirdImg, vector<Rect> diffRoiFiltered, int offsetx, int offsety, map<int, struct elipParam>& fiveElip);
	void judgePos(Mat& roi, Point& gunPos, map<int, struct elipParam>& fiveElip);
	void calcElip(map<int, struct elipParam>& fiveElip, Circle_msg& box_msg3);
	Mat getRoiEdge(Mat& thirdImg, Circle_msg& box_msg3, int offsetx, int offsety, int roiw, int roih, int mode);
	vector<Rect> filterDiffRoi(Mat& diffRoi, Mat& thirdImg, Mat& firstImg, Circle_msg& box_msg3,
								int offsetx, int offsety, int roiw, int roih);

private:
	Mat firstImgT;
	Mat firstImgRoiEdgMapCannyT;
	Mat firstImgRoiEdgMapSobelT;
	pair<int, int> score;
	vector<Point2f> allguns;
	Circle_msg firstImgMsgT;
	vector<Point> currentPos;

	struct circlParam circlePrm;
	struct defctParam defctPrm;
	bool logEnable;
	int LogIntermedia;
	logger  camLogger;
};


class fatherPapa {
public:
	static fatherPapa* getInstance();
	void setCamDev(string camIp);
	cameraDevice* getCamDev(string camIp);
	~fatherPapa()
	{
		map<string, cameraDevice*>::iterator ite;
		ite = allDev.begin();
		while (ite != allDev.end())
		{
			delete ite->second;
			ite++;
		}
	}

private:
	map<string, cameraDevice*> allDev;
	//static fatherPapa* instance;
	std::mutex m_mutex;
};

void uploadRslt(Mat* src, char* camIp, int* goal, int* cnt);




