
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

// return fft size pow2
int getFFTSize(int n) {
    int i=0;
    while(true) {
        if(pow(2,i)>=n)
            break;
        else
            i++;
    }
    return i;
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

//---------------------------------------------------------------------------------------
//------------------------------------- vDSP Helper -------------------------------------


float* MOSSE::createDSPSplitComplex(DSPSplitComplex& splitComplex) {
    float* memory = (float*) malloc(totalSize * sizeof(float));
    splitComplex = DSPSplitComplex{memory, memory + totalSize/2};
    return memory;
}

void MOSSE::doFFT(cv::Mat& mat, DSPSplitComplex* splitComplex) {
    vDSP_ctoz((DSPComplex *) mat.ptr<float>(), 2, splitComplex, 1, totalSize/2);
    vDSP_fft2d_zrip(FFTSetup, splitComplex, 1, 0, wlog2, hlog2, FFT_FORWARD);
}

// c = a .* (conj b)
void MOSSE::doMulti(DSPSplitComplex* a, DSPSplitComplex* b, DSPSplitComplex* c) {
    // vector dot product
    vDSP_zvmul(b, 1, a, 1, c, 1, totalSize/2, -1); //conj on b
    
    // first row, first two coloums
    c->realp[0] = a->realp[0] * b->realp[0];
    c->imagp[0] = a->imagp[0] * b->imagp[0];
    
    // second row, first two coloums
    int ind = size.width/2;
    c->realp[ind] = a->realp[ind] * b->realp[ind];
    c->imagp[ind] = a->imagp[ind] * b->imagp[ind];
    
    // sub rows, first two coloums
    ind += size.width/2;
    for(int i=0; i<size.height/2-1; i++) {
        
        // first item in row
        float r1 = a->realp[ind];
        float r2 = b->realp[ind];
        float c1 = a->realp[ind + size.width/2];
        float c2 = b->realp[ind + size.width/2];
        
        float r3 = r1*r2 + c1*c2; // conj
        float c3 = r2*c1 - r1*c2;
        
        c->realp[ind] = r3;
        c->realp[ind + size.width/2] = c3;
        
        // second item in row
        r1 = a->imagp[ind];
        r2 = b->imagp[ind];
        c1 = a->imagp[ind + size.width/2];
        c2 = b->imagp[ind + size.width/2];
        
        r3 = r1*r2 + c1*c2; // conj
        c3 = r2*c1 - r1*c2;
        
        c->imagp[ind] = r3;
        c->imagp[ind + size.width/2] = c3;
        
        // increment index by two rows
        ind += size.width;
    }
}

// conj (c = a ./ b)
void MOSSE::doDivide(DSPSplitComplex* a, DSPSplitComplex* b, DSPSplitComplex* c) {
    // first row, first two coloums
    c->realp[0] = a->realp[0] / b->realp[0];
    c->imagp[0] = a->imagp[0] / b->imagp[0];
    
    // second row, first two coloums
    int ind = size.width/2;
    c->realp[ind] = a->realp[ind] / b->realp[ind];
    c->imagp[ind] = a->imagp[ind] / b->imagp[ind];
    
    // sub rows, first two coloums
    ind += size.width/2;
    for(int i=0; i<size.height/2-1; i++) {
        
        // first item in row
        float r1 = a->realp[ind];
        float r2 = b->realp[ind];
        float c1 = a->realp[ind + size.width/2];
        float c2 = b->realp[ind + size.width/2];
        
        float ss = r2*r2 + c2*c2;
        float r3 = (r1*r2 + c1*c2)/ss;
        float c3 = (c1*r2 - r1*c2)/ss;
        
        c->realp[ind] = r3;
        c->realp[ind + size.width/2] = -c3;
        
        // second item in row
        r1 = a->imagp[ind];
        r2 = b->imagp[ind];
        c1 = a->imagp[ind + size.width/2];
        c2 = b->imagp[ind + size.width/2];
        
        ss = r2*r2 + c2*c2;
        r3 = (r1*r2 + c1*c2)/ss;
        c3 = (c1*r2 - r1*c2)/ss;
        
        c->imagp[ind] = r3;
        c->imagp[ind + size.width/2] = -c3;
        
        // increment index by two rows
        ind += size.width;
    }
    
    // sub rows, other coloums
    ind = 1;
    for(int row=0; row<size.height; row++) {
        for(int col=0; col<size.width/2-1; col++) {
            float r1 = a->realp[ind];
            float r2 = b->realp[ind];
            float c1 = a->imagp[ind];
            float c2 = b->imagp[ind];
            
            float ss = r2*r2 + c2*c2;
            float r3 = (r1*r2 + c1*c2)/ss;
            float c3 = (c1*r2 - r1*c2)/ss;
            
            c->realp[ind] = r3;
            c->imagp[ind] = -c3;
            
            ind++;
        }
        ind++;
    }
}

void MOSSE::doScale(DSPSplitComplex* a, float scale) {
    vDSP_vsmul(a->realp, 1, &scale, a->realp, 1, totalSize/2);
    vDSP_vsmul(a->imagp, 1, &scale, a->imagp, 1, totalSize/2);
}

//---------------------------------------------------------------------------------------
//-------------------------------  MOSSE Implementation ---------------------------------

MOSSE::MOSSE(Mat& frame, cv::Rect rect){
    // random number
    srand((unsigned int)time(NULL));
    
    // compute the optimal bounding box size
    wlog2 = getFFTSize(rect.width);
    hlog2 = getFFTSize(rect.height);
    int w = pow(2,wlog2);
    int h = pow(2,hlog2);
    totalSize = w*h;
    int x1=floor((2*rect.x+rect.width-w)/2);
    int y1=floor((2*rect.y+rect.height-h)/2);
    
    pos.x=x1+0.5*(w-1);
    pos.y=y1+0.5*(h-1);
    size.width=w;
    size.height=h;
    
    // init FFT
    FFTSetup = vDSP_create_fftsetup(11, FFT_RADIX2);
    G_M = createDSPSplitComplex(G);
    H1_M = createDSPSplitComplex(H1);
    H2_M = createDSPSplitComplex(H2);
    H_M = createDSPSplitComplex(H);
    I_M = createDSPSplitComplex(I);
    H1t_M = createDSPSplitComplex(H1t);
    H2t_M = createDSPSplitComplex(H2t);
    R_M = createDSPSplitComplex(R);
    
    // crop the image
    Mat img;
    getRectSubPix(frame, size, pos, img);
    
    // init response matrix
    last_resp.create(size.height, size.width, cv::DataType<float>::type);
    
    // create Hanning window for preprocess
    createHanningWindow(win, size, CV_32F);
    
    // create ground truth
    Mat g = Mat::zeros(size,CV_32F);
    g.at<float>(h/2, w/2) = 1;
    GaussianBlur(g, g, cv::Size(-1,-1), 2.0);
    double minVal; double maxVal;
    cv::minMaxLoc( g, &minVal, &maxVal);
    g=g/maxVal;
    doFFT(g, &G);
    
    // random warp the image to create bootstrap trainning data
    for(int i=0; i<128 ; i++){
        Mat a=rnd_warp(img);
        preprocess(a);
        //cout<<a<<endl;
        
        doFFT(a, &I);
        doMulti(&G, &I, &H1t);
        doMulti(&I, &I, &H2t);
        vDSP_zvadd(&H1, 1, &H1t, 1, &H1, 1, totalSize/2);
        vDSP_zvadd(&H2, 1, &H2t, 1, &H2, 1, totalSize/2);
    }
    
    // compute the H
    update_kernel();
    
    // run the first frame
    update(frame);
}

MOSSE::~MOSSE() {
    //clean FFT
    vDSP_destroy_fftsetup(FFTSetup);
    free(G_M);
    free(H_M);
    free(H1_M);
    free(H2_M);
    free(H1t_M);
    free(H2t_M);
    free(I_M);
    free(R_M);
}

void MOSSE::update(Mat& frame, float rate){
    
    int updateCount = 0;
    while(updateCount < updateThre) {
        // run filter to predict movement
        getRectSubPix(frame, size, pos, last_img);
        preprocess(last_img);
        
        // correlate
        cv::Point delta_xy;
        double psr=correlate(last_img,delta_xy);
        
        if(psr>0.8) {
            isGood = true;
        } else {
            isGood = false;
        }
        
        // update location
        pos.x += delta_xy.x;
        pos.y += delta_xy.y;
        
        // counting
        updateCount++;
        
        if(abs(delta_xy.x) < size.width*boundaryThre &&
           abs(delta_xy.y) < size.height*boundaryThre)
            break;
    }
    if(updateCount>1)
        std::cout<<"!!large motion iterative update!!:"<<updateCount<<std::endl;
    
    // train the filte based on the prediction
    getRectSubPix(frame, size, pos, last_img);
    preprocess(last_img);

    doFFT(last_img, &I);
    doMulti(&G, &I, &H1t);
    doMulti(&I, &I, &H2t);
    
    doScale(&H1t, rate);
    doScale(&H1, 1.0-rate);
    vDSP_zvadd(&H1, 1, &H1t, 1, &H1, 1, totalSize/2);
    doScale(&H2t, rate);
    doScale(&H2, 1.0-rate);
    vDSP_zvadd(&H2, 1, &H2t, 1, &H2, 1, totalSize/2);

    update_kernel();
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

double MOSSE::correlate(Mat& img, cv::Point &delta_xy){
    
    // circular convolve in frequencey domain
    doFFT(img, &I);
    doMulti(&I, &H, &R);
    
    // convert response into spatial
    vDSP_fft2d_zrip(FFTSetup, &R, 1, 0, wlog2, hlog2, FFT_INVERSE);
    vDSP_ztoc(&R, 1, (DSPComplex*) last_resp.ptr<float>() , 2, totalSize/2);
    
    // compute dx dy
    double minVal;double maxVal;
    cv::Point minLoc; cv::Point maxLoc;
    minMaxLoc(last_resp,&minVal, &maxVal, &minLoc, &maxLoc);
    delta_xy= cv::Point(maxLoc.x-last_resp.size().width/2,
                    maxLoc.y-last_resp.size().height/2);

    // compute psr
    double psr = 0.9;
    if(isDebug){
        Mat side_resp=last_resp(
            cv::Rect(max(0,maxLoc.x-2),max(0,maxLoc.y-2),4,4));
        Mat mean; Mat stddev;
        meanStdDev(side_resp, mean, stddev);
        double smean=mean.at<float>(0);
        double sstd=stddev.at<float>(0);
        psr=(maxVal-smean)/(sstd+eps);
    }

    return psr;
}


void MOSSE::update_kernel(){
    // H <= *(H1 / H2)
    doDivide(&H1, &H2, &H);
}