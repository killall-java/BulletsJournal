#include "stdafx.h"
#include "getgun.h"

static map<string, Mat> firstImgs;   // 存储每个相机上一时刻的一幅图
static map<string, pair<int, int> > scores;   // pair中第一个表示截至到当前打了多少枪，第二个表示当前是几环
static map<string, vector<Point> > allguns;    // instore all the shot position of every camera
static map<string, Circle_msg> firstImgMsgs;  // instore the first image circle info

/*函数：求两点之间的距离
输入：四个坐标信息x1、 y1、x2、y2
输出：double型变量

*/
double Cal_Distance(double x1, double y1, double x2, double y2){
	return sqrt((x1*x1 + x2 * x2 - 2 * x1*x2) + (y1*y1 + y2 * y2 - 2 * y1*y2));
}

/*椭圆融合函数，当两个圆环的半径接近且圆心相近时，将其融合为一个圆环
*
*输入：需要融合的两个椭圆对应的box1、box2
*输出：融合后的椭圆box */
RotatedRect FusionEllipse(RotatedRect box1, RotatedRect box2){
	RotatedRect box;
	box.center.x = (box1.center.x + box2.center.x) / 2;
	box.center.y = (box1.center.y + box2.center.y) / 2;
	box.size.width = (box1.size.width + box2.size.width) / 2;
	box.size.height = (box1.size.height + box2.size.height) / 2;
	box.angle = (box1.angle + box2.angle) / 2;
	return box;
}

/* 圆环检测函数
输入：图片及结构体指针
输出：无
对输入图像进行处理，得到图像中的5个圆环信息保存在结构体中*/

