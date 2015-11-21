#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <opencv2/opencv.hpp>
using namespace cv; //move to local namespace
const float eps = 0.00001;

class MOSSE{
public:

//MOSSE(){};
MOSSE(Mat&, cv::Rect);
cv::Point update(Mat&,double rate=0.125);
Mat state_vis();
void draw_state(Mat&);

private:

void preprocess(Mat&);
    double correlate(Mat&,Mat&,cv::Point&);
void update_kernel();
void shiftRows(Mat&);
void shiftRows(Mat&,int);
void shiftCols(Mat&, int);

Size2i size;//size.width size.height
    cv::Point  pos;//pos.x pos.y
    Mat G;
    Mat H1;
    Mat H2;
    Mat H;
    Mat win;
    Mat last_img;
    Mat last_resp;
    double psr;
    bool good=false;

};
