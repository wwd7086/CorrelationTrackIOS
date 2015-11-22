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
    __weak IBOutlet UILabel *fpsText;
    __weak IBOutlet UILabel *sizeText;
    CAShapeLayer * rectOverlay;
    
    // opencv video stream
    CvVideoCamera* _videoCamera;
    
    // Tracker
    MOSSE* tracker;
    bool needInitialize;
    bool startTrack;
    
    // Timer
    NSDate *lastFrameTime;
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
    self.videoCamera.defaultFPS = 240;
    self.videoCamera.grayscaleMode = YES;
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
    
    // log incoming frame rate
    NSDate *curFrameTime = [NSDate date];
    NSTimeInterval frameInterval = [curFrameTime timeIntervalSinceDate:lastFrameTime];
    NSLog(@"frameInterval = %f", frameInterval);
    lastFrameTime = curFrameTime;
    
    cv::Point loc;
    if(startTrack){
        if(needInitialize){
            // set up tracker
            cv::Rect rect = [self getRect];
            delete tracker;
            tracker = new MOSSE(image, rect);
            needInitialize = false;
            // visualize
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                cv::Rect rect = tracker->getRect();
                sizeText.text = [NSString stringWithFormat:@"W:%d H:%d",rect.width,rect.height ];
            }];
        } else {
            tracker->update(image);
            cv::circle(image, tracker->getCenter(), 10, Scalar(255,0,255));
        }
        
        // log processing frame rate
        NSDate *finishTime = [NSDate date];
        NSTimeInterval methodExecution = [finishTime timeIntervalSinceDate:curFrameTime];
        NSLog(@"execution = %f", methodExecution);
        // visualize
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            [self moveRect: tracker->getRect()];
            fpsText.text = [NSString stringWithFormat:@"fps: %d", (int)(1/methodExecution)];
        }];
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
    
    [[NSOperationQueue mainQueue] waitUntilAllOperationsAreFinished];
    [self refreshRect];
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
-(void)moveRect:(cv::Rect)rect {
    
    UIBezierPath * rectPath=[UIBezierPath bezierPath];
    
    //Rectangle coordinates
    CGPoint view1Center=CGPointMake(rect.x*1.6, rect.y*1.6);
    CGPoint view4Center=CGPointMake(view1Center.x+rect.width*1.6, view1Center.y+rect.height*1.6);
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
