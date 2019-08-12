#include "stdafx.h"
#include "getgun.h"
#include <io.h>
#include <direct.h>
#include <stdlib.h>


fatherPapa instance;

void initCirclMsg(Circle_msg& cirMsg)
{
	cirMsg.box1.center.x = 0;
	cirMsg.box1.center.y = 0;
	cirMsg.box2.center.x = 0;
	cirMsg.box2.center.y = 0;
	cirMsg.box3.center.x = 0;
	cirMsg.box3.center.y = 0;
	cirMsg.box4.center.x = 0;
	cirMsg.box4.center.y = 0;
	cirMsg.box5.center.x = 0;
	cirMsg.box5.center.y = 0;
	cirMsg.box_standard.center.x = 0;
	cirMsg.box_standard.center.y = 0;
	cirMsg.flag = 0;
}



/*
计算两点之间的距离
*/
double dis_point(Point a, Point b)
{
	double dis = sqrt(((double)a.x - (double)b.x)*((double)a.x - (double)b.x) + ((double)a.y - (double)b.y)*((double)a.y - (double)b.y));
	return dis;
}

Mat getRoiEdge1(Mat& thirdImg)
{
	Mat singleImgRoi = thirdImg.clone();
	int erodeSize = 3;
	int dilateSize = 3;
	Mat elementForDilate = getStructuringElement(MORPH_RECT, Size(dilateSize, dilateSize), Point(0, 0));
	blur(singleImgRoi, singleImgRoi, Size(3, 3));//均值滤波
	dilate(singleImgRoi, singleImgRoi, elementForDilate);//膨胀

	Canny(singleImgRoi, singleImgRoi, 20, 80, 3, false);//边缘检测
	return singleImgRoi;
}

/*椭圆融合函数，当两个圆环的半径接近且圆心相近时，将其融合为一个圆环
*
*输入：需要融合的两个椭圆对应的box1、box2
*输出：融合后的椭圆box */
RotatedRect FusionEllipse(RotatedRect box1, RotatedRect box2)
{
	RotatedRect box;
	box.center.x = (box1.center.x + box2.center.x) / 2;
	box.center.y = (box1.center.y + box2.center.y) / 2;
	box.size.width = (box1.size.width + box2.size.width) / 2;
	box.size.height = (box1.size.height + box2.size.height) / 2;
	box.angle = (box1.angle + box2.angle) / 2;
	return box;
}

/*

计算向量ab的方向与x轴正方向的角度
*/
double cal_anger(Point a, Point b)
{
	double xa = a.x;
	double ya = a.y;
	double xb = b.x;
	double yb = b.y;
	double result_anger;
	if (fabs(xa - xb)<0.00001)
		if (ya <= yb)
		{
			result_anger = (3.14159260 / 2);
			return result_anger;
		}
		else
		{
			result_anger = (3.14159260 / 2) * 3;
			return result_anger;
		}
	double tan_ab = (ya - yb) / (xa - xb);

	if (tan_ab>0)
	{
		result_anger = atan(tan_ab);
		if (yb<ya)
			result_anger = result_anger + 3.1415926;
	}
	else if (tan_ab<0)
	{
		result_anger = atan(tan_ab);
		if (yb>ya)
			result_anger = result_anger + 3.1415926;
		else
			result_anger = result_anger + 6.2831852;
	}
	else if (tan_ab == 0)
	{
		if (xb<xa)
		{
			result_anger = 3.1415926;
		}
		else
			result_anger = 0;
	}
	return result_anger;

}

/*
double Circle_percent(vector<Point> Contours_1)
统计轮廓Contours1中的轮廓点在椭圆圆环中方位覆盖情况，用于判断是否为椭圆弧
*/
double Circle_percent(cv::vector<Point> Contours_1)
{
	int num_location[100];
	int num_ex = 0;
	for (int i = 0; i<100; i++)
		num_location[i] = 0;
	RotatedRect box;
	box = fitEllipse(Mat(Contours_1));
	Point a = box.center;
	for (int i = 0; i<(int)Contours_1.size(); i++)
	{
		Point b = Contours_1[i];
		double anger_i = cal_anger(a, b);
		int locat = (int)(anger_i / (0.1256637 / 2));
		if (locat>100)
			continue;
		else
		{
			num_location[locat] = 1;
		}
	}
	for (int i = 0; i<100; i++)
	{
		if (num_location[i] == 1)
			num_ex++;
	}
	double circle_percent = ((double)num_ex) / 100;
	return circle_percent;
}

/*
函数：vector<vector<Point>> contours_numfilter(vector<vector<Point>> contours_src,int _tooLessPixels)
功能：对轮廓集合进行过滤，删除集合点个数小于_tooLessPixels的轮廓；

*/
void contours_numfilter(cv::vector< cv::vector<Point> > contours_src, int _tooLessPixels, cv::vector< cv::vector<Point> >& contours_des)
{
	for (int i = 0; i<(int)contours_src.size(); i++)
	{
		if ((int)contours_src[i].size()>(_tooLessPixels))
			contours_des.push_back(contours_src[i]);

	}

}

/*
函数：vector<Point> circle_filtersingle(vector<Point> contours_src)
对轮廓进行椭圆过滤，输出新的轮廓
*/
void circle_filtersingle(cv::vector<Point> contours_src, cv::vector<Point>& contours_des)
{
	RotatedRect box;
	box = fitEllipse(Mat(contours_src));
	double Long_r = (double)box.size.height;
	double Short_r = (double)box.size.width;
	double linshi_r;
	if (Long_r<Short_r)
	{
		linshi_r = Long_r;
		Long_r = Short_r;
		Short_r = linshi_r;
	}
	//vector<Point> contours_des;
	for (int i = 0; i<(int)contours_src.size(); i++)
	{
		double dis = dis_point(box.center, contours_src[i]);
		if (((dis + 3)>Short_r / 2) && ((dis - 3)<Long_r / 2))
			contours_des.push_back(contours_src[i]);
	}
	return;
}

/*
函数名：
vector <vector<Point>> circle_filter(vector<vector<Point
功能：对轮廓向量进行椭圆过滤
*/
void circle_filter(cv::vector<cv::vector<Point>> &contoursall_src, cv::vector< cv::vector<Point> >& contours_result)
{
	cv::vector<Point> contour_linshi;
	//vector< vector<Point> > contours_result;
	for (int i = 0; i<(int)contoursall_src.size(); i++)
	{
		contour_linshi.clear();
		circle_filtersingle(contoursall_src[i], contour_linshi);
		contours_result.push_back(contour_linshi);
	}
	return;
}



/*
函数名：
void Cilrcle_recognize(Mat img,Circle_msg *box_all)
功能：
识别图像中的圆环信息，将其存入结构体*box_all中
算法步骤：
图像预处理->提取轮廓->删减轮廓->遍历轮廓判断是否为圆环->生成box向量->box向量融合排序->计算第四第五个圆环->输出结构体
20190518，乔洋
---------------------------------------------------------
*/

