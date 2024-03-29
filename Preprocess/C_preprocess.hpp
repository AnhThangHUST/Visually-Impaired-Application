//
// Created by ykuta on 02-Mar-19.
//
// #ifndef BITMAP
// #define BITMAP
// #endif
#ifndef READINGASSISTANCE_NATIVE_LIB_H
#define READINGASSISTANCE_NATIVE_LIB_H
// #define JNIIMPORT
// #define JNIEXPORT  __attribute__ ((visibility ("default")))
// #define JNICALL

#include <iostream>
#include <math.h>
#include <string.h>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include <numeric>
#include <random>
#include <algorithm>
#include <time.h>

using namespace std;
using namespace cv;

static const int MEMORY_OK = 0;
static const int UCHAR_ARRAY_ERROR = 3;
enum Action {
    chup_anh = 0,
    nghieng_len = 1,
    nghieng_xuong = 2,
    nghieng_trai = 3,
    nghieng_phai = 4,
    sang_trai = 5,
    sang_phai = 6,
    len_tren = 7,
    xuong_duoi = 8,
    nang_len = 9,
    ha_xuong = 10
};

typedef struct {
    float cropBounds[4]; //left, top, right, bottom
    unsigned char* transforms;
    int size;
} TransformList;

typedef struct {
    unsigned int width;
    unsigned int height;

    unsigned int redWidth;
    unsigned int redHeight;
    unsigned int greenWidth;
    unsigned int greenHeight;
    unsigned int blueWidth;
    unsigned int blueHeight;

    unsigned char* red;
    unsigned char* green;
    unsigned char* blue;

    TransformList transformList;
} Bitmap;

// double area_triangle(double a, double b, double c);

// void imageresize (cv::Mat image_in, cv::Mat *image_out);

// void enforceContrast(cv::Mat image, cv::Mat &dst, string option="global");

// void enforceThreshold(cv::Mat image, cv::Mat *Threshold);

// void smoothImage(cv::Mat image, int kerSize, cv::Mat *dst, string option = "Gausian");

// double area_triangle(double a, double b, double c) {
//     double s = (a + b + c)/2;
//     s = sqrt(s * (s - a) * (s - b) * (s - c));
//     return s;
// }
void help();
int C_preprocess(cv::Mat src, cv::Mat &dst, Action &ac, std::vector <cv::Point2f> *point);

class PreProcess {
public:
    PreProcess(cv::Mat image, float height_threshold, float width_threshold);
    int CharSize(char *image);
    float morphological(int charSize);
    void detectFromOrigin();
    void detectEdges(std::vector<cv::Vec2f> lines);
    void rotate(char *image);
    void showImageWithLine();
    void process();
    void printlines();
    void EdgeProcess();
    void boundingbox(cv::Mat src, std::vector <cv::Vec2f> lines);
    int linear_equation(float a1, float b1, float c1, float a2, float b2, float c2, cv::Point2f *_point);
    int take_action();
    cv::Point2f* take_point();

public:
    int numofEdge;
    cv::Mat image, color_dst;
    int status;
    float dilation;
    int charSize;
    cv::Vec2f original; // First detected edge
    cv::Vec2f parallel; // Edge that is parallel to original
    cv::Vec2f perpendicular1, perpendicular2; // Edges that are perpendicular to original
    float angle_threshold = M_PI/10.0;
    float height_threshold, width_threshold;
    std::vector<cv::Vec2i> point_list;
    std::vector<cv::Vec2f> rec_lines;
    cv::Point2f point[4];
    Action action;

    struct str {
        bool operator() ( Point2f a, Point2f b ){
            if ( a.y != b.y )
                return a.y < b.y;
            return a.x <= b.x ;
        }
    } comp;
};


#endif //READINGASSISTANCE_NATIVE_LIB_H