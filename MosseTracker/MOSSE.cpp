
#include "MOSSE.h"


Mat rnd_warp(Mat& a){
    Size a_shape=a.size();
    int h=a_shape.height;
    int w=a_shape.width;
    Mat T= Mat::zeros(Size(2,3),CV_32F);
    double coef=0.2;
    double ang=(rand()-0.5)*coef;
    double c=cos(ang); double s=sin(ang);

    Mat T22 (T, Rect(0, 0, 2, 2) ); // using a rectangle
    //Mat E = A(Range::all(), Range(1,3));
    T22.at<float>(0,0) = c;
    T22.at<float>(0,1) = -s;
    T22.at<float>(1,0) = s;
    T22.at<float>(1,1) = c;
    

    Mat R = Mat(2, 2, CV_32F);
    randu(R, 0, 1);

    T22 += (R - 0.5*Mat::ones(Size(2,2),CV_32F)*coef);//
    Mat center = Mat(2, 1, CV_32F);
    center.at<float>(0,0) = w/2;
    center.at<float>(1,0) = h/2;

    T.col(2) = center - T22*center;
    
    Mat warped;
    warpAffine(a, warped, T, Size (w, h),BORDER_REFLECT);
    return warped;



}



Mat divSpec(Mat& A,Mat& B){

	Mat Ar=A.col(0);
	Mat Ai=A.col(1);
	Mat Br=B.col(0);
	Mat Bi=B.col(1);


	Mat C=(Ar+1j*Ai)/(Br+1j*Bi);
    Size C_size=C.size();


    Mat D= Mat::zeros(Size(C_size.height,2),CV_32F);

    for(int i=0;i<C_size.height;i++){
        D.at<float>(i,0)=C.at<std::complex<float>>(i).real();
        D.at<float>(i,1)=C.at<std::complex<float>>(i).imag();
    }

    return D;
}


MOSSE::MOSSE(Mat& frame, Rect rect){
    int x1=rect.x;
    int y1=rect.y;
    int x2=rect.x+rect.width;
    int y2=rect.y+rect.height;
    int w = getOptimalDFTSize(x2-x1+1);
    int h = getOptimalDFTSize(y2-y1+1); 
    x1=floor((x1+x2-w)/2);
    y1=floor((y1+y2-h)/2);
    int x=x1+0.5*(w-1);//take care of int
    int y=y1+0.5*(h-1);//take care of int
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
    g.at<float>(floor(h/2), floor(w/2)) = 1;

    Size ksize=Size(-1,-1);
    GaussianBlur(g, g, ksize, 2.0);
    double minVal; double maxVal; 
    cv::minMaxLoc( g, &minVal, &maxVal);
    g=(1/maxVal)*g;

    Mat G;
    cv::dft(g,G,DFT_COMPLEX_OUTPUT);

      
    Mat H1; Mat H2;
    H1 = Mat::zeros(G.size(),CV_32F);
    H2 = Mat::zeros(G.size(),CV_32F);

    for(int i=0; i<128 ; i++){
    	Mat a=rnd_warp(img);
        preprocess(a);
    	Mat A;
		cv::dft(a,A,DFT_COMPLEX_OUTPUT);
		Mat H1_temp;
		Mat H2_temp;
		mulSpectrums(G, A, H1_temp, 0, true );
		mulSpectrums(A, A, H2_temp, 0, true );
		H1=H1+H1_temp;
		H2=H2+H2_temp;
    }
    
    update_kernel();
    update(frame);
}


cv::Point MOSSE::update(Mat& frame, double rate){
	int x=pos.x;
	int y=pos.y;
	//int w=size.width;
	//int h=size.height;

	Mat img;

    getRectSubPix(frame, size, pos, img);
    last_img=img;
    preprocess(img);

    Mat last_resp;
   
    
    double psr;
    Point delta_xy;//dx=delta_xy[0] dy=delta_xy[1]
    psr=correlate(img,last_resp,delta_xy);
    if(psr>0.8){good=true;}
    if(good==false){return Point(0,0);}

    pos.x=x+delta_xy.x;
    pos.y=y+delta_xy.y;


    getRectSubPix(frame, size, pos, img);
    last_img=img;
    preprocess(img);



    Mat A;
	cv::dft(img,A,DFT_COMPLEX_OUTPUT);
	Mat H1_temp;
	Mat H2_temp;
	mulSpectrums(G, A, H1_temp, 0, true );
	mulSpectrums(A, A, H2_temp, 0, true );
	H1=H1*(1.0-rate)+H1_temp*rate;
	H2=H2*(1.0-rate)+H2_temp*rate;
	
    update_kernel();
    
    return pos;

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

	int x=pos.x; int y=pos.y;
	int w=size.width; int h=size.height;
	int x1=int(x-0.5*w);
	int y1=int(y-0.5*h);
	int x2=int(x+0.5*w);
	int y2=int(y+0.5*h);
    
	//rectangle(vis, Point (x1,y1), Point (x2,y2));

	if(good==true){
		//circle(vis, Point (int(x), int(y)),2,(0, 0, 255),-1);
	}
	else{
		//line(vis, Point (x1, y1), Point (x2, y2), (0, 0, 255));
        //line(vis, Point (x2, y1), Point (x1, y2), (0, 0, 255));
	}

	//draw_str(vis, (x1, y2+16), 'PSR: %.2f' % self.psr)<TODO>//import from common

}


    void MOSSE::preprocess(Mat& image){
	Mat img;
    Mat image32;
    image.convertTo(image32, CV_32FC1);
        log(image32+1*Mat::ones(image32.size(),CV_32FC1),img);

    Mat mean;Mat stddev;
    meanStdDev(img, mean, stddev);
    double smean=mean.at<float>(0);
    double sstd=stddev.at<float>(0);

        img =(img-smean*Mat::ones(img.size(),CV_32FC1)) / (sstd+eps);
    image=img*win;

}


double MOSSE::correlate(Mat& img, Mat& last_resp, Point &delta_xy){

	Mat C;
	Mat img_dft;
	cv::dft(img,img_dft,DFT_COMPLEX_OUTPUT);
	mulSpectrums(img_dft, H, C, 0, true );

	Mat resp;
	idft(C, resp,DFT_SCALE|DFT_REAL_OUTPUT);

    Size2i resp_shape=resp.size();
    int w=resp_shape.width;
    int h=resp_shape.height;

    double minVal;double maxVal; Point minLoc; Point maxLoc;
    minMaxLoc(resp,&minVal, &maxVal, &minLoc, &maxLoc);

    Mat side_resp=resp.clone();
    //rectangle(side_resp, Point (maxLoc.x-5,maxLoc.y-5), Point (maxLoc.x+5,maxLoc.y+5));

    
    Mat mean; Mat stddev;
    meanStdDev(side_resp, mean, stddev);
    double smean=mean.at<float>(0);
    double sstd=stddev.at<float>(0);
    double psr=(maxVal-smean)/(sstd+eps);


    last_resp=resp;
    delta_xy= Point(floor(maxLoc.x-w/2),floor(maxLoc.y-h/2));
    return psr;

}


void MOSSE::update_kernel(){
	H=divSpec(H1,H2);
	H.col(1)=H.col(1)*(-1);
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