void cameraDevice::Circle_recognize(Mat img, Circle_msg& box_result)
{
	cv::vector< cv::vector<Point> > filterContours4;
	cv::vector< cv::vector<Point> > filterContours3;
	cv::vector< cv::vector<Point> > filterContours2;
	cv::vector< cv::vector<Point> > filterContours1;
	cv::vector< cv::vector<Point> > filterContours;
	cv::vector< cv::vector<Point> > contours;
	cv::vector< cv::Vec4i > hierarchy;
	//得到轮廓图片
	Mat src_gray = getRoiEdge1(img);
	//得到轮廓向量
	findContours(src_gray, contours, hierarchy, RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));//检测轮廓，其中参数4：检测所有轮廓，参数5：仅保存拐点信息
																							  //轮廓点数过滤
	contours_numfilter(contours, 50, filterContours1);
	//对轮廓进行椭圆环毛刺删除处理
	circle_filter(filterContours1, filterContours2);
	contours_numfilter(filterContours2, 50, filterContours3);
	//对轮廓进行椭圆环毛刺删除处理
	circle_filter(filterContours3, filterContours4);
	contours_numfilter(filterContours4, 50, filterContours);

	//hierarchy.clear();
	//contours.clear();

	cv::vector<RotatedRect> box_cash;//保存椭圆信息
	for (int i = 0; i < (int)filterContours.size(); i++)
	{

		{	RotatedRect box;
		box = fitEllipse(Mat(filterContours[i]));
		double anger_percent = Circle_percent(filterContours[i]);
		if (anger_percent >= 0.9)
			box_cash.push_back(box);
		}
	}

	filterContours.clear();
	filterContours1.clear();
	/*
	将box_cash中的box按hieht从小到大排序
	*/
	//Circle_msg box_result;
	//initCirclMsg(box_result);
	if ((int)box_cash.size()<3)
	{
		RotatedRect box_linsh;
		box_linsh.angle = 0;
		box_linsh.center.x = 0;		box_linsh.center.y = 0;
		box_linsh.size.height = 0;	box_linsh.size.width = 0;
		box_result.box_standard = box_linsh;
		box_result.box1 = box_linsh;
		box_result.box2 = box_linsh;
		box_result.box3 = box_linsh;
		box_result.box4 = box_linsh;
		box_result.box5 = box_linsh;
		box_result.flag = false;
		return;
	}

	RotatedRect box_lishi = box_cash[0];
	for (int i = 0; i<((int)box_cash.size()) - 1; i++)
		for (int j = i + 1; j<(int)box_cash.size(); j++)
		{
			if (box_cash[j].size.height<box_cash[i].size.height)
			{
				box_lishi = box_cash[i];
				box_cash[i] = box_cash[j];
				box_cash[j] = box_lishi;
			}
		}

	/*
	删除box_cash中的重复椭圆
	*/

	RotatedRect box_max = box_cash[0];
	for (int i = 0; i<(int)box_cash.size(); i++)
	{
		if ((box_cash[i].size.area())>box_max.size.area())
			box_max = box_cash[i];
	}

	cv::vector<RotatedRect>::iterator iter = box_cash.begin();
	int cap = (int)box_cash.size();
	int i = 0;
	while (i<cap)
	{
		double dis_box_boxmax = dis_point(box_cash[i].center, box_max.center);

		//	if((fabs(box_cash[i].size.height-box_cash[i-1].size.height)/box_cash[i].size.height<0.3)||(box_cash[i].size.height<40)||dis_box_boxmax>30)
		if ((box_cash[i].size.height<40) || dis_box_boxmax>30)
		{
			iter = box_cash.erase(iter);
			cap--;
		}
		else
		{
			iter++;
			i++;
		}

	}
	cap = (int)box_cash.size();
	if (box_cash.size()<3)
	{
		RotatedRect box_linsh;
		box_linsh.angle = 0;
		box_linsh.center.x = 0;		box_linsh.center.y = 0;
		box_linsh.size.height = 0;	box_linsh.size.width = 0;
		box_result.box_standard = box_cash[0];
		box_result.box1 = box_linsh;
		box_result.box2 = box_linsh;
		box_result.box3 = box_linsh;
		box_result.box4 = box_linsh;
		box_result.box5 = box_linsh;
		box_result.flag = false;
		return;
	}

	i = 1;
	iter = box_cash.begin();
	iter++;
	while (i<cap)
	{
		if (fabs(box_cash[i].size.height - box_cash[i - 1].size.height) / box_cash[i - 1].size.height<0.25)
		{
			box_cash[i - 1] = FusionEllipse(box_cash[i], box_cash[i - 1]);
			iter = box_cash.erase(iter);
			cap--;
		}
		else
		{
			iter++;
			i++;
		}

	}

	if (box_cash.size()<3)
	{
		RotatedRect box_linsh;
		box_linsh.angle = 0;
		box_linsh.center.x = 0;		box_linsh.center.y = 0;
		box_linsh.size.height = 0;	box_linsh.size.width = 0;
		box_result.box_standard = box_cash[0];
		box_result.box1 = box_linsh;
		box_result.box2 = box_linsh;
		box_result.box3 = box_linsh;
		box_result.box4 = box_linsh;
		box_result.box5 = box_linsh;
		box_result.flag = false;
		return;
	}

	double dis_box_boxmax = dis_point(box_cash[0].center, box_max.center);
	iter = box_cash.begin();
	if ((box_cash[0].size.height<40) || (dis_box_boxmax>30))
	{
		iter = box_cash.erase(iter);
	}

	if (box_cash.size()<3)
	{
		RotatedRect box_linsh;
		box_linsh.angle = 0;
		box_linsh.center.x = 0;		box_linsh.center.y = 0;
		box_linsh.size.height = 0;	box_linsh.size.width = 0;
		box_result.box_standard = box_cash[0];
		box_result.box1 = box_linsh;
		box_result.box2 = box_linsh;
		box_result.box3 = box_linsh;
		box_result.box4 = box_linsh;
		box_result.box5 = box_linsh;
		box_result.flag = false;
	}
	else
	{
		RotatedRect box_linsh;
		box_linsh.angle = 0;
		box_linsh.center.x = 0;		box_linsh.center.y = 0;
		box_linsh.size.height = 0;	box_linsh.size.width = 0;
		box_result.box_standard = box_cash[0];
		box_result.box1 = box_cash[0];
		box_result.box2 = box_cash[1];
		box_result.box3 = box_cash[2];
		//计算第四个圆环的参数

		RotatedRect box_3;
		box_3.angle = 2 * box_cash[2].angle - box_cash[1].angle;//椭圆偏转角
		box_3.center.x = 2 * box_cash[2].center.x - box_cash[1].center.x;
		box_3.center.y = 2 * box_cash[2].center.y - box_cash[1].center.y;
		box_3.size.height = 2 * box_cash[2].size.height - box_cash[1].size.height;
		box_3.size.width = 2 * box_cash[2].size.width - box_cash[1].size.width;
		box_result.box4 = box_3;

		//计算第五个圆环的参数
		RotatedRect box_4;
		box_4.angle = 2 * box_result.box4.angle - box_cash[2].angle;//椭圆偏转角	    
		box_4.center.x = box_result.box4.center.x;
		box_4.center.y = box_result.box4.center.y;
		box_4.size.height = 2 * box_cash[2].size.height - box_cash[0].size.height;
		box_4.size.width = 2 * box_cash[2].size.width - box_cash[0].size.width;
		box_result.box5 = box_4;
		box_result.flag = true;
	}
	contours.clear();
	filterContours1.clear();
	filterContours.clear();
	hierarchy.clear();
	return;
}


float calculElip(float a, float b, float cx, float cy, float x0, float y0) {
	float xx, yy;

	xx = (x0 - cx) * (x0 - cx) / a / a;
	yy = (y0 - cy) * (y0 - cy) / b / b;
	return (xx + yy);
}

/////////////////////////////////////////////////////////////////

