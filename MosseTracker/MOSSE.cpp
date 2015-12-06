
#include "MOSSE.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>

using namespace cv;
using namespace std;

// return random value from 0-1
float randNum(){
    return ((float)rand()) / RAND_MAX;
}

// random smaller affine pertubation
Mat rnd_warp(cv::Mat& a){

    // affine warp matrix
    Mat T= Mat::zeros(2,3,CV_32F);
    
    // random rotation
    double coef=0.2;
    double ang=(randNum()-0.5)*coef; //-0.1~0.1
    double c=cos(ang); double s=sin(ang);
    T.at<float>(0,0) = c + (randNum()-0.5)*coef;
    T.at<float>(0,1) = -s + (randNum()-0.5)*coef;
    T.at<float>(1,0) = s + (randNum()-0.5)*coef;
    T.at<float>(1,1) = c + (randNum()-0.5)*coef;
    
    // random translation
    int h=a.size().height;
    int w=a.size().width;
    Mat center = Mat(2, 1, CV_32F);
    center.at<float>(0,0) = w/2;
    center.at<float>(1,0) = h/2;
    T.col(2) = center - (T.colRange(0, 2))*center;
    
    // do the warpping
    //cout<<T<<endl;
    Mat warped;
    warpAffine(a, warped, T, a.size(), BORDER_REFLECT);
    //cout<<warped<<endl;
    return warped;
}

// element division between two complex matrix
Mat divSpec(Mat& A,Mat& B, bool conj=true){
    
    vector<Mat> Ari(2),Bri(2);
    cv::split(A,Ari);
    cv::split(B,Bri);
    
    // compute elmenet by elment
    Mat Cr = Mat::zeros(A.size(),CV_32F);
    Mat Ci = Mat::zeros(A.size(),CV_32F);
    for(int i=0;i<A.rows; i++) {
        for(int j=0;j<A.cols; j++) {
            float ar = Ari[0].at<float>(i,j);
            float ai = Ari[1].at<float>(i,j);
            float br = Bri[0].at<float>(i,j);
            float bi = Bri[1].at<float>(i,j);
            
            float ss = br*br + bi*bi;
            float cr = (ar*br + ai*bi)/ss;
            float ci = (ai*br - ar*bi)/ss;
            
            Cr.at<float>(i,j)=cr;
            Ci.at<float>(i,j)=ci;
        }
    }
    
    if(conj) {
        Ci = -Ci;
    }
    
    vector<Mat> Cri;
    Cri.push_back(Cr);
    Cri.push_back(Ci);
    
    Mat C;
    merge(Cri, C);
    return C;
}

MOSSE::MOSSE(Mat& frame, Rect rect){
    // random number
    srand((unsigned int)time(NULL));
    
    // compute the optimal bounding box size
    int w = getOptimalDFTSize(rect.width);
    int h = getOptimalDFTSize(rect.height);
    int x1=floor((2*rect.x+rect.width-w)/2);
    int y1=floor((2*rect.y+rect.height-h)/2);
    
    pos.x=x1+0.5*(w-1);
    pos.y=y1+0.5*(h-1);
    size.width=w;
    size.height=h;
    
    // crop the image
    Mat img;
    getRectSubPix(frame, size, pos, img);
    
    // create Hanning window for preprocess
    createHanningWindow(win, size, CV_32F);
    
    // create ground truth
    Mat g = Mat::zeros(size,CV_32F);
    g.at<float>(h/2, w/2) = 1;
    GaussianBlur(g, g, Size(-1,-1), 2.0);
    double minVal; double maxVal;
    cv::minMaxLoc( g, &minVal, &maxVal);
    g=g/maxVal;
    cv::dft(g,G,DFT_COMPLEX_OUTPUT);
    
    // create filter satisfied the ground truth
    H1 = Mat::zeros(G.size(),CV_32FC2);
    H2 = Mat::zeros(G.size(),CV_32FC2);
    
    // random warp the image to create bootstrap trainning data
    for(int i=0; i<128 ; i++){
        Mat a=rnd_warp(img);
        preprocess(a);
        //cout<<a<<endl;
        Mat A;
        cv::dft(a,A,DFT_COMPLEX_OUTPUT);
        Mat H1_temp;
        Mat H2_temp;
        mulSpectrums(G, A, H1_temp, 0, true );
        mulSpectrums(A, A, H2_temp, 0, true );
        H1+=H1_temp;
        H2+=H2_temp;
        //cout << H1 << H2 << endl;
    }
    
    // compute the H
    update_kernel();
    
    // run the first frame
    update(frame);
}


void MOSSE::update(Mat& frame, double rate){

    while(true) {
        // run filter to predict movement
        getRectSubPix(frame, size, pos, last_img);
        preprocess(last_img);
        
        // correlate
        Mat last_resp;
        Point delta_xy;
        double psr=correlate(last_img,last_resp,delta_xy);
        
        if(psr>0.8) {
            isGood = true;
        } else {
            isGood = false;
        }
        
        // update location
        pos.x += delta_xy.x;
        pos.y += delta_xy.y;
        
        if(abs(delta_xy.x) < size.width*boundaryThre &&
           abs(delta_xy.y) < size.height*boundaryThre)
            break;
    }
    
    // train the filte based on the prediction
    getRectSubPix(frame, size, pos, last_img);
    preprocess(last_img);
    
    Mat A;
    cv::dft(last_img,A,DFT_COMPLEX_OUTPUT);
    Mat H1_temp;
    Mat H2_temp;
    mulSpectrums(G, A, H1_temp, 0, true );
    mulSpectrums(A, A, H2_temp, 0, true );
    H1=H1*(1.0-rate)+H1_temp*rate;
    H2=H2*(1.0-rate)+H2_temp*rate;
    update_kernel();
}

