#ifndef MOSSE_H
#define MOSSE_H

#include <opencv2/opencv.hpp>
#include <Accelerate/Accelerate.h>

class MOSSE{
public:
    
    // constructor
    MOSSE(cv::Mat& frame, cv::Rect rect);
    ~MOSSE();
    
    // call it every frame
    void update(cv::Mat& frame, float rate=0.125);
    
    // return center of bounding box
    cv::Point getCenter() {
        return pos;
    }
    // retrun bounding box
    cv::Rect getRect() {
        return cv::Rect(pos.x-size.width/2,pos.y-size.height/2,
                        size.width,size.height);
    }
    
private:
    
    void preprocess(cv::Mat& img);
    double correlate(cv::Mat& img, cv::Point& delta_xy);
    void update_kernel();
    
    float* createDSPSplitComplex(DSPSplitComplex& splitComplex);
    void doFFT(cv::Mat& mat, DSPSplitComplex* splitComplex);
    void doMulti(DSPSplitComplex* a, DSPSplitComplex* b, DSPSplitComplex* c);
    void doDivide(DSPSplitComplex* a, DSPSplitComplex* b, DSPSplitComplex* c);
    void doScale(DSPSplitComplex* a, float scale);

    // the size of bounding box
    cv::Size size;
    int wlog2;
    int hlog2;
    unsigned long totalSize;
    // the center point of bounding box
    cv::Point pos;
    
    // ground truth response
    DSPSplitComplex G;
    float* G_M;
    // filter
    DSPSplitComplex H1;
    float* H1_M;
    DSPSplitComplex H2;
    float* H2_M;
    DSPSplitComplex H;
    float* H_M;
    // temp filter
    DSPSplitComplex H1t;
    float* H1t_M;
    DSPSplitComplex H2t;
    float* H2t_M;
    // temp sub image
    DSPSplitComplex I;
    float* I_M;
    // temp filter response
    DSPSplitComplex R;
    float* R_M;
    
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
    
    // FFT vdsp
    FFTSetup FFTSetup;
};

#endif