//Circle_msg CircleRecgnize(string filename )
void CircleRecgnize(Mat img, Circle_msg *box_all){
	//vector< vector<Point> > contours;
	vector< vector<Point> > filterContours;
	//vector< Vec4i > hierarchy;

	Mat imag_erc = img.clone();
	int erodeSize = 3;
	int dilateSize = 3;
	Mat elementForErode = getStructuringElement(MORPH_RECT, Size(erodeSize, erodeSize), Point(0, 0));
	Mat elementForDilate = getStructuringElement(MORPH_RECT, Size(dilateSize, dilateSize), Point(0, 0));
	dilate(imag_erc, imag_erc, elementForDilate);

	//提取轮廓得到轮廓向量contours
	Mat src_contour = imag_erc.clone();
	Mat imag_contour = Mat::zeros(src_contour.size(), CV_8UC3);
	blur(src_contour, src_contour, Size(3, 3));
	//cvtColor(src_contour, src_contour, COLOR_BGR2GRAY);
	Canny(src_contour, src_contour, 20, 80, 3, false);
	std::vector<std::vector<Point>> contours;
	std::vector<Vec4i> hierarchy;
	findContours(src_contour, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));//检测轮廓，其中参数4：检测所有轮廓，参数5：仅保存拐点信息

	//删除像素点少的轮廓，去掉伪轮廓
	int _tooLessPixels = 100;
	for (int i = 0; i < (int)contours.size(); i++)
	{
		if (fabs(contourArea(contours[i])) > _tooLessPixels)
		{
			filterContours.push_back(contours[i]);
		}
	}

	//获取围绕面积最大的轮廓
	int max = 0;
	for (int i = 1; i < (int)contours.size(); i++)
	{
		if (fabs(contourArea(contours[max])) < fabs(contourArea(contours[i])))
			max = i;
	}
	RotatedRect box_max = fitEllipse(Mat(contours[max]));

	//对误拟合椭圆过滤，讲椭圆存入数组box_cash[]
	double x_max = (double)box_max.center.x;
	double y_max = (double)box_max.center.y;
	RotatedRect box_cash[100];
	int box_num = 0;
	for (int i = 0; i < (int)filterContours.size(); i++)
	{
		//drawContours(img, filterContours, i, Scalar(0,255,255), 1, 8, hierarchy, 0, Point(0,0));
		RotatedRect box = fitEllipse(Mat(filterContours[i]));
		if ((0.9 < (box.size.height / box.size.width)) && ((box.size.height / box.size.width) < 1.11))
		{
			double x1 = (double)box.center.x;
			double y1 = (double)box.center.y;
			double dis = Cal_Distance(x1, y1, x_max, y_max);
			if (dis < (((double)box_max.size.height) / 10))
			{
				box_cash[box_num] = box;
				box_num++;
				//ellipse(img, box, Scalar(255,255,0), 1, 8);
			}
		}
	}
	//对同圆环的椭圆进行融合得到融合后的椭圆数组box_cash

	int ellipse_num = box_num, ellipse_num1 = box_num;
	for (int i = 0; i < ellipse_num - 1; i++)
	{
		for (int j = i + 1; j < ellipse_num; j++)
		{
			float d = (abs(box_cash[i].size.width*box_cash[i].size.height - box_cash[j].size.width*box_cash[j].size.height)
				/ (box_cash[i].size.width*box_cash[i].size.height));
			if ((d < 0.2) && (d > 0))//面积差比小于0.2，视为同圆环，进行融合
			{
				box_cash[i] = FusionEllipse(box_cash[i], box_cash[j]);
				for (int k = j; k <= ellipse_num; k++)
				{
					box_cash[k] = box_cash[k + 1];
				}
				ellipse_num1--;
				j--;
			}
		}
	}
	//对box_cash进行排序从小到大存储
	RotatedRect box_mid;
	for (int i = 0; i < ellipse_num1 - 1; i++)
	for (int j = i + 1; j < ellipse_num1; j++)
	{
		if (box_cash[i].size.height > box_cash[j].size.height)
		{
			box_mid = box_cash[i];
			box_cash[i] = box_cash[j];
			box_cash[j] = box_mid;
		}
	}
	//计算第四个圆环的参数
	RotatedRect box_3;
	box_3.angle = 2 * box_cash[2].angle - box_cash[1].angle;//椭圆偏转角
	box_3.center.x = 2 * box_cash[2].center.x - box_cash[1].center.x;
	box_3.center.y = 2 * box_cash[2].center.y - box_cash[1].center.y;
	box_3.size.height = 2 * box_cash[2].size.height - box_cash[1].size.height;
	box_3.size.width = 2 * box_cash[2].size.width - box_cash[1].size.width;
	box_cash[3] = box_3;

	//计算第五个圆环的参数
	RotatedRect box_4;
	box_4.angle = 2 * box_cash[3].angle - box_cash[2].angle;//椭圆偏转角
	box_4.center.x = box_cash[3].center.x;
	box_4.center.y = box_cash[3].center.y;
	box_4.size.height = 2 * box_cash[2].size.height - box_cash[0].size.height;
	box_4.size.width = 2 * box_cash[2].size.width - box_cash[0].size.width;
	box_cash[4] = box_4;


	//返回的结构体中保存了五个圆环的信息
	box_all->box1.angle = box_cash[0].angle;
	box_all->box1.center.x = box_cash[0].center.x;
	box_all->box1.center.y = box_cash[0].center.y;
	box_all->box1.size.height = box_cash[0].size.height;
	box_all->box1.size.width = box_cash[0].size.width;

	box_all->box2.angle = box_cash[1].angle;
	box_all->box2.center.x = box_cash[1].center.x;
	box_all->box2.center.y = box_cash[1].center.y;
	box_all->box2.size.height = box_cash[1].size.height;
	box_all->box2.size.width = box_cash[1].size.width;

	box_all->box3.angle = box_cash[2].angle;
	box_all->box3.center.x = box_cash[2].center.x;
	box_all->box3.center.y = box_cash[2].center.y;
	box_all->box3.size.height = box_cash[2].size.height;
	box_all->box3.size.width = box_cash[2].size.width;

	box_all->box4.angle = box_cash[3].angle;
	box_all->box4.center.x = box_cash[3].center.x;
	box_all->box4.center.y = box_cash[3].center.y;
	box_all->box4.size.height = box_cash[3].size.height;
	box_all->box4.size.width = box_cash[3].size.width;

	box_all->box5.angle = box_cash[4].angle;
	box_all->box5.center.x = box_cash[4].center.x;
	box_all->box5.center.y = box_cash[4].center.y;
	box_all->box5.size.height = box_cash[4].size.height;
	box_all->box5.size.width = box_cash[4].size.width;

	filterContours.clear();
	contours.clear();
	hierarchy.clear();
}


float calculElip(float a, float b, float cx, float cy, float x0, float y0){
	float xx, yy;

	xx = (x0 - cx) * (x0 - cx) / a / a;
	yy = (y0 - cy) * (y0 - cy) / b / b;
	return (xx + yy);
}