cameraDevice::cameraDevice()
{
	circlePrm.erodeSize = 3;
	circlePrm.dilateSize = 3;
	circlePrm.erodeType = MORPH_RECT;
	circlePrm.dilateType = MORPH_RECT;
	circlePrm.blurSz = { 3, 3 };
	circlePrm.cannythreshold1 = 20;
	circlePrm.cannythreshold2 = 80;
	circlePrm.cannyapertureSz = 3;
	circlePrm.findCounterMode = RETR_TREE;
	circlePrm.findCounterMethod = CV_CHAIN_APPROX_NONE;
	circlePrm.tooLessPixels = 10;
	circlePrm.mianjiThreshold = 100;
	circlePrm.HeigWidthRio = 0.2;
	circlePrm.boxCashHeigWidthRio = 0.2;

	setCirclParam();

	/////////////////////////////////////

	defctPrm.imgshiftxThrd = 0.5;
	defctPrm.imgshiftyThrd = 0.5;
	defctPrm.diffImgGain = 1.6;
	defctPrm.mediaBlurSz = 5;
	defctPrm.roiXmarg = 10;
	defctPrm.roiYmarg = 10;
	defctPrm.roiHmarg = 20;
	defctPrm.roiWmarg = 20;
	defctPrm.erodeSize = 3;
	defctPrm.thresholdMin = 60;
	defctPrm.thresholdMax = 255;
	defctPrm.thresholdType = 0;
	defctPrm.secdMediaBlurSz = 3;
	defctPrm.findCounterMode = CV_RETR_EXTERNAL;
	defctPrm.findCounterMethod = CHAIN_APPROX_SIMPLE;
	defctPrm.ratioThrdMax = 2.2;
	defctPrm.ratioThrdMin = 0.4;
	defctPrm.areaThrdMax = 200;
	defctPrm.areaThrdMin = 15;
	defctPrm.gunRectOffsetMarg = 5;
	defctPrm.gunRectSzMarg = 10;
	defctPrm.meandiffThrd = 10;
	defctPrm.elipThrd = 1.05;
	defctPrm.posShiftThrd = 2.0;
	setDefctParam();
	logEnable = false;
	LogIntermedia = 0;
	setGlobalParam();
}


void cameraDevice::updateImgAndGunPos(Mat& firstImg, vector<Point2f>& allguns, Circle_msg& firstImgMsg, float imgshiftx, float imgshifty, Circle_msg& box_msg3)
{
	cv::Size dst_sz = firstImg.size();
	cv::Mat t_mat = cv::Mat::zeros(2, 3, CV_32FC1);
	t_mat.at<float>(0, 0) = 1;
	t_mat.at<float>(0, 2) = imgshiftx; //水平平移量
	t_mat.at<float>(1, 1) = 1;
	t_mat.at<float>(1, 2) = imgshifty; //竖直平移量
									   //根据平移矩阵进行仿射变换
	cv::warpAffine(firstImg, firstImg, t_mat, dst_sz);

	// update shot position
	//vector<Point>* allgun = &allguns;
	if (allguns.begin() != allguns.end())
	{
		for (int ii = 0; ii < allguns.size(); ii++)
		{
			allguns[ii].x += imgshiftx;
			allguns[ii].y += imgshifty;
		}
	}

	// update fisrt image position
	firstImgMsg.box1.center.x = box_msg3.box1.center.x;
	firstImgMsg.box1.center.y = box_msg3.box1.center.y;
}

