#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <map>
#include <list>

using namespace cv;
using namespace std;


//结构体保存一幅图像中保存的5个圆环的信息
struct Circle_msg
{
	RotatedRect box1;
	RotatedRect box2;
	RotatedRect box3;
	RotatedRect box4;
	RotatedRect box5;
};


double Cal_Distance(double x1, double y1, double x2, double y2);
RotatedRect FusionEllipse(RotatedRect box1, RotatedRect box2);
void CircleRecgnize(Mat img, Circle_msg *box_all);

float calculElip(float a, float b, float cx, float cy, float x0, float y0);
void calcGun(Mat src, char* camIp);
void uploadRslt(Mat src, std::string camIp, int* goal, int* cnt);
