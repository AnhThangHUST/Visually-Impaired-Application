#include "C_preprocess.hpp"
#include "lbplibrary.hpp"

#define THRESHHOLD 50
#define DEBUG 1
using namespace std;
using namespace cv;
using namespace lbplibrary;
// //DINH NGHIA CAC DONG TAC
// enum Action {   chup_anh,
//                 nghieng_len,
//                 nghieng_xuong,
//                 nghieng_trai,
//                 nghieng_phai,
//                 sang_trai,
//                 sang_phai,
//                 len_tren,
//                 xuong_duoi,
//                 nang_len,
//                 ha_xuong
// };

double area_triangle(double a, double b, double c);

void imageresize(cv::Mat image_in, cv::Mat *image_out);

void enforceContrast(cv::Mat image, cv::Mat *dst, string option = "global");

void enforceThreshold(cv::Mat image, cv::Mat *Threshold);

void smoothImage(cv::Mat image, int kerSize, cv::Mat *dst, string option = "Gausian");

double area_triangle(double a, double b, double c) {
    double s = (a + b + c) / 2;
    s = sqrt(s * (s - a) * (s - b) * (s - c));
    return s;
}

Mat claheGO(Mat src, int _step = 8) {
    Mat CLAHE_GO = src.clone();
    int block = _step; //pblock
    int width = src.cols;
    int height = src.rows;
    int width_block = width / block; //每个小格子的长和宽
    int height_block = height / block;
    //存储各个直方图
    int tmp2[8 * 8][256] = {0};
    float C2[8 * 8][256] = {0.0};
    //分块
    int total = width_block * height_block;
    for (int i = 0; i < block; i++) {
        for (int j = 0; j < block; j++) {
            int start_x = i * width_block;
            int end_x = start_x + width_block;
            int start_y = j * height_block;
            int end_y = start_y + height_block;
            int num = i + block * j;
            //遍历小块,计算直方图
            for (int ii = start_x; ii < end_x; ii++) {
                for (int jj = start_y; jj < end_y; jj++) {
                    int index = src.at<uchar>(jj, ii);
                    tmp2[num][index]++;
                }
            }
            //裁剪和增加操作，也就是clahe中的cl部分
            //这里的参数 对应《Gem》上面 fCliplimit  = 4  , uiNrBins  = 255
            int average = width_block * height_block / 255;
            //关于参数如何选择，需要进行讨论。不同的结果进行讨论
            //关于全局的时候，这里的这个cl如何算，需要进行讨论
            int LIMIT = 40 * average;
            int steal = 0;
            for (int k = 0; k < 256; k++) {
                if (tmp2[num][k] > LIMIT) {
                    steal += tmp2[num][k] - LIMIT;
                    tmp2[num][k] = LIMIT;
                }
            }
            int bonus = steal / 256;
            //hand out the steals averagely
            for (int k = 0; k < 256; k++) {
                tmp2[num][k] += bonus;
            }
            //计算累积分布直方图
            for (int k = 0; k < 256; k++) {
                if (k == 0)
                    C2[num][k] = 1.0f * tmp2[num][k] / total;
                else
                    C2[num][k] = C2[num][k - 1] + 1.0f * tmp2[num][k] / total;
            }
        }
    }
    //计算变换后的像素值
    //根据像素点的位置，选择不同的计算方法
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            //four coners
            if (i <= width_block / 2 && j <= height_block / 2) {
                int num = 0;
                CLAHE_GO.at<uchar>(j, i) = (int) (C2[num][CLAHE_GO.at<uchar>(j, i)] * 255);
            } else if (i <= width_block / 2 && j >= ((block - 1) * height_block + height_block / 2)) {
                int num = block * (block - 1);
                CLAHE_GO.at<uchar>(j, i) = (int) (C2[num][CLAHE_GO.at<uchar>(j, i)] * 255);
            } else if (i >= ((block - 1) * width_block + width_block / 2) && j <= height_block / 2) {
                int num = block - 1;
                CLAHE_GO.at<uchar>(j, i) = (int) (C2[num][CLAHE_GO.at<uchar>(j, i)] * 255);
            } else if (i >= ((block - 1) * width_block + width_block / 2) &&
                       j >= ((block - 1) * height_block + height_block / 2)) {
                int num = block * block - 1;
                CLAHE_GO.at<uchar>(j, i) = (int) (C2[num][CLAHE_GO.at<uchar>(j, i)] * 255);
            }
                //four edges except coners
            else if (i <= width_block / 2) {
                //线性插值
                int num_i = 0;
                int num_j = (j - height_block / 2) / height_block;
                int num1 = num_j * block + num_i;
                int num2 = num1 + block;
                float p = (j - (num_j * height_block + height_block / 2)) / (1.0f * height_block);
                float q = 1 - p;
                CLAHE_GO.at<uchar>(j, i) = (int) (
                        (q * C2[num1][CLAHE_GO.at<uchar>(j, i)] + p * C2[num2][CLAHE_GO.at<uchar>(j, i)]) * 255);
            } else if (i >= ((block - 1) * width_block + width_block / 2)) {
                //线性插值
                int num_i = block - 1;
                int num_j = (j - height_block / 2) / height_block;
                int num1 = num_j * block + num_i;
                int num2 = num1 + block;
                float p = (j - (num_j * height_block + height_block / 2)) / (1.0f * height_block);
                float q = 1 - p;
                CLAHE_GO.at<uchar>(j, i) = (int) (
                        (q * C2[num1][CLAHE_GO.at<uchar>(j, i)] + p * C2[num2][CLAHE_GO.at<uchar>(j, i)]) * 255);
            } else if (j <= height_block / 2) {
                //线性插值
                int num_i = (i - width_block / 2) / width_block;
                int num_j = 0;
                int num1 = num_j * block + num_i;
                int num2 = num1 + 1;
                float p = (i - (num_i * width_block + width_block / 2)) / (1.0f * width_block);
                float q = 1 - p;
                CLAHE_GO.at<uchar>(j, i) = (int) (
                        (q * C2[num1][CLAHE_GO.at<uchar>(j, i)] + p * C2[num2][CLAHE_GO.at<uchar>(j, i)]) * 255);
            } else if (j >= ((block - 1) * height_block + height_block / 2)) {
                //线性插值
                int num_i = (i - width_block / 2) / width_block;
                int num_j = block - 1;
                int num1 = num_j * block + num_i;
                int num2 = num1 + 1;
                float p = (i - (num_i * width_block + width_block / 2)) / (1.0f * width_block);
                float q = 1 - p;
                CLAHE_GO.at<uchar>(j, i) = (int) (
                        (q * C2[num1][CLAHE_GO.at<uchar>(j, i)] + p * C2[num2][CLAHE_GO.at<uchar>(j, i)]) * 255);
            }
                //双线性插值
            else {
                int num_i = (i - width_block / 2) / width_block;
                int num_j = (j - height_block / 2) / height_block;
                int num1 = num_j * block + num_i;
                int num2 = num1 + 1;
                int num3 = num1 + block;
                int num4 = num2 + block;
                float u = (i - (num_i * width_block + width_block / 2)) / (1.0f * width_block);
                float v = (j - (num_j * height_block + height_block / 2)) / (1.0f * height_block);
                CLAHE_GO.at<uchar>(j, i) = (int) ((u * v * C2[num4][CLAHE_GO.at<uchar>(j, i)] +
                                                   (1 - v) * (1 - u) * C2[num1][CLAHE_GO.at<uchar>(j, i)] +
                                                   u * (1 - v) * C2[num2][CLAHE_GO.at<uchar>(j, i)] +
                                                   v * (1 - u) * C2[num3][CLAHE_GO.at<uchar>(j, i)]) *
                                                  255);
            }
            //smooth
            CLAHE_GO.at<uchar>(j, i) =
                    CLAHE_GO.at<uchar>(j, i) + (CLAHE_GO.at<uchar>(j, i) << 8) + (CLAHE_GO.at<uchar>(j, i) << 16);
        }
    }
    return CLAHE_GO;
}