Mat cameraDevice::getRoiEdge(Mat& thirdImg, Circle_msg& box_msg3, int offsetx, int offsety, int roiw, int roih, int mode)
{

	Mat singleImgRoi = thirdImg(Rect(offsetx, offsety, roiw, roih)).clone();
	GaussianBlur(singleImgRoi, singleImgRoi, Size(5, 5), 0, 0);
	Mat singleImgRoiSobel;
	if (mode == 0)
		Canny(singleImgRoi, singleImgRoiSobel, 20, 80, 3, false);//边缘检测
	else if (mode == 1)
	{
		Mat grad_x, grad_y;
		Mat abs_grad_x, abs_grad_y;

		//求x方向梯度
		Sobel(singleImgRoi, grad_x, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT);
		//Scharr(singleImgRoi, grad_x, CV_16S, 1, 0, 1, 0);
		convertScaleAbs(grad_x, abs_grad_x);
		//求y方向梯度
		Sobel(singleImgRoi, grad_y, CV_16S, 0, 1, 3, 1, 1, BORDER_DEFAULT);
		//Scharr(singleImgRoi, grad_y, CV_16S, 0, 1, 1, 0); 
		convertScaleAbs(grad_y, abs_grad_y);
		//合并梯度
		addWeighted(abs_grad_x, 2, abs_grad_y, 2, 0, singleImgRoiSobel);

		if (singleImgRoiSobel.channels() != 1 || singleImgRoiSobel.dims != 2) {
			cv::cvtColor(singleImgRoiSobel, singleImgRoiSobel, CV_BGR2GRAY);
		}
		threshold(singleImgRoiSobel, singleImgRoiSobel, 10, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

		//		Mat singleImgRoiL;
		//		resize(singleImgRoiSobel, singleImgRoiL, Size(roiw * 2, roih * 2));
		Mat element = getStructuringElement(1, Size(3, 3));// MORPH_CROSS
		morphologyEx(singleImgRoiSobel, singleImgRoiSobel, MORPH_CLOSE, element);
	}
	/*
	namedWindow("singleImgRoiSobel", 0);
	cvResizeWindow("singleImgRoiSobel", 600, 500);
	imshow("singleImgRoiSobel", singleImgRoiSobel);
	waitKey();
	destroyWindow("singleImgRoiSobel");
	*/
	return singleImgRoiSobel;
}

Mat cameraDevice::getDiffImg(Mat& thirdImg, Mat& firstImg, Circle_msg& box_msg3,
	int offsetx, int offsety, int roiw, int roih)
{
	// get time spacial diff bullet position
	Mat diff, diffRoi, dst;
	Mat firstRoi, thirdRoi;
	firstRoi = firstImg(Rect(offsetx, offsety, roiw, roih)).clone();
	GaussianBlur(firstRoi, firstRoi, Size(3, 3), 0, 0);
	thirdRoi = thirdImg(Rect(offsetx, offsety, roiw, roih)).clone();
	GaussianBlur(thirdRoi, thirdRoi, Size(3, 3), 0, 0);
	absdiff(firstRoi, thirdRoi, diffRoi);
	//medianBlur(diffRoi, diffRoi, 3);
	diffRoi = diffRoi * 1.1;
	//imshow("diff1", diff*10);
	//adaptiveBilateralFilter(diffRoi, dst, Size(3,3),20);
	//bilateralFilter(diffRoi, dst, 7, 100, 100);
	/*
	namedWindow("diffRoi", 0);
	cvResizeWindow("diffRoi", 600, 500);
	imshow("diffRoi", diffRoi);
	waitKey();
	destroyWindow("diffRoi");
	*/

	if (logEnable && LogIntermedia == 2)
	{
		string logPath = camLogger.getLogPath();
		string diffimgp = logPath + to_string(camLogger.getImgId()) + "\\diffRoiImg.jpg";

		imwrite(diffimgp, diffRoi);
	}

	return diffRoi;
}

vector<Rect> cameraDevice::filterDiffRoi(Mat& diffRoi, Mat& thirdImg, Mat& firstImg, Circle_msg& box_msg3,
	int offsetx, int offsety, int roiw, int roih)
{
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y, dst;
	/*
	//求x方向梯度
	Sobel(diffRoi, grad_x, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT);
	//Scharr(diffRoi, grad_x, CV_16S, 1, 0, 1, 0);
	convertScaleAbs(grad_x, abs_grad_x);
	//求y方向梯度
	Sobel(diffRoi, grad_y, CV_16S, 0, 1, 3, 1, 1, BORDER_DEFAULT);
	//Scharr(diffRoi, grad_y, CV_16S, 0, 1, 1, 0);
	convertScaleAbs(grad_y, abs_grad_y);
	//合并梯度
	addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, dst);

	//int dilateSize = 3;
	//Mat elementForDilate = getStructuringElement(MORPH_RECT, Size(dilateSize, dilateSize), Point(0, 0));
	//dilate(diffimg, diffimg, elementForDilate);
	//int erodeSize = defctPrm.erodeSize; // 3
	//Mat elementForErode = getStructuringElement(MORPH_RECT, Size(erodeSize, erodeSize), Point(0, 0));
	//erode(roi, roi, elementForErode);
	*/
	if (diffRoi.channels() != 1 || diffRoi.dims != 2) {
		cv::cvtColor(diffRoi, diffRoi, CV_BGR2GRAY);
	}
	// 不能用OTSU
	CvScalar mean, std_dev;
	IplImage ipl;
	ipl = IplImage(diffRoi);
	cvAvgSdv(&ipl, &mean, &std_dev);

	//threshold(diffRoi, dst, mean.val[0], 255, CV_THRESH_BINARY | CV_THRESH_OTSU); // 60,255,0
	threshold(diffRoi, dst, mean.val[0] + 3 * std_dev.val[0], 255, CV_THRESH_BINARY); // 60,255,0

	Mat elementForErode = getStructuringElement(MORPH_RECT, Size(3, 3), Point(0, 0));
	erode(dst, dst, elementForErode);
	Mat elementForDilate = getStructuringElement(MORPH_RECT, Size(3, 3), Point(0, 0));
	dilate(dst, dst, elementForDilate);

	if (logEnable && LogIntermedia == 2)
	{
		string logPath = camLogger.getLogPath();
		string thresholdimg = logPath + to_string(camLogger.getImgId()) + "\\threshold.jpg";
		imwrite(thresholdimg, dst);
	}

	/*
	namedWindow("diffRoithrd", 0);
	cvResizeWindow("diffRoithrd", 600, 500);
	imshow("diffRoithrd", dst);
	waitKey();
	destroyWindow("diffRoithrd");
	*/

	// 此时，我们已经得到了较好的边缘图像，但是对于不同光照条件下，仍然会存在一些噪声点，这个时候，就用canny和sobel进一步过滤，得到真正的
	// 弹孔
	// 提取图中小的轮廓
	cv::vector< cv::vector<Point> > contours;
	cv::vector< cv::Vec4i > hierarchy;
	//得到轮廓向量
	Mat dstRoi = dst.clone();
	findContours(dstRoi, contours, hierarchy, RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));//检测轮廓，其中参数4：检测所有轮廓，参数5：仅保存拐点信息

																							// get 第二幅图的roi edge
	Mat firstRoiCanny;
	Mat thirdRoiCanny;
	firstRoiCanny = getRoiEdge(firstImg, box_msg3, offsetx, offsety, roiw, roih, 0);
	if (logEnable && LogIntermedia == 2)
	{
		string logPath = camLogger.getLogPath();
		string firstRoiCannyP = logPath + to_string(camLogger.getImgId()) + "\\firstRoiCanny.jpg";
		imwrite(firstRoiCannyP, firstRoiCanny);
	}
	thirdRoiCanny = getRoiEdge(thirdImg, box_msg3, offsetx, offsety, roiw, roih, 0);
	if (logEnable && LogIntermedia == 2)
	{
		string logPath = camLogger.getLogPath();
		string thirdRoiCannyP = logPath + to_string(camLogger.getImgId()) + "\\thirdRoiCanny.jpg";
		imwrite(thirdRoiCannyP, thirdRoiCanny);
	}
	//**************************************************
	// 找小圆
	cv::vector< cv::vector<Point> > contoursThird, contoursFirst;
	cv::vector< cv::Vec4i > hierarchyThird, hierarchyFirst;
	Mat thirdRoiCannyTemp = thirdRoiCanny.clone();
	Mat firstRoiCannyTemp = firstRoiCanny.clone();
	findContours(thirdRoiCannyTemp, contoursThird, hierarchyThird, RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));//检测轮廓，其中参数4：检测所有轮廓，参数5：仅保存拐点信息
	findContours(firstRoiCannyTemp, contoursFirst, hierarchyFirst, RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));//检测轮廓，其中参数4：检测所有轮廓，参数5：仅保存拐点信息

																												 // filter large 
	cv::vector< cv::vector<Point> >::iterator iterT, iterF;
	cv::vector< cv::Vec4i >::iterator iterHT, iterHF;
	vector< Rect > cannyRectThird, cannyRectFirst;
	for (iterT = contoursThird.begin(), iterHT = hierarchyThird.begin(); iterT != contoursThird.end(), iterHT != hierarchyThird.end();)
	{
		Rect boxT;
		boxT = boundingRect(Mat(*iterT));
		if ((float)boxT.width >= (float)(box_msg3.box1.size.width / 3) ||
			(float)boxT.height >= (float)(box_msg3.box1.size.height / 3) ||
			(float)(boxT.width / boxT.height) > 2 ||
			(float)(boxT.height / boxT.width) > 2 ||
			(boxT.width < 3 && boxT.height < 3) ||
			boxT.x < (int)(box_msg3.box1.size.width / 2) ||  // 靠近边缘的都不是弹孔
			boxT.y < (int)(box_msg3.box1.size.height / 2) ||
			boxT.x + boxT.width + (int)(box_msg3.box1.size.width / 2) > roiw ||
			boxT.y + boxT.height + (int)(box_msg3.box1.size.height / 2) > roih) //  
		{
			iterT = contoursThird.erase(iterT);
			iterHT = hierarchyThird.erase(iterHT);
		}
		else {
			cannyRectThird.push_back(boxT);
			iterT++;
			iterHT++;
		}
	}

	// filter large 
	for (iterF = contoursFirst.begin(), iterHF = hierarchyFirst.begin(); iterF != contoursFirst.end(), iterHF != hierarchyFirst.end();)
	{
		Rect boxT;
		boxT = boundingRect(Mat(*iterF));
		if ((float)boxT.width >= (float)(box_msg3.box1.size.width / 4) ||
			(float)boxT.height >= (float)(box_msg3.box1.size.height / 4) ||
			(boxT.width < 3 && boxT.height < 3) ||
			(float)(boxT.width / boxT.height) > 2 ||
			(float)(boxT.height / boxT.width) > 2 ||
			boxT.x < (int)(box_msg3.box1.size.width / 2) ||  // 靠近边缘的都不是弹孔
			boxT.y < (int)(box_msg3.box1.size.height / 2) ||
			boxT.x + boxT.width + (int)(box_msg3.box1.size.width / 2) > roiw ||
			boxT.y + boxT.height + (int)(box_msg3.box1.size.height / 2) > roih) // 
		{
			iterF = contoursFirst.erase(iterF);
			iterHF = hierarchyFirst.erase(iterHF);
		}
		else {
			cannyRectFirst.push_back(boxT);
			iterF++;
			iterHF++;
		}
	}

	// get final filter points
	vector< Rect > cannyRectFinal;
	for (int i = 0; i < cannyRectThird.size(); i++)
	{
		// get cent point
		Point2f firstRectC, thirdRectC;
		thirdRectC.x = (float)(cannyRectThird[i].x + 0.5 * cannyRectThird[i].width);
		thirdRectC.y = (float)(cannyRectThird[i].y + 0.5 * cannyRectThird[i].height);
		bool ifThirdNotInFirst = true;
		for (int j = 0; j < cannyRectFirst.size(); j++)
		{
			firstRectC.x = (float)(cannyRectFirst[j].x + 0.5 * cannyRectFirst[j].width);
			firstRectC.y = (float)(cannyRectFirst[j].y + 0.5 * cannyRectFirst[j].height);

			if (fabs(firstRectC.x - thirdRectC.x) <  0.35 * cannyRectThird[i].width &&
				fabs(firstRectC.y - thirdRectC.y) < 0.35 * cannyRectThird[i].height)
			{
				ifThirdNotInFirst = false;
				break;
			}
		}
		if (ifThirdNotInFirst)
		{
			cannyRectFinal.push_back(cannyRectThird[i]);
		}
	}

	if (logEnable && LogIntermedia == 2)
	{
		Mat drawMt1 = thirdImg(Rect(offsetx, offsety, roiw, roih)).clone();
		for (int i = 0; i < cannyRectFinal.size(); i++)
		{
			Point p1, p2;
			p1.x = cannyRectFinal[i].x - 2;
			p1.y = cannyRectFinal[i].y - 2;
			p2.x = cannyRectFinal[i].x + cannyRectFinal[i].width + 4;
			p2.y = cannyRectFinal[i].y + cannyRectFinal[i].height + 4;
			rectangle(drawMt1, p1, p2, Scalar(255, 255, 255), 1, 8, 0);
		}
		string logPath = camLogger.getLogPath();
		string cannyCandit = logPath + to_string(camLogger.getImgId()) + "\\cannyCandit.jpg";
		imwrite(cannyCandit, drawMt1);
	}

	///********************************************

	Mat thirdImgRoi = thirdImg(Rect(offsetx, offsety, roiw, roih)).clone();
	if (logEnable && LogIntermedia == 2)
	{
		string logPath = camLogger.getLogPath();
		string thirdImgRoiP = logPath + to_string(camLogger.getImgId()) + "\\thirdImgRoi.jpg";
		imwrite(thirdImgRoiP, thirdImgRoi);
	}
	Mat firstImgRoi = firstImg(Rect(offsetx, offsety, roiw, roih)).clone();
	if (logEnable && LogIntermedia == 2)
	{
		string logPath = camLogger.getLogPath();
		string firstImgRoiP = logPath + to_string(camLogger.getImgId()) + "\\firstImgRoi.jpg";
		imwrite(firstImgRoiP, firstImgRoi);
	}
	vector<Rect> allHole;

	vector<Rect> gunTemp;
	// 得到当前这一枪真正的弹孔图
	for (int i = 0; i < contours.size(); i++)
	{
		// 如果轮廓很大或者很小，肯定不是弹孔,没必要再放进去
		Rect boxT;
		boxT = boundingRect(Mat(contours[i]));
		if (((float)(boxT.width / boxT.height) >= 2 ||
			(float)(boxT.height / boxT.width) >= 2) ||
			(boxT.width < 4 && boxT.height < 4) ||
			boxT.x < (int)(box_msg3.box1.size.width / 2) ||  // 靠近边缘的都不是弹孔
			boxT.y < (int)(box_msg3.box1.size.height / 2) ||
			boxT.x + boxT.width + (int)(box_msg3.box1.size.width / 2) > roiw ||
			boxT.y + boxT.height + (int)(box_msg3.box1.size.height / 2) > roih) // 
		{
			if (logEnable && LogIntermedia == 2)
			{
				Mat mm = dst(Rect(boxT.x, boxT.y, boxT.width, boxT.height));
				mm = { Scalar(0,0,0) };
			}
		}
		else {
			gunTemp.push_back(boxT);
		}
	}

	if (gunTemp.size() == 0)
		return allHole;

	// 合并离的很近的区域, 就这样把，看着舒服
	for (int i = 0; i < (gunTemp.size() - 1); i++)
	{
		for (int j = i + 1; j < gunTemp.size(); j++)
		{
			if (gunTemp[j].x != 0 && gunTemp[j].y != 0)
			{
				Point firstCent, secdCent;
				firstCent.x = gunTemp[i].x + (int)(gunTemp[i].width / 2);
				firstCent.y = gunTemp[i].y + (int)(gunTemp[i].height / 2);
				secdCent.x = gunTemp[j].x + (int)(gunTemp[j].width / 2);
				secdCent.y = gunTemp[j].y + (int)(gunTemp[j].height / 2);
				Size distn;
				distn.width = (int)(gunTemp[i].width / 2 + gunTemp[j].width / 2);
				distn.height = (int)(gunTemp[i].height / 2 + gunTemp[j].height / 2);
				if (abs(abs(firstCent.x - secdCent.x) - distn.width) < 3 &&
					abs(abs(firstCent.y - secdCent.y) - distn.height) < 3)
				{
					gunTemp[i].width = max(gunTemp[i].x + gunTemp[i].width, gunTemp[j].x + gunTemp[j].width) - min(gunTemp[i].x, gunTemp[j].x);
					gunTemp[i].height = max(gunTemp[i].y + gunTemp[i].height, gunTemp[j].y + gunTemp[j].height) - min(gunTemp[i].y, gunTemp[j].y);
					gunTemp[i].x = min(gunTemp[i].x, gunTemp[j].x);
					gunTemp[i].y = min(gunTemp[i].y, gunTemp[j].y);
					gunTemp[j].x = 0;
					gunTemp[j].y = 0;
				}
			}
		}
	}
	if (logEnable && LogIntermedia == 2)
	{
		Mat drawMt2 = thirdImg(Rect(offsetx, offsety, roiw, roih)).clone();
		for (int i = 0; i < gunTemp.size(); i++)
		{
			Point p1, p2;
			p1.x = gunTemp[i].x - 2;
			p1.y = gunTemp[i].y - 2;
			p2.x = gunTemp[i].x + gunTemp[i].width + 4;
			p2.y = gunTemp[i].y + gunTemp[i].height + 4;
			rectangle(drawMt2, p1, p2, Scalar(0, 0, 0), 1, 8, 0);
		}
		string logPath = camLogger.getLogPath();
		string gunCandit = logPath + to_string(camLogger.getImgId()) + "\\gunCandit.jpg";
		imwrite(gunCandit, drawMt2);
	}
	/*
	namedWindow("before-final", 0);
	cvResizeWindow("before-final", 600, 500);
	imshow("before-final", dst);
	waitKey();
	destroyWindow("before-final");
	*/
	for (int i = 0; i < gunTemp.size(); i++)
	{
		Rect boxT = gunTemp[i];
		float boxTArea = (float)(boxT.width * boxT.height);
		Point2f centT;
		centT.x = (float)(boxT.x + 0.5f * boxT.width);
		centT.y = (float)(boxT.y + 0.5f * boxT.height);
		if ((boxT.x != 0 && boxT.y != 0) &&
			((float)(boxT.width / boxT.height) < 2 && (float)(boxT.height / boxT.width) < 2))
		{
			// also in cannyRectFinal
			bool thisRectInCannyRect = false;
			for (int ii = 0; ii < cannyRectFinal.size(); ii++)
			{
				Point2f cannyCentT;
				cannyCentT.x = (float)(cannyRectFinal[ii].x + 0.5f * cannyRectFinal[ii].width);
				cannyCentT.y = (float)(cannyRectFinal[ii].y + 0.5f * cannyRectFinal[ii].height);
				float cannyBoxTArea = (float)(cannyRectFinal[ii].width * cannyRectFinal[ii].height);
				if (fabs(centT.x - cannyCentT.x) < 0.6 * boxT.width &&
					fabs(centT.y - cannyCentT.y) < 0.6 * boxT.height)
				{
					thisRectInCannyRect = true;
				}
			}

			// 在 sobel和canny图中对应位置，看看是否为弹孔
			Mat imgMMthird = thirdImgRoi(Rect(boxT.x + 1, boxT.y + 1, boxT.width - 2, boxT.height - 2));
			Mat imgMMfirst = firstImgRoi(Rect(boxT.x + 1, boxT.y + 1, boxT.width - 2, boxT.height - 2));
			Mat imgMMthirdExd = thirdImgRoi(Rect(boxT.x - 2, boxT.y - 2, boxT.width + 4, boxT.height + 4));
			Mat imgMMfirstExd = firstImgRoi(Rect(boxT.x - 2, boxT.y - 2, boxT.width + 4, boxT.height + 4));

			// 图像roi的均值和方差
			CvScalar meanImgfirst, std_devImgfirst;
			IplImage iplImgfirst;
			iplImgfirst = IplImage(imgMMfirst);
			cvAvgSdv(&iplImgfirst, &meanImgfirst, &std_devImgfirst);

			CvScalar meanImgthird, std_devImgthird;
			IplImage iplImgthird;
			iplImgthird = IplImage(imgMMthird);
			cvAvgSdv(&iplImgthird, &meanImgthird, &std_devImgthird);

			CvScalar meanImgfirstExd, std_devImgfirstExd;
			IplImage iplImgfirstExd;
			iplImgfirstExd = IplImage(imgMMfirstExd);
			cvAvgSdv(&iplImgfirstExd, &meanImgfirstExd, &std_devImgfirstExd);

			CvScalar meanImgthirdExd, std_devImgthirdExd;
			IplImage iplImgthirdExd;
			iplImgthirdExd = IplImage(imgMMthirdExd);
			cvAvgSdv(&iplImgthirdExd, &meanImgthirdExd, &std_devImgthirdExd);

			// 根据以上信息进行过滤， 基本上可以得到最终的弹孔， 保存弹孔信息
			float meanImgthirdExdVal = 0, meanImgfirstExdVal = 0;
			meanImgthirdExdVal = (meanImgthirdExd.val[0] * (boxT.width + 6) * (boxT.height + 6) -
				meanImgthird.val[0] * boxT.width * boxT.height) / ((boxT.width + 6) * (boxT.height + 6) - boxT.width * boxT.height);
			meanImgfirstExdVal = (meanImgfirstExd.val[0] * (boxT.width + 6) * (boxT.height + 6) -
				meanImgfirst.val[0] * boxT.width * boxT.height) / ((boxT.width + 6) * (boxT.height + 6) - boxT.width * boxT.height);

			if (meanImgthirdExdVal > (meanImgthird.val[0] + 1 * std_devImgthird.val[0]) &&
				thisRectInCannyRect) //是弹孔
			{
				allHole.push_back(boxT);
			}
			else
			{
				if (logEnable && LogIntermedia == 2)
				{
					Mat mm = dst(Rect(boxT.x, boxT.y, boxT.width, boxT.height));
					mm = { Scalar(0,0,0) };
				}
			}
		}
		else {
			if (logEnable && LogIntermedia == 2)
			{
				Mat mm = dst(Rect(boxT.x, boxT.y, boxT.width, boxT.height));
				mm = { Scalar(0,0,0) };
			}
		}
	}
	/*
	namedWindow("final", 0);
	cvResizeWindow("final", 600, 500);
	imshow("final", dst);
	waitKey();
	destroyWindow("final");
	*/

	if (logEnable && LogIntermedia == 2)
	{
		string logPath = camLogger.getLogPath();
		string finalFilteredImg = logPath + to_string(camLogger.getImgId()) + "\\finalFilteredImg.jpg";
		imwrite(finalFilteredImg, dst);
	}

	return allHole;
}

