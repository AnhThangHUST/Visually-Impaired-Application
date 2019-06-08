#include "page_dewarp.hpp"
#include "C_preprocess.hpp"
#include <fstream>
#include "stdio.h"
using namespace std;
// enum Action {
//     chup_anh = 0,
//     nghieng_len = 1,
//     nghieng_xuong = 2,
//     nghieng_trai = 3,
//     nghieng_phai = 4,
//     sang_trai = 5,
//     sang_phai = 6,
//     len_tren = 7,
//     xuong_duoi = 8,
//     nang_len = 9,
//     ha_xuong = 10
// };
string paction[] = {"chup_anh", "nghieng_len", "nghieng_xuong", "nghieng_trai",
					"nghieng_phai", "sang_trai", "sang_phai", "len_tren",
					"xuong_duoi", "nang_len", "ha_xuong"};
int main(int argc, char **argv)
{
	ofstream outfile;
	outfile.open("C_PREPROCESS_RESULT.txt", ofstream::out | ofstream::app);

	float width_threshold = 200;
	float height_threshold = 300;
	cv::Mat dst;
	cv::Mat image;
	Action ac, _ac;
	if (argc == 1)
	{
		return 0;
	}

	string filename(argv[1]);
	if (filename.empty())
	{
		//help();
		cout << "Nhap vao anh" << endl;
		return -1;
	}
	image = cv::imread(filename);
	if (image.empty())
	{
		//help();
		cout << "can not open " << filename << endl;
		return -1;
	}
	//cv::cvtColor(image, dst, cv::COLOR_BGR2GRAY);
	//page_dewarp(image.clone(), dst);
	//C_preprocess(dst, dst, ac);
	std::vector<cv::Point2f> point;
	C_preprocess(image, dst, ac, &point);
	outfile << "Action for " << argv[1] << ": " << paction[ac] << endl;
	outfile.close();
	int opt = filename.find("jpg");
	string sub = filename.substr(0, opt);
	cout << "SUB STRING = " << sub << endl;
	cv::imwrite(sub + "_result.png", dst);
	//return 0;
	
	#if 0
	cv::Mat texture_ = cv::Mat::zeros(dst.size(), dst.type());
	std::vector<cv::Point> point2;
	for (int i = 0; i < point.size(); i++)
		point2.push_back(cv::Point((int)point[i].x, (int)point[i].y));
	cv::Point tmp;
	tmp = point2[2];
	point2[2] = point2[3];
	point2[3] = tmp;
	cv::Mat out2;

	cv::fillConvexPoly(texture_,			  //Image to be drawn on
					   point2,				  //C-Style array of points
					   Scalar(255, 255, 255), //Color , BGR form
					   4,					  // connectedness, 4 or 8
					   0);
	cv::bitwise_and(texture_, dst, out2);
	cv::namedWindow("ConvexPoly", cv::WINDOW_NORMAL);
	cv::resize(out2, out2, cv::Size(), 0.25, 0.25);
	cv::imshow("ConvexPoly", out2);
	cv::waitKey(0);
	#endif

#if 1
	if (point.size() == 4) {
		page_dewarp(image, dst, point, "outfile_prefix");

		cv::imwrite(sub + "_dewarp_result.png", dst);
	}
#endif
}