std::vector <cv::Point2f> line_point;

int PreProcess::linear_equation(float a1, float b1, float c1, float a2, float b2, float c2, cv::Point2f *_point) {

    double determinant = a1 * b2 - a2 * b1;
    if (determinant != 0) {
        _point->x = (c1 * b2 - b1 * c2) / determinant;
        _point->y = (a1 * c2 - c1 * a2) / determinant;
        //printf("TEST x = %f, y = %f\n", _point->x, _point->y);
        return 1;
    } else
        return 0;
}

int PreProcess::take_action() {
    return action;
}

cv::Point2f *PreProcess::take_point() {
    return this->point;
}

// HAM XU LY LENH DIEU KHIEN MAY ANH
void PreProcess::EdgeProcess() {
    // CAU LENH IN RA SO CANH (o_O!)
    //printf("Num of Edge = %d\n", numofEdge);
    float point_value;
    cv::Point2f top_left, top_right, bottom_left, bottom_right;
    double top, left, right, bottom, diagonal, deltax, deltay;

    if (numofEdge < 4) {
        //printf("Nang anh len\n");
        PreProcess::action = Action::nang_len;
        return;
    } else {
        // Tim vi tri 4 diem cat trong anh
        linear_equation(cos(original[1]), sin(original[1]), original[0], cos(perpendicular1[1]), sin(perpendicular1[1]),
                        perpendicular1[0], &point[0]);
        linear_equation(cos(original[1]), sin(original[1]), original[0], cos(perpendicular2[1]), sin(perpendicular2[1]),
                        perpendicular2[0], &point[1]);
        linear_equation(cos(parallel[1]), sin(parallel[1]), parallel[0], cos(perpendicular1[1]), sin(perpendicular1[1]),
                        perpendicular1[0], &point[2]);
        linear_equation(cos(parallel[1]), sin(parallel[1]), parallel[0], cos(perpendicular2[1]), sin(perpendicular2[1]),
                        perpendicular2[0], &point[3]);
        sort(point, point + 4, comp);
        if (point[1].x < point[2].x) {
            cv::Point2f tmp;
            tmp = point[1];
            point[1] = point[2];
            point[2] = tmp;
        }
        //printf("Image Size = %d, %d\n", image.size().height, image.size().width);
        // for (int i = 0 ; i < 4; i++) {
        //     printf("x = %f, y = %f\n", point[i].x, point[i].y);
        //     if (point[i].x < 0 || point[i].x > image.size().width){
        //         printf("Nang anh len\n");
        //         return;
        //     }

        //     if (point[i].y < 0 || point[i].y > image.size().height) {
        //         printf("Nang anh len\n");
        //         return;
        //     }

        // }
        top_left = point[0];
        top_right = point[1];
        bottom_left = point[3];
        bottom_right = point[2];
        line_point.push_back(point[0]);
        line_point.push_back(point[1]);
        line_point.push_back(point[2]);
        line_point.push_back(point[3]);
        // Distance
        top = cv::norm(top_right - top_left);
        right = cv::norm(top_right - bottom_right);
        left = cv::norm(top_left - bottom_left);
        bottom = cv::norm(bottom_right - bottom_left);
        diagonal = cv::norm(top_left - bottom_right);

        // TINH DIEN TICH ANH
        double area = area_triangle(top, right, diagonal) + area_triangle(left, bottom, diagonal);
        double image_area = (double) image.size().height * image.size().width;
        // printf("LINES: %lf\n%lf\n%lf\n%lf\n", top, right, left, bottom);
        // printf("area = %lf, Image area = %lf\n", area, image_area);
        deltax = top - bottom;
        deltay = right - left;
        if (deltax > 0 && deltax > top * 1 / 4) {
            //printf("Nghieng len\n");
            PreProcess::action = Action::nghieng_len;
            return;
        } else if (deltax < 0 && abs(deltax) > bottom * 1 / 4) {
            //printf("Nghieng xuong\n");
            PreProcess::action = Action::nghieng_xuong;
            return;
        } else if (deltay > 0 && deltay > right * 1 / 4) {
            //printf("Nghieng phai\n");
            PreProcess::action = Action::nghieng_phai;
            return;
        } else if (deltay < 0 && abs(deltay) > left * 1 / 4) {
            //printf("Nghieng trai\n");
            PreProcess::action = Action::nghieng_trai;
            return;
        }
        if (area > 0.3 * image_area) {

            //printf("chup anh\n");
            PreProcess::action = Action::chup_anh;
            return;
        }
        if (std::max(top_left.y, top_right.y) < 50) {
            //printf("Di may anh len\n");
            PreProcess::action = Action::nang_len;
            return;
        }

        if (std::max(bottom_left.y, bottom_right.y) > image.size().height - 50) {
            //printf("Di may anh xuong\n");
            PreProcess::action = Action::xuong_duoi;
            return;
        }

        if (std::max(bottom_left.x, top_left.x) < 50) {
            //printf("Di may anh sang trai\n");
            PreProcess::action = Action::sang_trai;
            return;
        }

        if (std::max(bottom_right.x, top_right.x) < 50) {
            //printf("Di may anh sang phair\n");
            PreProcess::action = Action::sang_phai;
            return;
        } else {
            //printf("Ha may xuong\n");
            PreProcess::action = Action::ha_xuong;
            return;
        }
    }
}