void cameraDevice::getBullet(Mat& thirdImg, vector<Rect> diffRoiFiltered, int offsetx, int offsety, map<int, struct elipParam>& fiveElip)
{
	currentPos.clear();  // 先把当前的环数清零
	for (int i = 0; i < diffRoiFiltered.size(); i++)
	{
		int candtX, candtY;
		candtX = diffRoiFiltered[i].x + offsetx + diffRoiFiltered[i].width * 1.0f / 2;
		candtY = diffRoiFiltered[i].y + offsety + diffRoiFiltered[i].height * 1.0f / 2;

		// check if it not in the vector
		bool isnotin = true;
		for (int jj = 0; jj < allguns.size(); jj++)
		{
			Point newpos;
			newpos.x = candtX;
			newpos.y = candtY;
			if (abs(allguns[jj].x - newpos.x) < defctPrm.posShiftThrd &&  // 2.0
				abs(allguns[jj].y - newpos.y) < defctPrm.posShiftThrd)
			{
				isnotin = false;
				break;
			}
		}

		if (isnotin)
		{
			Point gunPos;
			gunPos.x = candtX;
			gunPos.y = candtY;
			allguns.push_back(gunPos);
			Mat roi;
			if (logEnable)
				roi = thirdImg.clone();
			judgePos(roi, gunPos, fiveElip);
		}
	}
}