Mat MOSSE::state_vis(){
    Mat f;
    idft(H, f,DFT_SCALE|DFT_REAL_OUTPUT);
    Size f_size=f.size();
    int w=f_size.width;
    int h=f_size.height;
    
    shiftRows(f,-floor(h/2));
    shiftCols(f,-floor(w/2));
    
    double fminVal; double fmaxVal; Point fminLoc; Point fmaxLoc;
    minMaxLoc( f, &fminVal, &fmaxVal, &fminLoc, &fmaxLoc );
    
    Mat kernel =(f-fminVal*(Mat::ones(f.size(),CV_32FC1))/ (fmaxVal-fminVal)*255 );
    
    Mat kernel8;
    kernel.convertTo(kernel8, UINT8_MAX);
    
    Mat resp;
    resp=last_resp;
    double minVal; double maxVal; Point minLoc; Point maxLoc;
    minMaxLoc( resp, &minVal, &maxVal, &minLoc, &maxLoc );
    resp=(resp/maxVal)*255;
    
    Mat resp8;
    resp.convertTo(resp8, UINT8_MAX);
    
    
    Mat vis;
    hconcat(vis, kernel8, resp8);
    return vis;
}

void MOSSE::draw_state(Mat& vis){
//    int x=pos.x; int y=pos.y;
//    int w=size.width; int h=size.height;
//    int x1=int(x-0.5*w);
//    int y1=int(y-0.5*h);
//    int x2=int(x+0.5*w);
//    int y2=int(y+0.5*h);
//    
//    //rectangle(vis, Point (x1,y1), Point (x2,y2));
//    
//    if(good==true){
//        //circle(vis, Point (int(x), int(y)),2,(0, 0, 255),-1);
//    }
//    else{
//        //line(vis, Point (x1, y1), Point (x2, y2), (0, 0, 255));
//        //line(vis, Point (x2, y1), Point (x1, y2), (0, 0, 255));
//    }
//    
//    //draw_str(vis, (x1, y2+16), 'PSR: %.2f' % self.psr)<TODO>//import from common
}

void MOSSE::preprocess(Mat& image){
    // log space
    Mat image32;
    image.convertTo(image32, CV_32FC1);
    log(image32 + Mat::ones(image32.size(),CV_32FC1),image);
    
    // normalize
    Mat mean; Mat stddev;
    meanStdDev(image, mean, stddev);
    double smean=mean.at<double>(0);
    double sstd=stddev.at<double>(0);
    image = (image-smean*Mat::ones(image.size(),CV_32FC1)) / (sstd+eps);
    
    // center defuse weighting
    image=image.mul(win);
}

double MOSSE::correlate(Mat& img, Mat& last_resp, Point &delta_xy){
    
    // circular convolve in frequencey domain
    Mat C, I;
    cv::dft(img,I,DFT_COMPLEX_OUTPUT);
    mulSpectrums(I, H, C, 0, true );
    
    // convert response into spatial
    idft(C, last_resp, DFT_SCALE|DFT_REAL_OUTPUT);
    
    // compute dx dy
    double minVal;double maxVal; Point minLoc; Point maxLoc;
    minMaxLoc(last_resp,&minVal, &maxVal, &minLoc, &maxLoc);
    delta_xy= Point(maxLoc.x-last_resp.size().width/2,
                    maxLoc.y-last_resp.size().height/2);

    // compute psr
    Mat side_resp=last_resp(
        Rect(max(0,maxLoc.x-2),max(0,maxLoc.y-2),4,4));
    Mat mean; Mat stddev;
    meanStdDev(side_resp, mean, stddev);
    double smean=mean.at<float>(0);
    double sstd=stddev.at<float>(0);
    double psr=(maxVal-smean)/(sstd+eps);
    return psr;
}


void MOSSE::update_kernel(){
    // H <= *(H1 / H2)
    H=divSpec(H1,H2,true);
}


//circular shift one row from up to down
void MOSSE::shiftRows(Mat& mat) {
    
    Mat temp;
    Mat m;
    int k = (mat.rows-1);
    mat.row(k).copyTo(temp);
    for(; k > 0 ; k-- ) {
        m = mat.row(k);
        mat.row(k-1).copyTo(m);
    }
    m = mat.row(0);
    temp.copyTo(m);
    
}

void MOSSE::shiftRows(Mat& mat,int n) {
    
    if( n < 0 ) {
        n = -n;
        flip(mat,mat,0);
        for(int k=0; k < n;k++) {
            shiftRows(mat);
        }
        flip(mat,mat,0);
    } else {
        for(int k=0; k < n;k++) {
            shiftRows(mat);
        }
    }
}

//circular shift n columns from left to right if n > 0, -n columns from right to left if n < 0
void MOSSE::shiftCols(Mat& mat, int n) {
    
    if(n < 0){
        n = -n;
        flip(mat,mat,1);
        transpose(mat,mat);
        shiftRows(mat,n);
        transpose(mat,mat);
        flip(mat,mat,1);
    } else {
        transpose(mat,mat);
        shiftRows(mat,n);
        transpose(mat,mat);
    }
}

