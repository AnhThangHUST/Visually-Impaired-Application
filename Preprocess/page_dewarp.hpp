#ifndef PAGE_DEWARP_HPP
#define PAGE_DEWARP_HPP

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>


#define PAGE_MARGIN_X 50 // reduced px to ignore near L/R edge
#define PAGE_MARGIN_Y 20 //reduced px to ignore near T/B edge

#define OUTPUT_ZOOM 1.0   // how much to zoom output relative to *original* image
#define OUTPUT_DPI 300    // just affects stated DPI of PNG, not appearance
#define REMAP_DECIMATE 16 // downscaling factor for remapping image

#define ADAPTIVE_WINSZ 55 // window size for adaptive threshold in reduced px

#define TEXT_MIN_WIDTH 15     // min reduced px width of detected text contour
#define TEXT_MIN_HEIGHT 2     // min reduced px height of detected text contour
#define TEXT_MIN_ASPECT 1.5   // filter out text contours below this w/h ratio
#define TEXT_MAX_THICKNESS 10 // max reduced px thickness of detected text contour

#define EDGE_MAX_OVERLAP 1.0  // max reduced px horiz. overlap of contours in span
#define EDGE_MAX_LENGTH 100.0 // max reduced px length of edge connecting contours
#define EDGE_ANGLE_COST 10.0  // cost of angles in edges (tradeoff vs. length)
#define EDGE_MAX_ANGLE 7.5    // maximum change in angle allowed between contours

#define RVEC_IDX(src) slice(src, 0, 3)  // index of rvec in params vector
#define TVEC_IDX(src) slice(src, 3, 6)  // index of tvec in params vector
#define CUBIC_IDX(src) slice(src, 6, 8) // index of cubic slopes in params vector
#define slice(src, a, b) src.begin() + a, src.begin() + b - 1

#define SPAN_MIN_WIDTH 60   // minimum reduced px width for span
#define SPAN_PX_PER_STEP 40 // reduced px spacing for sampling along spans
#define FOCAL_LENGTH 1.2    // normalized focal length of camera

#define DEBUG_LEVEL 0      // 0=none, 1=some, 2=lots, 3=all
#define DEBUG_OUTPUT "file" // file, screen, both

#define WINDOW_NAME "Dewarp" // Window name for visualization
#define MASK_TYPE_TEXT 0
using namespace cv;
using namespace std;


// void project_keypoints(const std::vector<double> &pvec, std::vector<int> *keypoint_index, std::vector<cv::Point2d> *imagepoints);
// void get_page_extents(cv::Mat small, cv::Mat page_mask, std::vector<cv::Point> *page_outline);
// std::vector<cv::Point2d> norm2pix(cv::Size s, std::vector<cv::Point2d> pts, bool as_integer);
// void get_mask(std::string name, cv::Mat small, cv::Mat page_mask, int mask_type, cv::Mat *mask);
// void blob_mean_and_tangent(std::vector<cv::Point> contour, double *center, double *tangent);
// void pix2norm(cv::Size s, std::vector<cv::Point> pts);
// void get_default_params(std::vector<cv::Point> corners, std::vector<double> ycoords, std::vector<std::vector<double>> xcoords, 
//                         double *rough_dims, std::vector<int> *span_counts, std::vector<double> *params);
// void polyval(std::vector<double> poly, std::vector<cv::Point2d> xy_coords, std::vector<cv::Point3d> *objpoints);
// void project_xy(std::vector<cv::Point2d> &xy_coords, std::vector<double> pvec, std::vector<cv::Point2d> *imagepoints);
// double be_like_target1(const column_vector &x);

//void debug_show(char *name, int step, char *text, cv::Mat display);
int page_dewarp(cv::Mat img_src, cv::Mat &img_dst, std::vector <cv::Point2f> line_point, string outfile_prefix);


class Dewarp
{

  public:
    std::vector<cv::Point2d> keypoint;
    
    int round_nearest_multiple(int i, int factor);
    int pix2norm(cv::MatSize shape, cv::Mat pts);

  private:
    std::vector<int> *keypoint_index;
};

class Optimize
{
  public:
    void make_keypoint_index(std::vector<int> span_counts);
    //static double be_like_target(const column_vector &x);
  
    double Minimize(std::vector<double> params);
    double get_page_dims(std::vector<double> params);
  
    std::vector<int> span_counts;
    
    void remap_image(string name, cv::Mat img, cv::Mat small, cv::Mat &thresh, std::vector<double> page_dims, std::vector<double> params, string outfile_prefix);

    
};

class ContourInfo
{
  private:
  public:
    double point0[2];
    double point1[2];
    double center[2];
    double angle;
    ContourInfo *pred, *succ;
    double local_xrng[2];
    std::vector<cv::Point> contour;
    cv::Rect rect;
    cv::Mat mask;
    double tangent[2];
    double interval_measure_overlap(double *int_a, double *int_b);

    ContourInfo()
    {
        pred = NULL;
        succ = NULL;
    }
    ContourInfo(std::vector<cv::Point> c, cv::Rect r, cv::Mat m);

    bool operator == (const ContourInfo &ci0) const
    {
        return (
          point0[0] == ci0.point0[0] &&
          point0[1] == ci0.point0[1] &&
          point1[0] == ci0.point1[0] &&
          point1[1] == ci0.point1[1] &&
          center[0] == ci0.center[0] &&
          center[1] == ci0.center[1] &&
          angle == ci0.angle &&
          local_xrng[0] == ci0.local_xrng[0] &&
          local_xrng[1] == ci0.local_xrng[1] &&
          contour.size() == ci0.contour.size()          
        );
    }
    // sai tai Thang loz
    // ContourInfo(const ContourInfo& ci) {
    //   this = &ci;
    // }

    double project_x(cv::Point point)
    {
        return (this->tangent[0] * (point.x - this->center[0]) + this->tangent[1] * (point.y - this->center[1]));
    }

    double local_overlap(ContourInfo other)
    {
        double int_b[2];
        int_b[0] = project_x(cv::Point(other.point0[0], other.point0[1]));
        int_b[1] = project_x(cv::Point(other.point1[0], other.point1[1]));
        return interval_measure_overlap(this->local_xrng, int_b);
    }

    static bool sortContourInfo(ContourInfo ci0, ContourInfo ci1)
    {
        return ci0.rect.y < ci1.rect.y;
    }
};


class Edge
{
  public:
    double score;
    ContourInfo *cinfo_a;
    ContourInfo *cinfo_b;

    Edge()
    {
        score = 0;
    }

    Edge(double s, ContourInfo *ci_a, ContourInfo *ci_b)
    {
        this->score = s;
        this->cinfo_a = ci_a;
        this->cinfo_b = ci_b;
    }

    static bool sortEdge(Edge e0, Edge e1)
    {
        return e0.score < e1.score;
    }
};



#endif