void cameraDevice::judgePos(Mat& roi, Point& gunPos, map<int, struct elipParam>& fiveElip)
{
	int resltH = 0;
	float el1, el2, el3, el4, el5;
	el1 = calculElip(fiveElip[1].a, fiveElip[1].b, fiveElip[1].cx, fiveElip[1].cy, gunPos.x, gunPos.y);
	el2 = calculElip(fiveElip[2].a, fiveElip[2].b, fiveElip[2].cx, fiveElip[2].cy, gunPos.x, gunPos.y);
	el3 = calculElip(fiveElip[3].a, fiveElip[3].b, fiveElip[3].cx, fiveElip[3].cy, gunPos.x, gunPos.y);
	el4 = calculElip(fiveElip[4].a, fiveElip[4].b, fiveElip[4].cx, fiveElip[4].cy, gunPos.x, gunPos.y);
	el5 = calculElip(fiveElip[5].a, fiveElip[5].b, fiveElip[5].cx, fiveElip[5].cy, gunPos.x, gunPos.y);

	// 10
	if (el1 <= defctPrm.elipThrd) // 1.05
	{
		score.second = 10;
		score.first++;
		currentPos.push_back(gunPos);

	}
	else if (el1 > defctPrm.elipThrd && el2 <= defctPrm.elipThrd)
	{
		score.second = 9;
		score.first++;
		currentPos.push_back(gunPos);

	}
	else if (el2 > defctPrm.elipThrd && el3 <= defctPrm.elipThrd)
	{
		score.second = 8;
		score.first++;
		currentPos.push_back(gunPos);

	}
	else if (el3 > defctPrm.elipThrd && el4 <= defctPrm.elipThrd)
	{
		score.second = 7;
		score.first++;
		currentPos.push_back(gunPos);

	}
	else if (el4 > defctPrm.elipThrd && el5 <= 1.2)
	{
		score.second = 6;
		score.first++;
		currentPos.push_back(gunPos);
	}
	else
	{
		score.second = 0;
	}

}

void cameraDevice::calcElip(map<int, struct elipParam>& fiveElip, Circle_msg& box_msg3)
{
	for (int i = 1; i < 6; i++)
	{
		struct elipParam elipP;
		if (i == 1)
		{
			elipP.a = box_msg3.box1.size.width / 2;
			elipP.b = box_msg3.box1.size.height / 2;
			elipP.cx = box_msg3.box1.center.x;
			elipP.cy = box_msg3.box1.center.y;
		}
		else if (i == 2) {
			elipP.a = box_msg3.box2.size.width / 2;
			elipP.b = box_msg3.box2.size.height / 2;
			elipP.cx = box_msg3.box2.center.x;
			elipP.cy = box_msg3.box2.center.y;
		}
		else if (i == 3) {
			elipP.a = box_msg3.box3.size.width / 2;
			elipP.b = box_msg3.box3.size.height / 2;
			elipP.cx = box_msg3.box3.center.x;
			elipP.cy = box_msg3.box3.center.y;
		}
		else if (i == 4) {
			elipP.a = box_msg3.box4.size.width / 2;
			elipP.b = box_msg3.box4.size.height / 2;
			elipP.cx = box_msg3.box4.center.x;
			elipP.cy = box_msg3.box4.center.y;
		}
		else if (i == 5) {
			elipP.a = box_msg3.box5.size.width / 2;
			elipP.b = box_msg3.box5.size.height / 2;
			elipP.cx = box_msg3.box5.center.x;
			elipP.cy = box_msg3.box5.center.y;
		}
		fiveElip[i] = elipP;
	}
}