PreProcess::PreProcess(cv::Mat image, float height_threshold, float width_threshold) {
    this->image = image.clone();
    this->height_threshold = height_threshold;
    this->width_threshold = width_threshold;
    original[0] = parallel[0] = perpendicular1[0] = perpendicular2[0] = -1;
    numofEdge = 0;
    status = charSize = dilation = 0;
};

int PreProcess::CharSize(char *image) {
    return charSize;
}

float PreProcess::morphological(int charSize) {
    int kerSize = int(charSize / 2);
    if (DEBUG)
        printf("%f\n", kerSize);
    char kernel[charSize][kerSize];
}

void PreProcess::detectEdges(vector <cv::Vec2f> lines) {
    if (lines.size() == 0) {
        status = 0;
        return;
    }
    int re = 0;
    retry:
    numofEdge = 1;
    this->original = lines[0]; // original lay la canh dau tien
//    rec_lines.push_back(original);
//    return;
    if (lines.size() == 1) {
        //printf("Just Detect one line");
        //numofEdge = 1;
        return;
    }
    cv::Mat itmp = image.clone();
    for (size_t i = 0; i < lines.size(); i++) {
        float rho = lines[i][0], theta = lines[i][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;
        pt1.x = cvRound(x0 + 1000 * (-b));
        pt1.y = cvRound(y0 + 1000 * (a));
        pt2.x = cvRound(x0 - 1000 * (-b));
        pt2.y = cvRound(y0 - 1000 * (a));
        line(itmp, pt1, pt2, Scalar(0, 0, 255), 3, 1);
    }
    //imshow("source", src);
    cv::resize(itmp, itmp, cv::Size(), 0.25, 0.25);
    imshow("detected lines", itmp);

    // waitKey();

    //goto end;
    float rho0 = this->original[0];
    float theta0 = this->original[1];
    double tmp1, tmp2;
    for (int i = 1; i < lines.size(); i++) {
        if (numofEdge == 4)
            break;
        float rho = lines[i][0], theta = lines[i][1];
        float delta = abs(theta - theta0);
        if (this->parallel[0] == -1 || numofEdge == 4) {
            // printf("1111 rho = %f,  rho0 = %f\n", rho, rho0)
            if ((theta + theta0 - 2 * M_PI) < angle_threshold && rho * rho0 < 0)
                if (abs(abs(rho0) - abs(rho)) < 30)
                    continue;
            if (abs(rho - rho0) > width_threshold)
                if (delta < this->angle_threshold || abs(delta - M_PI) < this->angle_threshold ||
                    abs(delta - 2 * M_PI) < this->angle_threshold) {
                    //       printf("DELTAAAA rho = %f,  rho0 = %f\n", rho, rho0);
                    //     printf("DELTAAAA angle = %f,  angle0 = %f\n", theta, theta0);
                    if (numofEdge < 4) {
                        this->parallel = lines[i];
                        numofEdge += 1;
                    }
//                    else
//                    {
//                        tmp1 = abs(rho - original[0]);
//                        tmp2 = abs(rho - parallel[0]);
//                        if ( tmp1 > width_threshold && tmp1 < abs(original[0] - parallel[0]))
//                            parallel = lines[i];
//                        else if ( tmp2 > width_threshold && tmp2 < abs(original[0] - parallel[0]))
//                            original = lines[i];
////                        if (abs(rho - rho0) < abs(rho0 - parallel[0]))
////                            this->parallel = lines[i];
//                    }
//

                    continue;
                }
        }
        if (abs(delta - M_PI / 2) < this->angle_threshold) {
            if (this->perpendicular1[0] == -1) {
                this->perpendicular1 = lines[i];
                numofEdge += 1;
            } else if (this->perpendicular2[0] == -1) {
                if (abs(perpendicular1[1] + theta - 2 * M_PI) < angle_threshold && rho * this->perpendicular1[0] < 0)
                    if (abs(abs(rho) - abs(this->perpendicular1[0])) < 30)
                        continue;
                float height = abs(rho - this->perpendicular1[0]);
                if (height > height_threshold) {

                    this->perpendicular2 = lines[i];
                    numofEdge += 1;
                }
            }
//            else
//            {
//                if (abs(perpendicular1[1] + theta - 2 * M_PI) < angle_threshold && rho * this->perpendicular1[0] < 0)
//                {
//                    tmp2 = abs(abs(rho) - abs(perpendicular2[0]));
//                    if ( tmp2 > height_threshold && tmp2 < abs(abs(perpendicular1[0]) - abs(perpendicular2[0])))
//                        perpendicular1 = lines[i];
//                }
//                if (abs(perpendicular2[1] + theta - 2 * M_PI) < angle_threshold && rho * this->perpendicular2[0] < 0)
//                {
//                    tmp1 = abs(abs(rho) - abs(perpendicular1[0]));
//                    if ( tmp1 > height_threshold && tmp1 < abs(abs(perpendicular2[0]) - abs(perpendicular1[0])))
//                        perpendicular2 = lines[i];
//                }
//                tmp1 = abs(rho - this->perpendicular1[0]);
//                tmp2 = abs(rho - this->perpendicular2[0]);
//                if ( tmp1 > height_threshold && tmp1 < abs(perpendicular1[0] - perpendicular2[0]))
//                    perpendicular2 = lines[i];
//                else if ( tmp2 > height_threshold && tmp2 < abs(perpendicular1[0] - perpendicular2[0]))
//                    perpendicular1 = lines[i];
//            }
        }
    }

    if (numofEdge < 4 && re == 0) {
        re = 1;
        height_threshold = width_threshold = 100;
        goto retry;
    }
    if (numofEdge == 4) {
        for (int i = 1; i < lines.size(); i++) {
            float rho = lines[i][0], theta = lines[i][1];
            float delta = abs(theta - original[1]);
            if (delta < this->angle_threshold || abs(delta - M_PI) < this->angle_threshold ||
                abs(delta - 2 * M_PI) < this->angle_threshold) {
//                if ((theta + theta0 - 2 *M_PI) < angle_threshold && rho*rho0 < 0)
//                    if (abs(abs(rho0) - abs(rho)) < 30)
//                        continue;
                if (abs(rho - original[0]) > width_threshold && abs(rho - original[0]) < abs(original[0] - parallel[0]))
                    parallel = lines[i];
                else if (abs(rho - parallel[0]) > width_threshold &&
                         abs(rho - parallel[0]) < abs(original[0] - parallel[0]))
                    original = lines[i];
                continue;
            } else if(abs(delta - M_PI / 2) < this->angle_threshold)
            {
                if (abs(rho - perpendicular1[0]) > height_threshold &&
                    abs(rho - perpendicular1[0]) < abs(perpendicular1[0] - perpendicular2[0]))
                    perpendicular2 = lines[i];
                else if (abs(rho - perpendicular2[0]) > height_threshold &&
                         abs(rho - perpendicular2[0]) < abs(perpendicular1[0] - perpendicular2[0]))
                    perpendicular1 = lines[i];
                continue;
            }
        }
    }
    rec_lines.push_back(original);
    rec_lines.push_back(parallel);
    rec_lines.push_back(perpendicular1);
    rec_lines.push_back(perpendicular2);
}

cv::Vec2f twoPoints2Polar(const cv::Vec4i &line) {
    // Get points from the vector
    cv::Point2f p1(line[0], line[1]);
    cv::Point2f p2(line[2], line[3]);

    // Compute 'rho' and 'theta'
    float rho = abs(p2.x * p1.y - p2.y * p1.x) / cv::norm(p2 - p1);
    float theta = -atan2((p2.x - p1.x), (p2.y - p1.y));

    // You can have a negative distance from the center
    // when the angle is negative
    if (theta < 0) {
        rho = -rho;
    }

    return cv::Vec2f(rho, theta);
}

void PreProcess::process() {
    cv::Mat candy_img, dilation_dst, gray, gray_in, dst;
    vector <cv::Vec2f> lines1;
    vector <Vec4i> lines;
    //image = claheGO(image);
    boundingbox(image, lines1);
    cv::Mat kernel = getStructuringElement(cv::MORPH_RECT,
                                           cv::Size(charSize, charSize));
    cv::cvtColor(image, gray_in, cv::COLOR_BGR2GRAY);
    //cv::GaussianBlur( gray_in, gray, Size( 19, 19), 0, 0 );
    //cv::medianBlur(gray_in, gray, 11);
    //cv::blur( gray_in, gray, Size( 19, 19), Point(-1,-1) );

    dilate(gray_in, dilation_dst, kernel, cv::Point(-1, -1), 1);
    if (DEBUG) {
        cv::Mat debug;
        cv::namedWindow("Dilation window", cv::WINDOW_NORMAL);
        cv::resize(dilation_dst, debug, cv::Size(), 0.25, 0.25);
        cv::imshow("Dilation window", debug);
        cv::imwrite("dialtion.png", dilation_dst);
        cv::waitKey(0);
    }

    //cv::cvtColor(dilation_dst, gray, cv::COLOR_BGR2GRAY);
    //smoothImage(dilation_dst, PreProcess::charSize, &dst);
    enforceContrast(dilation_dst, &dst, "local");
    //smoothImage(dst, PreProcess::charSize, &dst);
    smoothImage(dilation_dst, 4, &dst);
    //enforceThreshold(dst, &dst);
    cv::Canny(dst, candy_img, 15, 40, 3, true);
    if (DEBUG) {
        cv::Mat candy;
        cv::imwrite("candy.png", candy_img);
        cv::resize(candy_img, candy, cv::Size(), 0.5, 0.5);
        cv::namedWindow("Display Candy", cv::WINDOW_NORMAL);
        cv::imshow("Display Candy", candy);
        cv::waitKey(0);
    }
    //cvtColor(candy_img, candy_img, cv::COLOR_GRAY2BGR);
    //candy_img.convertTo(candy_img, CV_8UC1);
    //cv::HoughLinesP(candy_img, lines, 1, M_PI / 180, 65, 30, 30);
    cv::HoughLines(candy_img, lines1, 1, M_PI / 180, 60);
    //vector<cv::Vec2f> lines2;
    // for (int i = 0; i < lines.size(); i++)
    //     lines2.push_back(twoPoints2Polar(lines[i]));
    // printf("So line tim duoc la: %d\n", lines.size());
    detectEdges(lines1);
    if (1) {
        //printlines();
        showImageWithLine();
    }

    EdgeProcess();
}

void PreProcess::printlines() {
    printf("%.2f, %.2f \n", original[0], original[1]);
    printf("%.2f, %.2f\n", parallel[0], parallel[1]);
    printf("%f, %.2f\n", perpendicular1[0], perpendicular1[1]);
    printf("%.2f, %.2f\n", perpendicular2[0], perpendicular2[1]);
}

void PreProcess::showImageWithLine() {
    color_dst = image.clone();
    for (size_t i = 0; i < rec_lines.size(); i++) {
        float rho = rec_lines[i][0];
        float theta = rec_lines[i][1];
        // printf("rho = %.2f, theta = %.2f", rho, theta);
        if (rho == -1)
            continue;
        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;
        cv::Point pt1(cvRound(x0 + 1000 * (-b)),
                      cvRound(y0 + 1000 * (a)));
        cv::Point pt2(cvRound(x0 - 1000 * (-b)),
                      cvRound(y0 - 1000 * (a)));
        cv::line(color_dst, pt1, pt2, cv::Scalar(0, 0, 255), 3, 8);
    }
    if (color_dst.empty()) {
        //printf("NULLLLLL");
        return;
    }
    cv::Mat debug;
    cv::resize(color_dst, debug, cv::Size(), 0.25, 0.25);
    if (DEBUG) {
        cv::namedWindow("Detected Lines", 1);
        cv::imshow("Detected Lines", debug);
        cv::waitKey(0);
        cv::imwrite("Deteced_lines.jpg", debug);
    }
}

void PreProcess::boundingbox(cv::Mat src, vector <cv::Vec2f> lines) {

    vector <vector<cv::Point>> contours;
    RNG rng(12345);
    vector <cv::Vec4i> hierarchy;
    cv::Mat candy_img, gray;
    //cv::cvtColor(src, gray, cv::COLOR_GRAY2BGR);
    //cv::medianBlur(src, src, 11);
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    cv::Canny(gray, candy_img, 50, 100, 3, true);
    cv::findContours(candy_img, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    vector <vector<Point>> contours_poly(contours.size());
    vector <Rect> boundRect(contours.size());
    //vector<Point2f>center( contours.size() );
    //vector<float>radius( contours.size() );
    std::vector<int> height_list;
    for (int i = 0; i < contours.size(); i++) {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
        boundRect[i] = boundingRect(Mat(contours_poly[i]));
        //minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
    }

    for (int i = 0; i < boundRect.size(); i++) {
        int x = boundRect[i].x;
        int y = boundRect[i].y;
        int width = boundRect[i].width;
        int height = boundRect[i].height;
        //printf("height: %d\n", height);
        if (height < 25)
            height_list.push_back(height);
    }
    //std::cout << height_list << endl;
    // for (int i = 0; i < height_list.size(); i++)
    // {
    //     cout << height_list[i] << " ";
    // }
    // cout << endl;
    double sum = 0;
    double sum2 = 0;
    for (unsigned int i = 0; i < height_list.size(); i++) {
        sum += height_list[i];
    }
    double mean = sum / height_list.size();
    for (unsigned int i = 0; i < height_list.size(); i++) {
        sum2 += pow((mean - height_list[i]), 2);
    }
    //double sum = std::accumulate(height_list.begin(), height_list.end(), 0.0);
    //double mean = sum / height_list.size();
    // double accum = 0.0;
    // std::for_each (std::begin(height_list), std::end(height_list), [&](const double d) {
    //     accum += (d - mean) * (d - mean);
    // });

    double stdev = sqrt(sum2 / (height_list.size() - 1));
    // printf("mean = %lf, std = %lf\n", mean, stdev);
    int count = 0;
    int char_size = 0;
    cv::Vec2i point;
    for (int i = 0; i < boundRect.size(); i++) {
        if (boundRect[i].height > (mean - 0.2 * stdev) && (boundRect[i].height < (mean + 0.2 * stdev))) {
            char_size += boundRect[i].height;
            count += 1;
            point[0] = boundRect[i].x;
            point[1] = boundRect[i].y;
            point_list.push_back(point);
        } else {
            boundRect[i].height = 0;
            boundRect[i].width = 0;
        }
    }

    char_size = (int) char_size / count;

    // if (charSize < 10)
    //     this->charSize = 5;
    // else
    //     this->charSize = (int) char_size/2;
    this->charSize = (int) char_size;
    //printf("Kernel Size = %d\n", char_size);
    //charSize = 6;
    //printf("PUSSHHHHHHHHH");
    //std::cout << '\n";
    //printf("PUSSHHHHHHHHH");
    Mat drawing = Mat::zeros(src.size(), CV_8UC3);
    for (int i = 0; i < contours.size(); i++) {
        Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
        drawContours(drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());
        if (boundRect[i].height == 0)
            //printf("printab");
            continue;
        rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
        //circle( drawing, center[i], (int)radius[i], color, 2, 8, 0 );
    }

    /// Show in a window
    if (DEBUG) {
        namedWindow("Contours", WINDOW_AUTOSIZE);
        cv::imwrite("contours.png", drawing);
        cv::resize(drawing, drawing, cv::Size(), 0.25, 0.25);
        imshow("Contours", drawing);
        waitKey(0);
    }
}

void help() {
    cout << "\n Super app detect line.\n"
            "Usage:\n"
            "./C_preprocess <image_name>\n"
         << endl;
}

void imageresize(cv::Mat image_in, cv::Mat *image_out) {
    int height = image_in.size().height;
    int width = image_in.size().width;

    if (height > width)
        cv::resize(image_in, *image_out, cv::Size(1000, 1500));
    else
        cv::resize(image_in, *image_out, cv::Size(1500, 1000));
}

void resize_to_screen1(cv::Mat src, cv::Mat *dst, int max_width = 1280, int max_height = 720) {
    int width = src.size().width;
    int height = src.size().height;

    double scale_x = double(width) / max_width;
    double scale_y = double(height) / max_height;

    int scale = (int) ceil(scale_x > scale_y ? scale_x : scale_y);

    if (scale > 1) {
        double invert_scale = 1 / (double) scale;
        printf("%f", invert_scale);
        cv::resize(src, *dst, cv::Size(0, 0), invert_scale, invert_scale);
    } else {
        *dst = src.clone();
    }
}

void enforceThreshold(cv::Mat image, cv::Mat *Threshold) {
    cv::threshold(image, *Threshold, 50, 255, cv::THRESH_TOZERO);
}

void enforceContrast(cv::Mat image, cv::Mat *dst, string option) {
    std::string local = "local";
    image.convertTo(image, CV_8UC1);
    // printf("SAI O DAYYYYYYYY");
    if (option.compare(local) == 0) {
        // cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0);
        // clahe->setClipLimit(4);
        // clahe->apply(image, *dst);
        *dst = claheGO(image);
    } else
        cv::equalizeHist(image, *dst);
    //cv::equalizeHist(image, *dst);
    // cv::Mat frame, img_lbp;
    // LBP *lbp;
    // // lbp = new VARLBP;
    // // lbp = new CSLBP;
    // // lbp = new CSLDP;
    // //lbp = new XCSLBP;
    // // lbp = new SILTP;
    // // lbp = new CSSILTP;
    // // lbp = new SCSLBP;
    // lbp = new BGLBP;
    // lbp->run(image, img_lbp);
    // cv::normalize(img_lbp, *dst, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    //printf("ENDDDDDDDDDDDDDDDDDD");
}

void smoothImage(cv::Mat image, int kerSize, cv::Mat *dst, string option) {
    string str = "Average";
    if (kerSize % 2 == 0)
        kerSize == kerSize - 1;
    if (str.compare(str) == 0)
        cv::blur(image, *dst, cv::Size(kerSize, kerSize));
    else
        cv::GaussianBlur(image, *dst, cv::Size(kerSize, kerSize), 2);
}

#if 0
int main( int argc, char** argv ) {
    clock_t start = clock();
    float width_threshold = 200;
    float height_threshold = 300;
    cv::Mat dst;
    cv::Mat image, image_resize;
    if (argc == 1) {
        help();
        return 0;
    }
    
    string filename = argv[1];
    if (filename.empty()) {
        help();
        cout << "Nhap vao anh" << endl;
        return -1;
    }
    image = cv::imread(filename, 0);
    if(image.empty()) {
        help();
        cout << "can not open " << filename << endl;
        return -1;
    }
    // Khoi tao
    //cv::cvtColor(image, dst, cv::COLOR_GRAY2BGR);

    imageresize(image, &image_resize);

    PreProcess image_process(image_resize, width_threshold, height_threshold);
    image_process.process();
    printf("Time: %.2fs\n", (double)(clock() - start)/CLOCKS_PER_SEC);
    Action a = chup_anh;
    printf("%d Tung loz",a);
}

#else

int C_preprocess(cv::Mat src, cv::Mat &dst, Action &ac, std::vector <cv::Point2f> *point) {
    float width_threshold = 150;
    float height_threshold = 250;
    cv::Mat image_resize;
    resize_to_screen1(src, &image_resize);
    printf("image_resize.height = %d\n", image_resize.size().height);
    printf("image_resize.width = %d\n", image_resize.size().width);
    PreProcess image_process(image_resize, width_threshold, height_threshold);
    image_process.process();
    ac = image_process.action;
    dst = image_process.color_dst.clone();
    *point = line_point;
}

#endif
