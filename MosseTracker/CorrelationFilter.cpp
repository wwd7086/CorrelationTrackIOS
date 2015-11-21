#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <opencv2/opencv.hpp>


using namespace cv;

float eps = 0.00001;



Mat rnd_warp(Mat& a){
    Size a_shape=a.shape();
    int h=a_shape.height;
    int w=a_shape.width;
    Mat T= Mat::zeros(Size(2,3),CV_32F);
    double coef=0.2;
    double ang=(rand()-0.5)*coef;
    double c=cos(ang); double s=sin(ang);

    Mat T22 (T, Rect(0, 0, 2, 2) ); // using a rectangle
    //Mat E = A(Range::all(), Range(1,3));
    T22=[c,-s;s,c];

    Mat R = Mat(2, 2, CV_32F);
    randu(R, 0, 1);

    T22 += (R - 0.5*ones(Size(2,2),CV_32F)*coef;//
    Mat c = [w/2;h/2];
    T.col(2) = c - T22*c;
    
    Mat warped;
    warpAffine(a, warped, T, Size (w, h), borderMode = BORDER_REFLECT);
    return warped;



}



Mat divSpec(Mat& A,Mat& B){

	Mat Ar=A.col(0);
	Mat Ai=A.col(1);
	Mat Br=B.col(0);
	Mat Bi=B.col(1);


	Mat C=(Ar+1j*Ai)/(Br+1j*Bi);
    C_size=c.size();


    Mat_<std::complex<double> D=Mat::zeros(Size(C_size.height,2),CV_32F);

    for(i=0;i<C_size.height;i++){
        D[i][0]=C[i].real();
        D[i][1]=C[i].imag();
    }

	return D
}


class MOSSE{
    
    Size2i size;//size.width size.height
    Point  pos;//pos.x pos.y
    Mat last_img;
    double psr;
    bool good=false;
public:


};


MOSSE::MOSSE(Mat& frame, Rect rect){
    int x1=rect.x;
    int y1=rect.y;
    int x2=rect.x+rect.width;
    int y2=rect.y+rect.height;
    int w = getOptimalDFTSize(x2-x1+1);
    int h = getOptimalDFTSize(y2-y1+1); 
    x1=floor((x1+x2-w)/2);
    y1=floor((y1+y2-h)/2);
    x=x1+0.5*(w-1);//take care of int
    y=y1+0.5*(h-1);//take care of int
    pos.x=x;
    pos.y=y;
    size.width=w;
    size.height=h;

    Mat img;
    getRectSubPix(frame, size, pos, img);


    Mat win;
    createHanningWindow(win, size, CV_32F);
    Mat g;
    g = Mat::zeros(size,CV_32F);
    g[floor(h/2), floor(w/2)] = 1;

    Size ksize=Size(-1,-1);
    GaussianBlur(g, g, ksize, 2.0);
    double minVal; double maxVal; 
    cv::minMaxLoc( g, &minVal, &maxVal);
    g=(1/maxVal)*g;

    Mat G;
    cv::dft(g,G,DFT_COMPLEX_OUTPUT);

      
    Mat H1; Mat H2;
    H1 = Mat::zeros(Mat::size(G),CV_32F);
    H2 = Mat::zeros(Mat::size(G),CV_32F);

    for(int i=0; i<128 ; i++){
    	Mat a=preprocess(rnd_warp(img));
    	Mat A;
		cv::dft(a,A,DFT_COMPLEX_OUTPUT);
		Mat H1_temp;
		Mat H2_temp;
		mulSpectrums(G, A, H1_temp, 0, conjB=True );
		mulSpectrums(A, A, H2_temp, 0, conjB=True );
		H1=H1+H1_temp;
		H2=H2+H2_temp;
    }
    
    update_kernel();
    update(frame);
}


void MOSSE::update(Mat& frame, double rate=0.125){
	int x=pos.x;
	int y=pos.y;
	int w=size.width;
	int h=size.height;

	Mat img;

    getRectSubPix(frame, size, pos, img);
    last_img=img;
    img=preprocess(img);

    Mat last_resp;
    vec2f delta_xy ;
    
    double=psr;
    vec2f delta_xy;//dx=delta_xy[0] dy=delta_xy[1]
    psr=correlate(img,&last_resp,&delta_xy);
    if(psr>0.8){good=true;}
    if(good==false){return;}

    pos.x=x+delta_xy[0];
    pos.y=y+delta_xy[1];


    getRectSubPix(frame, size, pos, img);
    last_img=img;
    img=preprocess(img);



    Mat A;
	cv::dft(img,A,DFT_COMPLEX_OUTPUT);
	Mat H1_temp;
	Mat H2_temp;
	mulSpectrums(G, A, H1_temp, 0, conjB=True );
	mulSpectrums(A, A, H2_temp, 0, conjB=True );
	H1=H1*(1.0-rate)+H1_temp*rate;
	H2=H2*(1.0-rate)+H2_temp*rate;
	
    update_kernel();

}


Mat MOSSE::state_vis(){
    Mat f;
    idft(H, f,DFT_SCALE|DFT_REAL_OUTPUT);
    Size f_size=f.size();
    w=f_size.width;
    h=f_size.height;

    shiftRows(f,-floor(h/2));
    shiftCols(f,-floor(w/2));

    double fminVal; double fmaxVal; Point fminLoc; Point fmaxLoc;
    minMaxLoc( f, &fminVal, &fmaxVal, &fminLoc, &fmaxLoc );

    kernel =( (f-fminVal*(Mat::ones(f.size(),CV_32FC1))/ (fmaxVal-fminVal)*255 );

    Mat kernel8;
    kernel.convertTo(kernel8, uint8);

    Mat resp;
    resp=last_resp;
    double minVal; double maxVal; Point minLoc; Point maxLoc;
    minMaxLoc( resp, &minVal, &maxVal, &minLoc, &maxLoc );
    resp=(resp/maxVal)*255);
    
    Mat resp8;
    resp.convertTo(resp8, uint8);


    Mat vis;
    vis=hconcat(last_img, kernel, resp8);
    return vis;


}



void MOSSE::draw_state(Mat& vis){

	int x=pos.x; int y=pos.y;
	int w=size.width; int h=size.height;
	int x1=int(x-0.5*w);
	int y1=int(y-0.5*h);
	int x2=int(x+0.5*w);
	int y2=int(y+0.5*h);
	rectangle(vis, Point (x1,y1), Point (x2,y2));

	if(good==true){
		circle(vis, Point (int(x), int(y)),2,(0, 0, 255),-1);
	}
	else{
		line(vis, Point (x1, y1), Point (x2, y2), (0, 0, 255));
        line(vis, Point (x2, y1), Point (x1, y2), (0, 0, 255));
	}

	//draw_str(vis, (x1, y2+16), 'PSR: %.2f' % self.psr)<TODO>//import from common

}


Mat MOSSE::preprocess(Mat& image){
	Mat img;
    Mat image32;
    image.convertTo(image32, CV_32FC1);
    log(image32+1*ones(image32.size,CV_32FC1),img);

    Mat mean;Mat stddev;
    meanStdDev(img, mean, stddev);
    double smean=mean[0];
    double sstd=stddev[0];

    img =(img-smean*ones(img.size,CV_32FC1)) / (sstd+eps);
    return img*win;

}


double MOSSE::correlate(Mat& img,Mat Mat& last_resp,Point &delta_xy){

	Mat C;
	Mat img_dft;
	cv::dft(img,imag_dft,DFT_COMPLEX_OUTPUT);
	mulSpectrums(img_dft, H, C, 0, conjB=True );

	Mat resp;
	idft(C, resp,DFT_SCALE|DFT_REAL_OUTPUT);

    Size2i resp_shape=resp.size();
    int w=resp_shape.wideth;
    int h=resp_shape.height;

    double minVal;double maxVal; Point minLoc; Point maxLoc;
    minMaxLoc(resp,&minVal, &maxVal, &minLoc, &maxLoc);

    side_resp=resp.clone();
    rectangle(side_resp, Point (maxLoc.x-5,maxLoc.y-5), Point (maxLoc.x+5,maxLoc.y+5));

    
    Mat mean; Mat stddev;
    meanStdDev(side_resp, mean, stddev);
    double smean=mean[0];//TAKE CARE
    double sstd=stddev[0];
    double psr=(mval-smean)/(sstd+eps);


    *last_resp=resp;
    *delta_xy= Point(floor(maxLoc.x-w/2),floor(maxLoc.y-h/2)); 
    return psr;

}


void MOSSE::update_kernal(){
	H=divSpec(H1,H2);
	H.cols(1)=H.cols(1)*(-1);
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