void cameraDevice::calcGun(Mat src, char* camIp)
{
	if (logEnable)
	{
		camLogger.setImgId();
		if (LogIntermedia == 1 || LogIntermedia == 2)
		{
			string logPath = camLogger.getLogPath();
			string folderp = logPath + to_string(camLogger.getImgId());
			string inputimg = logPath + to_string(camLogger.getImgId()) + "\\inputImg" + to_string(camLogger.getImgId()) + ".jpg";
			if (_access(folderp.c_str(), 6) == -1)
			{
				_mkdir(folderp.c_str());
			}
			imwrite(inputimg, src);
		}
	}

	struct defctParam defctPrm = getDefctParam();
	Mat firstImg;
	Mat thirdImg;
	string sIp = camIp;

	Circle_msg box_msg3;
	initCirclMsg(box_msg3);
	Circle_msg box_msg1;
	initCirclMsg(box_msg1);
	thirdImg = src;  // 先把这幅图拿出来求圆形
	Circle_recognize(thirdImg, box_msg3);
	if (logEnable && LogIntermedia == 2)
	{
		string circleInfo;
		circleInfo.append("find circle status = ").append(to_string(box_msg3.flag));
		camLogger.doLog(circleInfo);
		// save image
		if (box_msg3.flag)
		{
			Mat dstimg = thirdImg.clone();
			int thickness = 2;
			int lineType = 8;
			ellipse(dstimg, box_msg3.box1.center, box_msg3.box1.size, box_msg3.box1.angle, 0, 360, Scalar(255, 129, 0), thickness, lineType);
			ellipse(dstimg, box_msg3.box2.center, box_msg3.box2.size, box_msg3.box2.angle, 0, 360, Scalar(255, 129, 0), thickness, lineType);
			ellipse(dstimg, box_msg3.box3.center, box_msg3.box3.size, box_msg3.box3.angle, 0, 360, Scalar(255, 129, 0), thickness, lineType);
			ellipse(dstimg, box_msg3.box4.center, box_msg3.box4.size, box_msg3.box4.angle, 0, 360, Scalar(255, 129, 0), thickness, lineType);
			ellipse(dstimg, box_msg3.box5.center, box_msg3.box5.size, box_msg3.box5.angle, 0, 360, Scalar(255, 129, 0), thickness, lineType);
			string logPath = camLogger.getLogPath();
			string circleimg = logPath + to_string(camLogger.getImgId()) + "\\circleImg.jpg";
			imwrite(circleimg, dstimg);
		}
	}

	if (box_msg3.flag == false)  // 没有检测到圆，什么也不干
	{
		currentPos.clear();
		pair<int, int> init(score.first, 0);
		score = init;
		return;
	}
	if (!firstImgT.empty()) {  // 如果已经保存有这个相机的一张图片
		firstImg = firstImgT;
		map<int, struct elipParam> fiveElip;
		calcElip(fiveElip, box_msg3);

		box_msg1 = firstImgMsgT;
		// 计算相互之间的位移
		// 同时利用参考系
		float imgshiftx = box_msg3.box1.center.x - box_msg1.box1.center.x;
		float imgshifty = box_msg3.box1.center.y - box_msg1.box1.center.y;

		if (abs(imgshiftx) > 20 || abs(imgshifty) > 20) // 如果位移太大，认为是移动距离太大，不适合做进一步分析
		{
			currentPos.clear();
			firstImgT = thirdImg.clone();
			firstImgMsgT = box_msg3;
			pair<int, int> init(score.first, 0);
			score = init;
			return;
		}

		if (abs(imgshiftx) > defctPrm.imgshiftxThrd && abs(imgshifty) > defctPrm.imgshiftyThrd) { // 0.5, 0.5
			updateImgAndGunPos(firstImg, allguns, firstImgMsgT, imgshiftx, imgshifty, box_msg3);
		}

		Mat diffimgRoi;
		int offsetx, offsety, roiw, roih;
		offsetx = int(box_msg3.box1.center.x - (box_msg3.box1.size.width / 2) * 6);
		if (offsetx < 0)
			offsetx = 0;
		offsety = int(box_msg3.box1.center.y - (box_msg3.box1.size.height / 2) * 6);
		if (offsety < 0)
			offsety = 0;
		roiw = int(box_msg3.box1.size.width * 6);
		if ((offsetx + roiw) > thirdImg.size().width)
			roiw = thirdImg.size().width - offsetx - 1;
		roih = int(box_msg3.box1.size.height * 6);
		if ((offsety + roih) > thirdImg.size().height)
			roih = thirdImg.size().height - offsety - 1;
		// 分别得到 图像的diff, canny edge 的diff, sobel edge 的diff
		diffimgRoi = getDiffImg(thirdImg, firstImg, box_msg3, offsetx, offsety, roiw, roih);
		vector<Rect> diffRoiFiltered;
		diffRoiFiltered = filterDiffRoi(diffimgRoi, thirdImg, firstImg, box_msg3, offsetx, offsety, roiw, roih);
		if (diffRoiFiltered.size() == 0)
		{
			currentPos.clear();  // 先把当前的环数清零
			firstImgT = thirdImg.clone();
			firstImgMsgT = box_msg3;
			pair<int, int> init(score.first, 0);
			score = init;
			return;
		}
		// 判断是几环
		getBullet(thirdImg, diffRoiFiltered, offsetx, offsety, fiveElip);

		firstImgT = thirdImg.clone();
		firstImgMsgT = box_msg3;
	}
	else {
		firstImgT = thirdImg.clone();
		firstImgMsgT = box_msg3;
		pair<int, int> init(0, 0);
		score = init;
	}
}

void cameraDevice::drawGunPos(Mat* src, int* goal, int* cnt)
{
	//*cnt = score.first;
	//*goal = score.second;
	// draw rec on the shot
	Point p1, p2;
	vector<Point> tmp = getCurrentPos();
	for (int i = 0; i < tmp.size(); i++)
	{
		p1.x = tmp[i].x - 10;
		p1.y = tmp[i].y - 10;
		p2.x = tmp[i].x + 10;
		p2.y = tmp[i].y + 10;
		rectangle(*src, p1, p2, Scalar(255, 255, 255), 1, 8, 0);
	}
}