void calcGun(Mat src, char* camIp){
	
	Mat firstImg;
	Mat thirdImg;
	string sIp = camIp;

	//if (src.size() > 0)
	{
		Circle_msg box_msg3 = {};
		Circle_msg box_msg1 = {};
		thirdImg = src;  // 先把这幅图拿出来求圆形
		//Mat img = cvCreateImage(cvSize(thirdImg.size().width, thirdImg.size().height), IPL_DEPTH_8U, 1);
		//cvCopy(&thirdImg, &img);
		CircleRecgnize(thirdImg, &box_msg3);

		if (firstImgs.count(sIp)){  // 如果已经保存有这个相机的一张图片
			firstImg = firstImgs[sIp];

			float a1, b1, a2, b2, a3, b3, a4, b4, a5, b5;
			float c1x, c1y, c2x, c2y, c3x, c3y, c4x, c4y, c5x, c5y;

			a1 = box_msg3.box1.size.width / 2;
			b1 = box_msg3.box1.size.height / 2;
			c1x = box_msg3.box1.center.x;
			c1y = box_msg3.box1.center.y;

			a2 = box_msg3.box2.size.width / 2;
			b2 = box_msg3.box2.size.height / 2;
			c2x = box_msg3.box2.center.x;
			c2y = box_msg3.box2.center.y;

			a3 = box_msg3.box3.size.width / 2;
			b3 = box_msg3.box3.size.height / 2;
			c3x = box_msg3.box3.center.x;
			c3y = box_msg3.box3.center.y;

			a4 = box_msg3.box4.size.width / 2;
			b4 = box_msg3.box4.size.height / 2;
			c4x = box_msg3.box4.center.x;
			c4y = box_msg3.box4.center.y;

			a5 = box_msg3.box5.size.width / 2;
			b5 = box_msg3.box5.size.height / 2;
			c5x = box_msg3.box5.center.x;
			c5y = box_msg3.box5.center.y;

			box_msg1 = firstImgMsgs[sIp];
			// 计算相互之间的位移
			float imgshiftx = box_msg3.box1.center.x - box_msg1.box1.center.x;
			float imgshifty = box_msg3.box1.center.y - box_msg1.box1.center.y;

			if (abs(imgshiftx) > 0.15 || abs(imgshifty) > 0.15){
				cv::Size dst_sz = thirdImg.size();
				cv::Mat t_mat = cv::Mat::zeros(2, 3, CV_32FC1);
				t_mat.at<float>(0, 0) = 1;
				t_mat.at<float>(0, 2) = box_msg3.box1.center.x - box_msg1.box1.center.x; //水平平移量
				t_mat.at<float>(1, 1) = 1;
				t_mat.at<float>(1, 2) = box_msg3.box1.center.y - box_msg1.box1.center.y; //竖直平移量
				//根据平移矩阵进行仿射变换
				cv::warpAffine(firstImg, firstImg, t_mat, dst_sz);

				// update shot position
				vector<Point>* allgun = &allguns[sIp];
				if (allgun->begin() != allgun->end())
				{
					for (int ii = 0; ii < allgun->size(); ii++)
					{
						(*allgun)[ii].x += (int)imgshiftx;
						(*allgun)[ii].y += (int)imgshifty;
					}
				}
			}

			Mat diffimg;
			diffimg = firstImg - thirdImg;
			diffimg *= 1.6;
			medianBlur(diffimg, diffimg, 5);
			int offsetx = (int)(box_msg3.box5.center.x - box_msg3.box5.size.width / 2 - 10);
			int offsety = (int)(box_msg3.box5.center.y - box_msg3.box5.size.height / 2 - 10);
			int roiw = box_msg3.box5.size.width + 20;
			if (roiw + offsetx > diffimg.size().width - 1)
				roiw = diffimg.size().width - offsetx - 1;
			int roih = box_msg3.box5.size.height + 20;
			if (roih + offsety > diffimg.size().height - 1)
				roih = diffimg.size().height - offsety - 1;

			Mat roi = diffimg(Rect(offsetx, offsety, roiw, roih));
			//int dilateSize = 3;
			//Mat elementForDilate = getStructuringElement(MORPH_CROSS, Size(dilateSize, dilateSize), Point(0, 0));
			//dilate(diffimg, diffimg, elementForDilate);
			int erodeSize = 3;
			Mat elementForErode = getStructuringElement(MORPH_RECT, Size(erodeSize, erodeSize), Point(0, 0));
			erode(roi, roi, elementForErode);
			threshold(roi, roi, 60, 255, 0);
			medianBlur(roi, roi, 3);

#ifdef _AFXDLL
			std::vector<std::vector<cv::Point>> contourst;
			std::vector<cv::Vec4i> hierarchy;
#else
			std::vector<Mat> contourst(100);
			std::vector<Vec4i> hierarchy(100);
#endif
			findContours(roi, contourst, hierarchy, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));
			//vector<RotatedRect> box(contourst.size());
			vector<Rect> box(contourst.size());
			/*
			for (int j = 0; j < contourst.size(); j++)
			{
			box[j] = boundingRect(Mat(contourst[j]));
			Rect tmp;
			box[j].x += offsetx;
			box[j].y += offsety;
			cv::rectangle(img, box[j], Scalar(0, 0, 255),2);
			}
			namedWindow("diff", 0);
			cvResizeWindow("diff", 1400, 700);
			imshow("diff", img);
			waitKey();
			destroyWindow("diff");
			*/
			//Point2f rect[4];
			float distance, area;
			vector<Point>* allgun = &allguns[sIp];
			for (int i = 0; i < contourst.size(); i++)
			{
				//box[i] = minAreaRect(Mat(contourst[i]));
				box[i] = boundingRect(Mat(contourst[i]));
				box[i].x += offsetx;
				box[i].y += offsety;
				float ratio = box[i].width * 1.0f / (box[i].height * 1.0f);
				if (ratio > 0.4 && ratio < 2.2)
				{
					//box[i].points(rect);
					area = box[i].width * box[i].height;
					if (area > 15 && area < 200)
					{
						// calculate the mean in the rect and out of the rect
						Mat mat_means, mat_stddevs;
						int smroix = box[i].x - 5;
						smroix = smroix < 0 ? 0 : smroix;
						int smroiy = box[i].y - 5;
						smroiy = smroiy < 0 ? 0 : smroiy;
						int smroiw = box[i].width + 10;
						int smroih = box[i].height + 10;
						Mat sroi = thirdImg(Rect(smroix, smroiy, smroiw, smroih));
						meanStdDev(sroi, mat_means, mat_stddevs);

						Mat mat_meanl, mat_stddevl;
						int lgroix = smroix - 5;
						lgroix = lgroix < 0 ? 0 : lgroix;
						int lgroiy = smroiy - 5;
						lgroiy = lgroiy < 0 ? 0 : lgroiy;
						int lgroiw = smroiw + 10;
						int lgroih = smroih + 10;
						Mat lroi = thirdImg(Rect(lgroix, lgroiy, lgroiw, lgroih));
						meanStdDev(lroi, mat_meanl, mat_stddevl);

						int talps = smroiw * smroih;
						int talpl = lgroiw * lgroih;

						float exm = (mat_meanl.at<double>(0, 0) * talpl - mat_means.at<double>(0, 0) * talps) / (talpl - talps);

						float meandiff = exm - mat_means.at<double>(0, 0);

						//area = sqrt(powf((rect[0].x - rect[1].x), 2) + powf((rect[0].y - rect[1].y), 2)) *
						//	sqrt(powf((rect[1].x - rect[2].x), 2) + powf((rect[1].y - rect[2].y), 2));

						if (meandiff > 10)
						{
							// check if it not in the vector
							bool isnotin = true;
							for (int jj = 0; jj < allgun->size(); jj++)
							{
								Point newpos;
								newpos.x = box[i].x + box[i].width*1.0f / 2;
								newpos.y = box[i].y + box[i].height*1.0f / 2;
								if (abs((*allgun)[jj].x - newpos.x) < 5.0f &&
									abs((*allgun)[jj].y - newpos.y) < 5.0f)
								{
									isnotin = false;
									break;
								}
							}

							if (isnotin)
							{
								Point gunPos;
								gunPos.x = box[i].x + box[i].width*1.0f / 2;
								gunPos.y = box[i].y + box[i].height*1.0f / 2;
								allgun->push_back(gunPos);
								int resltH = 0;
								float el1, el2, el3, el4, el5;
								el1 = calculElip(a1, b1, c1x, c1y, box[i].x, box[i].y);
								el2 = calculElip(a2, b2, c2x, c2y, box[i].x, box[i].y);
								el3 = calculElip(a3, b3, c3x, c3y, box[i].x, box[i].y);
								el4 = calculElip(a4, b4, c4x, c4y, box[i].x, box[i].y);
								el5 = calculElip(a5, b5, c5x, c5y, box[i].x, box[i].y);

								// 10
								if (el1 <= 1.05)
								{
									scores[sIp].second = 10;
									scores[sIp].first++;
								}
								else if (el1 > 1.05 && el2 <= 1.05)
								{
									scores[sIp].second = 9;
									scores[sIp].first++;
								}
								else if (el2 > 1.05 && el3 <= 1.05)
								{
									scores[sIp].second = 8;
									scores[sIp].first++;
								}
								else if (el3 > 1.05 && el4 <= 1.05)
								{
									scores[sIp].second = 7;
									scores[sIp].first++;
								}
								else if (el4 > 1.05 && el5 <= 1.2)
								{
									scores[sIp].second = 6;
									scores[sIp].first++;
								}
							}
						}
					}
				}
			}
			firstImgs.erase(sIp);
			firstImgs [sIp] = thirdImg; // 更新
		}
		else{
			box_msg1.box1.center.x = box_msg3.box1.center.x;
			box_msg1.box1.center.y = box_msg3.box1.center.y;
			firstImgs[sIp] = thirdImg;
			firstImgMsgs[sIp] = box_msg1;
			pair<int, int> init(0,0);
			scores[sIp] = init;
		}
	}
}

void uploadRslt(Mat src, std::string camIp, int* goal, int* cnt){
	char* ip = (char*)camIp.data();
	//calcGun(src, ip);

	string s = camIp;
	if (scores.count(s)){
		pair<int, int> tmp = scores[s];
		*cnt = tmp.first;
		*goal = tmp.second;
	}

	ip = NULL;
}
