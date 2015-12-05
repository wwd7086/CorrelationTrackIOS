#ifndef MOSSE_H
#define MOSSE_H

#include <opencv2/opencv.hpp>

class MOSSE{
public:
    
    // constructor
    MOSSE(cv::Mat& frame, cv::Rect rect);
    // call it every frame
    void update(cv::Mat& frame, double rate=0.125);
    // return center
    cv::Point getCenter() {
        return pos;
    }
    cv::Rect getRect() {
        return cv::Rect(pos.x-size.width/2,pos.y-size.height/2,
                        size.width,size.height);
    }
    
    // some visualization TODO
    cv::Mat state_vis();
    void draw_state(cv::Mat& vis);
    
private:
    
    void preprocess(cv::Mat& img);
    double correlate(cv::Mat& img, cv::Mat& last_resp, cv::Point& delta_xy);
    void update_kernel();
    void shiftRows(cv::Mat& mat);
    void shiftRows(cv::Mat& mat,int n);
    void shiftCols(cv::Mat& mat, int n);
    
    
    // the size of bounding box
    cv::Size size;
    // the center point of bounding box
    cv::Point pos;
    
    // ground truth response in frequency
    cv::Mat G;
    // filter in frequency
    cv::Mat H1;
    cv::Mat H2;
    cv::Mat H;
    
    // hannng window for preprocessing
    cv::Mat win;
    
    // image insdie previous/cur bounding box
    cv::Mat last_img;
    // response from the image
    cv::Mat last_resp;
    
    // run extra scoring code
    bool isDebug = false;
    // metric for tracking result
    double psr;
    bool isGood=false;
    
    // parameters
    const float eps = 0.00001;
    const float boundaryThre = 0.25;
    const int updateThre = 4;
};

#endif