void cameraDevice::setDefctParam()
{
	// then read from cfg file
	string cfgF;
	cfgF.append("..\\cfg\\defctCfgParam.txt");
	ifstream fout(cfgF);
	string line;

	string::size_type left = 0, right = 0;
	string leftstr, rightstr;
	while (fout >> line)
	{
		right = line.find('=', left);
		leftstr = line.substr(0, right);
		rightstr = line.substr(right + 1, strlen(line.data()));

		if (leftstr == "imgshiftxThrd")
			defctPrm.imgshiftxThrd = atof(rightstr.c_str());
		else if (leftstr == "imgshiftyThrd")
			defctPrm.imgshiftyThrd = atof(rightstr.c_str());
		else if (leftstr == "diffImgGain")
			defctPrm.diffImgGain = atof(rightstr.c_str());
		else if (leftstr == "mediaBlurSz")
			defctPrm.mediaBlurSz = atoi(rightstr.c_str());
		else if (leftstr == "roiXmarg")
			defctPrm.roiXmarg = atoi(rightstr.c_str());
		else if (leftstr == "roiYmarg")
			defctPrm.roiYmarg = atoi(rightstr.c_str());
		else if (leftstr == "roiHmarg")
			defctPrm.roiHmarg = atoi(rightstr.c_str());
		else if (leftstr == "roiWmarg")
			defctPrm.roiWmarg = atoi(rightstr.c_str());
		else if (leftstr == "erodeSize")
			defctPrm.erodeSize = atoi(rightstr.c_str());
		else if (leftstr == "thresholdMin")
			defctPrm.thresholdMin = atoi(rightstr.c_str());
		else if (leftstr == "thresholdMax")
			defctPrm.thresholdMax = atoi(rightstr.c_str());
		else if (leftstr == "thresholdType")
			defctPrm.thresholdType = atoi(rightstr.c_str());
		else if (leftstr == "secdMediaBlurSz")
			defctPrm.secdMediaBlurSz = atoi(rightstr.c_str());
		else if (leftstr == "findCounterMode")
			defctPrm.findCounterMode = atoi(rightstr.c_str());
		else if (leftstr == "findCounterMethod")
			defctPrm.findCounterMethod = atoi(rightstr.c_str());
		else if (leftstr == "ratioThrdMax")
			defctPrm.ratioThrdMax = atof(rightstr.c_str());
		else if (leftstr == "ratioThrdMin")
			defctPrm.ratioThrdMin = atof(rightstr.c_str());
		else if (leftstr == "areaThrdMax")
			defctPrm.areaThrdMax = atoi(rightstr.c_str());
		else if (leftstr == "areaThrdMin")
			defctPrm.areaThrdMin = atoi(rightstr.c_str());
		else if (leftstr == "gunRectOffsetMarg")
			defctPrm.gunRectOffsetMarg = atoi(rightstr.c_str());
		else if (leftstr == "gunRectSzMarg")
			defctPrm.gunRectSzMarg = atoi(rightstr.c_str());
		else if (leftstr == "meandiffThrd")
			defctPrm.meandiffThrd = atoi(rightstr.c_str());
		else if (leftstr == "elipThrd")
			defctPrm.elipThrd = atof(rightstr.c_str());
		else if (leftstr == "posShiftThrd")
			defctPrm.posShiftThrd = atof(rightstr.c_str());
	}

}

struct circlParam cameraDevice::getCirclParam()
{
	return circlePrm;
}

void cameraDevice::setCirclParam()
{
	// then read from cfg file
	string cfgF;
	cfgF.append("..\\cfg\\circleCfgParam.txt");
	ifstream fout(cfgF);
	string line;

	string::size_type left = 0, right = 0;
	string leftstr, rightstr;
	while (fout >> line)
	{
		right = line.find('=', left);
		leftstr = line.substr(0, right);
		rightstr = line.substr(right + 1, strlen(line.data()));

		if (leftstr == "erodeSize")
			circlePrm.erodeSize = atoi(rightstr.c_str());
		else if (leftstr == "dilateSize")
			circlePrm.dilateSize = atoi(rightstr.c_str());
		else if (leftstr == "erodeType")
			circlePrm.erodeType = atoi(rightstr.c_str());
		else if (leftstr == "dilateType")
			circlePrm.dilateType = atoi(rightstr.c_str());
		else if (leftstr == "blurSzW")
			circlePrm.blurSz.width = atoi(rightstr.c_str());
		else if (leftstr == "blurSzH")
			circlePrm.blurSz.height = atoi(rightstr.c_str());
		else if (leftstr == "cannythreshold1")
			circlePrm.cannythreshold1 = atoi(rightstr.c_str());
		else if (leftstr == "cannythreshold2")
			circlePrm.cannythreshold2 = atoi(rightstr.c_str());
		else if (leftstr == "cannyapertureSz")
			circlePrm.cannyapertureSz = atoi(rightstr.c_str());
		else if (leftstr == "findCounterMode")
			circlePrm.findCounterMode = atoi(rightstr.c_str());
		else if (leftstr == "findCounterMethod")
			circlePrm.findCounterMethod = atoi(rightstr.c_str());
		else if (leftstr == "tooLessPixels")
			circlePrm.tooLessPixels = atoi(rightstr.c_str());
		else if (leftstr == "mianjiThreshold")
			circlePrm.mianjiThreshold = atoi(rightstr.c_str());
		else if (leftstr == "HeigWidthRio")
			circlePrm.HeigWidthRio = atof(rightstr.c_str());
		else if (leftstr == "boxCashHeigWidthRio")
			circlePrm.boxCashHeigWidthRio = atof(rightstr.c_str());
	}

}

struct defctParam cameraDevice::getDefctParam()
{
	return defctPrm;
}

void cameraDevice::setGlobalParam()
{
	// then read from cfg file
	string cfgF;
	cfgF.append("..\\cfg\\globalCfgParam.txt");
	ifstream fout(cfgF);
	string line;

	string::size_type left = 0, right = 0;
	string leftstr, rightstr;
	while (fout >> line)
	{
		right = line.find('=', left);
		leftstr = line.substr(0, right);
		rightstr = line.substr(right + 1, strlen(line.data()));

		if (leftstr == "logEnable")
			logEnable = atoi(rightstr.c_str());
		else if (leftstr == "LogIntermedia")
			LogIntermedia = atoi(rightstr.c_str());  // 1: only log input image, 2: log all information
	}
}

/////////////////////////////////////////////////////////////////

fatherPapa* fatherPapa::getInstance()
{
	return &instance;
}

void fatherPapa::setCamDev(string camIp)
{
	if (!allDev.count(camIp)) // no this camera
	{
		cameraDevice* newDev = new cameraDevice();
		if (!newDev)
			cout << "can not create camera device\n" << endl;
		allDev[camIp] = newDev;
	}
}

cameraDevice* fatherPapa::getCamDev(string camIp)
{
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	if (allDev.count(camIp)) // if has this camera, return it
	{
		return allDev[camIp];
	}
	else {
		setCamDev(camIp);
		return allDev[camIp];
	}
}


pair<int, int> cameraDevice::getScore()
{
	return score;
}

vector<Point> cameraDevice::getCurrentPos()
{
	return currentPos;
}

void cameraDevice::setLogger(string camIp)
{
	camLogger.setLogPath(camIp);
}

logger::logger()
{
	logPath = "..\\log\\";
	if (_access(logPath.c_str(), 6) == -1)
	{
		_mkdir(logPath.c_str());
	}

	camImgId = 0;
}

void logger::doLog(string msg)
{
	ofstream fin(logFile);
	fin << "line：" << __LINE__ << "fun：" << __FUNCTION__ << ": message" << msg;
}

void logger::setLogPath(string camIp)
{
	logPath.append(camIp);
	logFile.append(logPath).append("\\trace.log");
}
string logger::getLogPath()
{
	return logPath;
}

void uploadRslt(Mat* src, char* camIp, int* goal, int* cnt) {
	string s = camIp;
	fatherPapa* allDev = fatherPapa::getInstance();
	cameraDevice* pCamDev = allDev->getCamDev(s);

	pCamDev->calcGun(*src, camIp);

	pair<int, int> scr = pCamDev->getScore();
	*cnt = scr.first;
	*goal = scr.second;
	if (scr.first != 0 && scr.second != 0) {
		pCamDev->drawGunPos(src, goal, cnt);
	}

}


