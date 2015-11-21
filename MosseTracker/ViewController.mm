//
//  ViewController.m
//  MosseTracker
//
//  Created by Wenda Wang on 11/20/15.
//  Copyright © 2015 Wenda Wang. All rights reserved.
//

#import "ViewController.h"
#include "MOSSE.h"

@interface ViewController () {
    // UI elements
    __weak IBOutlet UIImageView *imageView;
    __weak IBOutlet UIButton *startButton;
    __weak IBOutlet UIButton *stopButton;
    __weak IBOutlet UIView *drag1;
    __weak IBOutlet UIView *drag2;
    CAShapeLayer * rectOverlay;
    
    // opencv video stream
    CvVideoCamera* _videoCamera;
    
    // Tracker
    MOSSE* tracker;
    bool needInitialize;
    bool startTrack;
}

- (IBAction)actionStart:(id)sender;
- (IBAction)actionStop:(id)sender;


@property (nonatomic, retain) CvVideoCamera* videoCamera;


@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // set up opencv video camera
    self.videoCamera = [[CvVideoCamera alloc] initWithParentView:imageView];
    self.videoCamera.defaultAVCaptureDevicePosition = AVCaptureDevicePositionBack;
    self.videoCamera.defaultAVCaptureSessionPreset = AVCaptureSessionPreset640x480;
    self.videoCamera.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationPortrait;
    self.videoCamera.defaultFPS = 30;
    self.videoCamera.grayscaleMode = NO;
    self.videoCamera.delegate = self;
    
    // set up bounding box
    [self addOverlayRect];
    [self addGestureRecognizersForViews];
    
    // set up button
    [self->startButton setEnabled:YES];
    [self->stopButton setEnabled:NO];
    
    // set up tracker
    needInitialize = true;
    startTrack = false;

}

-(void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation{
    [self refreshRect];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    // start live stream!
    [self.videoCamera start];
    
    // refresh bounding box
    [self refreshRect];
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:(BOOL)animated];
    // stop live stream!
    [self.videoCamera stop];
}

// main method to process the image and do tracking
- (void)processImage:(Mat&)image {
    // convert to greyscale image
    Mat imagebw;
    cvtColor(image, imagebw, CV_BGR2GRAY);
    
    if(startTrack){
        if(needInitialize){
            // set up tracker
            cv::Rect rect = [self getRect];
            delete tracker;
            tracker = new MOSSE(imagebw, rect);
            needInitialize = false;
            //[self moveRectTo: 240 with: 320];
        } else {
            cv::Point loc = tracker->update(imagebw);
            [self moveRectTo: loc.x with: loc.y];
        }
    }
}

// when the start button is pressed
- (IBAction)actionStart:(id)sender {
    [self->drag1 setHidden:YES];
    [self->drag2 setHidden:YES];
    [self->startButton setEnabled:NO];
    [self->stopButton setEnabled:YES];
    
    startTrack = true;
    needInitialize = true;
}

// when the stop button is pressed
- (IBAction)actionStop:(id)sender {
    [self->drag1 setHidden:NO];
    [self->drag2 setHidden:NO];
    [self->startButton setEnabled:YES];
    [self->stopButton setEnabled:NO];
    
    startTrack = false;
    needInitialize = true;
}

// get bounding box size into opencv
-(cv::Rect) getRect{
    int x = drag1.center.x/1.6;
    int y = drag1.center.y/1.6;
    int width = (drag2.center.x - drag1.center.x)/1.6;
    int height = (drag2.center.y - drag1.center.y)/1.6;
    return cv::Rect(x,y,width,height);
}

// move bounding box based on its updated location from opencv
-(void)moveRectTo:(int)xpos with:(int)ypos{
    // move drags
    CGFloat xscreen = xpos*1.6;
    CGFloat yscreen = ypos*1.6;
    CGFloat xoff = xscreen - drag1.center.x;
    CGFloat yoff = yscreen - drag1.center.y;
    // move 1
    drag1.center = CGPointMake(xscreen, yscreen);
    // move 2
    drag2.center = CGPointMake(drag2.center.x+xoff,drag2.center.y+yoff);
    // refresh
    [self refreshRect];
}

// add layer which bounding box is drawed on
-(void)addOverlayRect{
    self->rectOverlay=[CAShapeLayer layer];
    self->rectOverlay.fillColor=[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:0.1].CGColor;
    self->rectOverlay.lineWidth = 2.0f;
    self->rectOverlay.lineCap = kCALineCapRound;
    self->rectOverlay.strokeColor = [[UIColor redColor] CGColor];
    [self.view.layer addSublayer:self->rectOverlay];
}

// initialize the interactive drager
-(void)addGestureRecognizersForViews{
    UIPanGestureRecognizer * pan=[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panEventHandler:)];
    [self->drag1 addGestureRecognizer:pan];
    pan=[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panEventHandler:)];
    [self->drag2 addGestureRecognizer:pan];
    pan=[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panEventHandler:)];
}

// moving event for interactive drag
-(void)panEventHandler:(UIPanGestureRecognizer *)pan{
    [self translateView:pan.view becauseOfGestureRecognizer:pan];
    [self refreshRect];
}

// moving event for interactive drag
-(void)translateView:(UIView *)view becauseOfGestureRecognizer:(UIPanGestureRecognizer *)pan{
    UIView * target= pan.view;
    CGPoint translation= [pan translationInView:self.view];
    target.center=CGPointMake(target.center.x+translation.x, target.center.y+translation.y);
    [pan setTranslation:CGPointZero inView:self.view];
}

// redraw the rectangle based on the drags' positions
-(void)refreshRect{
    UIBezierPath * rectPath=[UIBezierPath bezierPath];
    //Rectangle coordinates
    CGPoint view1Center=[self->drag1 center];
    CGPoint view4Center=[self->drag2 center];
    CGPoint view2Center=CGPointMake(view4Center.x, view1Center.y);
    CGPoint view3Center=CGPointMake(view1Center.x, view4Center.y);
    
    //Rectangle drawing
    [rectPath moveToPoint:view1Center];
    [rectPath addLineToPoint:view2Center];
    [rectPath addLineToPoint:view4Center];
    [rectPath addLineToPoint:view3Center];
    [rectPath addLineToPoint:view1Center];
    
    self->rectOverlay.path=rectPath.CGPath;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
