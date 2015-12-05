//
//  MyVideoCamera.h
//  MosseTracker
//
//  Created by Wenda Wang on 12/4/15.
//  Copyright © 2015 Wenda Wang. All rights reserved.
//

#ifndef MyVideoCamera_h
#define MyVideoCamera_h

#import <UIKit/UIKit.h>
#import <Accelerate/Accelerate.h>
#import <AVFoundation/AVFoundation.h>
#import <ImageIO/ImageIO.h>
#include <opencv2/core.hpp>

@class MyVideoCamera;

@protocol MyVideoCameraDelegate <NSObject>

#ifdef __cplusplus
// delegate method for processing image frames
- (void)processImage:(cv::Mat&)image;
#endif

@end

@interface MyVideoCamera : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>
{
    AVCaptureSession* captureSession;
    AVCaptureConnection* videoCaptureConnection;
    AVCaptureVideoPreviewLayer *captureVideoPreviewLayer;
    
    AVCaptureVideoDataOutput *videoDataOutput;
    dispatch_queue_t videoDataOutputQueue;
    
    UIDeviceOrientation currentDeviceOrientation;
    
    BOOL cameraAvailable;
    BOOL captureSessionLoaded;
    BOOL running;
    BOOL grayscaleMode;

    AVCaptureDevicePosition defaultAVCaptureDevicePosition;
    AVCaptureVideoOrientation defaultAVCaptureVideoOrientation;
    
    UIView* parentView;
}

@property (nonatomic, retain) AVCaptureSession* captureSession;
@property (nonatomic, retain) AVCaptureConnection* videoCaptureConnection;

@property (nonatomic, readonly) BOOL running;
@property (nonatomic, readonly) BOOL captureSessionLoaded;

@property (nonatomic, readonly) AVCaptureVideoPreviewLayer *captureVideoPreviewLayer;
@property (nonatomic, assign) AVCaptureDevicePosition defaultAVCaptureDevicePosition;
@property (nonatomic, assign) AVCaptureVideoOrientation defaultAVCaptureVideoOrientation;

@property (nonatomic, retain) UIView* parentView;

@property (nonatomic, assign) id<MyVideoCameraDelegate> delegate;
@property (nonatomic, assign) BOOL grayscaleMode;

- (void)start;
- (void)stop;
- (void)switchCameras;

- (id)initWithParentView:(UIView*)parent;

- (void)createVideoPreviewLayer;

@end

#endif /* MyVideoCamera_h */
