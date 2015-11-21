
#include <opencv2/opencv.hpp>

const float eps = 0.00001;

class MOSSE{
public:
    
    // constructor
    MOSSE(cv::Mat& frame, cv::Rect rect);
    // call it every frame
    cv::Point update(cv::Mat& frame, double rate=0.125);
    // return center
    cv::Point getCenter() {
        return pos;
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
    // metric for tracking result
    double psr;
    bool isGood=false;
};
