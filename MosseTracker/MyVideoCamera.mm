//
//  MyVideoCamera.m
//  MosseTracker
//
//  Created by Wenda Wang on 12/4/15.
//  Copyright © 2015 Wenda Wang. All rights reserved.
//

#import "MyVideoCamera.h"
//#include "precomp.h"

#pragma mark - Private Interface

@interface MyVideoCamera ()

@property (nonatomic, retain) AVCaptureVideoPreviewLayer* captureVideoPreviewLayer;
@property (nonatomic, retain) AVCaptureVideoDataOutput *videoDataOutput;

- (void)deviceOrientationDidChange:(NSNotification*)notification;
- (void)startCaptureSession;
- (void)createVideoDataOutput;

- (void)setDesiredCameraPosition:(AVCaptureDevicePosition)desiredPosition;

@end


#pragma mark - Implementation


@implementation MyVideoCamera



#pragma mark Public


@synthesize defaultAVCaptureDevicePosition;
@synthesize defaultAVCaptureVideoOrientation;

@synthesize captureSession;
@synthesize captureVideoPreviewLayer;
@synthesize videoCaptureConnection;
@synthesize running;
@synthesize captureSessionLoaded;

@synthesize parentView;

@synthesize delegate;
@synthesize grayscaleMode;
@synthesize videoDataOutput;

#pragma mark - Constructors

- (id)init;
{
    self = [super init];
    if (self) {
        // react to device orientation notifications
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(deviceOrientationDidChange:)
                                                     name:UIDeviceOrientationDidChangeNotification
                                                   object:nil];
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        currentDeviceOrientation = [[UIDevice currentDevice] orientation];
        
        
        // check if camera available
        cameraAvailable = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
        NSLog(@"camera available: %@", (cameraAvailable == YES ? @"YES" : @"NO") );
        
        running = NO;
        
        // set camera default configuration
        self.defaultAVCaptureDevicePosition = AVCaptureDevicePositionFront;
        self.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationLandscapeLeft;
        
        self.parentView = nil;
    }
    return self;
}


- (id)initWithParentView:(UIView*)parent;
{
    self = [super init];
    if (self) {
        // react to device orientation notifications
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(deviceOrientationDidChange:)
                                                     name:UIDeviceOrientationDidChangeNotification
                                                   object:nil];
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        currentDeviceOrientation = [[UIDevice currentDevice] orientation];
        
        
        // check if camera available
        cameraAvailable = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
        NSLog(@"camera available: %@", (cameraAvailable == YES ? @"YES" : @"NO") );
        
        running = NO;
        
        // set camera default configuration
        self.defaultAVCaptureDevicePosition = AVCaptureDevicePositionFront;
        self.defaultAVCaptureVideoOrientation = AVCaptureVideoOrientationLandscapeLeft;
        
        self.parentView = parent;
    }
    return self;
}



- (void)dealloc;
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
}


#pragma mark - Public interface


- (void)start;
{
    if (![NSThread isMainThread]) {
        NSLog(@"[Camera] Warning: Call start only from main thread");
        [self performSelectorOnMainThread:@selector(start) withObject:nil waitUntilDone:NO];
        return;
    }
    
    if (running == YES) {
        return;
    }
    running = YES;
    
    if (cameraAvailable) {
        [self startCaptureSession];
    }
}


- (void)pause;
{
    running = NO;
    [self.captureSession stopRunning];
}



- (void)stop;
{
    running = NO;
    
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
    for (AVCaptureInput *input in self.captureSession.inputs) {
        [self.captureSession removeInput:input];
    }
    
    for (AVCaptureOutput *output in self.captureSession.outputs) {
        [self.captureSession removeOutput:output];
    }
    
    [self.captureSession stopRunning];
    self.captureSession = nil;
    self.captureVideoPreviewLayer = nil;
    self.videoCaptureConnection = nil;
    captureSessionLoaded = NO;
    
    self.videoDataOutput = nil;
}



// use front/back camera
- (void)switchCameras;
{
    BOOL was_running = self.running;
    if (was_running) {
        [self stop];
    }
    if (self.defaultAVCaptureDevicePosition == AVCaptureDevicePositionFront) {
        self.defaultAVCaptureDevicePosition = AVCaptureDevicePositionBack;
    } else {
        self.defaultAVCaptureDevicePosition  = AVCaptureDevicePositionFront;
    }
    if (was_running) {
        [self start];
    }
}



#pragma mark - Device Orientation Changes


- (void)deviceOrientationDidChange:(NSNotification*)notification
{
    (void)notification;
    UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
    
    switch (orientation)
    {
        case UIDeviceOrientationPortrait:
        case UIDeviceOrientationPortraitUpsideDown:
        case UIDeviceOrientationLandscapeLeft:
        case UIDeviceOrientationLandscapeRight:
            currentDeviceOrientation = orientation;
            break;
            
        case UIDeviceOrientationFaceUp:
        case UIDeviceOrientationFaceDown:
        default:
            break;
    }
    NSLog(@"deviceOrientationDidChange: %d", (int)orientation);
}



#pragma mark - Private Interface

- (void)createCaptureSession;
{
    // set a av capture session preset
    self.captureSession = [[AVCaptureSession alloc] init];
}

- (void)createCaptureDevice;
{
    // setup the device
    AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    [self setDesiredCameraPosition:self.defaultAVCaptureDevicePosition];
    NSLog(@"[Camera] device connected? %@", device.connected ? @"YES" : @"NO");
    NSLog(@"[Camera] device position %@", (device.position == AVCaptureDevicePositionBack) ? @"back" : @"front");
}


- (void)createVideoPreviewLayer;
{
    self.captureVideoPreviewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:self.captureSession];
    
    if ([self.captureVideoPreviewLayer respondsToSelector:@selector(connection)])
    {
        if ([self.captureVideoPreviewLayer.connection isVideoOrientationSupported])
        {
            [self.captureVideoPreviewLayer.connection setVideoOrientation:self.defaultAVCaptureVideoOrientation];
        }
    }
    
    if (parentView != nil) {
        self.captureVideoPreviewLayer.frame = self.parentView.bounds;
        self.captureVideoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
        [self.parentView.layer addSublayer:self.captureVideoPreviewLayer];
    }
    NSLog(@"[Camera] created AVCaptureVideoPreviewLayer");
}

- (void)setDesiredCameraPosition:(AVCaptureDevicePosition)desiredPosition;
{
    for (AVCaptureDevice *device in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo]) {
        if ([device position] == desiredPosition) {
            [self.captureSession beginConfiguration];
            
            NSError* error = nil;
            
            // support for autofocus
            if ([device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus]) {
                error = nil;
                if ([device lockForConfiguration:&error]) {
                    device.focusMode = AVCaptureFocusModeContinuousAutoFocus;
                    [device unlockForConfiguration];
                } else {
                    NSLog(@"unable to lock device for autofocos configuration %@", [error localizedDescription]);
                }
            }
            
            // remove old inputs
            for (AVCaptureInput *oldInput in self.captureSession.inputs) {
                [self.captureSession removeInput:oldInput];
            }

            // create input
            AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
            if (!input) {
                NSLog(@"error creating input %@", [error localizedDescription]);
            }
            [self.captureSession addInput:input];
            
            // confirm
            [self.captureSession commitConfiguration];
            
            break;
        }
    }
}

- (void)createVideoDataOutput;
{
    // Make a video data output
    self.videoDataOutput = [AVCaptureVideoDataOutput new];
    
    // In grayscale mode we want YUV (YpCbCr 4:2:0) so we can directly access the graylevel intensity values (Y component)
    // In color mode we, BGRA format is used
    OSType format = self.grayscaleMode ? kCVPixelFormatType_420YpCbCr8BiPlanarFullRange : kCVPixelFormatType_32BGRA;
    
    self.videoDataOutput.videoSettings  = [NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedInt:format]
                                                                      forKey:(id)kCVPixelBufferPixelFormatTypeKey];
    
    // discard if the data output queue is blocked (as we process the still image)
    [self.videoDataOutput setAlwaysDiscardsLateVideoFrames:YES];
    
    if ( [self.captureSession canAddOutput:self.videoDataOutput] ) {
        [self.captureSession addOutput:self.videoDataOutput];
    }
    [[self.videoDataOutput connectionWithMediaType:AVMediaTypeVideo] setEnabled:YES];
    
    
    // set FPS
    AVCaptureDeviceInput *currentInput = [self.captureSession.inputs objectAtIndex:0];
    AVCaptureDevice *device = currentInput.device;
    
    NSError *error = nil;
    [device lockForConfiguration:&error];
    
    // find the highest frame rate format
    AVCaptureDeviceFormat *bestFormat = nil;
    AVFrameRateRange *bestFrameRateRange = nil;
    for ( AVCaptureDeviceFormat *format in [device formats] ) {
        for ( AVFrameRateRange *range in format.videoSupportedFrameRateRanges ) {
            if ( range.maxFrameRate > bestFrameRateRange.maxFrameRate ) {
                bestFormat = format;
                bestFrameRateRange = range;
            }
        }
    }
    if ( bestFormat ) {
        error = nil;
        if ( [device lockForConfiguration:&error]) {
            device.activeFormat = bestFormat;
            device.activeVideoMinFrameDuration = bestFrameRateRange.minFrameDuration;
            device.activeVideoMaxFrameDuration = bestFrameRateRange.minFrameDuration;
            [device unlockForConfiguration];
            NSLog(@"set to hightest possible frame rate %f", bestFrameRateRange.maxFrameRate);
        } else {
            NSLog(@"unable to set to hightest frame rate %@", [error localizedDescription]);
        }
    }
    
    [device unlockForConfiguration];
    
    // set video mirroring for front camera (more intuitive)
    if ([self.videoDataOutput connectionWithMediaType:AVMediaTypeVideo].supportsVideoMirroring) {
        if (self.defaultAVCaptureDevicePosition == AVCaptureDevicePositionFront) {
            [self.videoDataOutput connectionWithMediaType:AVMediaTypeVideo].videoMirrored = YES;
        } else {
            [self.videoDataOutput connectionWithMediaType:AVMediaTypeVideo].videoMirrored = NO;
        }
    }
    
    // set default video orientation
    if ([self.videoDataOutput connectionWithMediaType:AVMediaTypeVideo].supportsVideoOrientation) {
        [self.videoDataOutput connectionWithMediaType:AVMediaTypeVideo].videoOrientation = self.defaultAVCaptureVideoOrientation;
    }
    
    // create a serial dispatch queue used for the sample buffer delegate as well as when a still image is captured
    // a serial dispatch queue must be used to guarantee that video frames will be delivered in order
    // see the header doc for setSampleBufferDelegate:queue: for more information
    videoDataOutputQueue = dispatch_queue_create("VideoDataOutputQueue", DISPATCH_QUEUE_SERIAL);
    [self.videoDataOutput setSampleBufferDelegate:self queue:videoDataOutputQueue];
    
    
    NSLog(@"[Camera] created AVCaptureVideoDataOutput");
}

- (void)startCaptureSession
{
    if (!cameraAvailable) {
        return;
    }
    
    if (self.captureSessionLoaded == NO) {
        [self createCaptureSession];
        [self createCaptureDevice];
        [self createVideoPreviewLayer];
        [self createVideoDataOutput];

        captureSessionLoaded = YES;
    }
    
    [self.captureSession startRunning];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    (void)captureOutput;
    (void)connection;
    if (self.delegate) {
        
        // convert from Core Media to Core Video
        CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
        CVPixelBufferLockBaseAddress(imageBuffer, 0);
        
        void* bufferAddress;
        size_t width;
        size_t height;
        size_t bytesPerRow;
        
        CGColorSpaceRef colorSpace;
        
        int format_opencv;
        
        OSType format = CVPixelBufferGetPixelFormatType(imageBuffer);
        if (format == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) {
            
            format_opencv = CV_8UC1;
            
            bufferAddress = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
            width = CVPixelBufferGetWidthOfPlane(imageBuffer, 0);
            height = CVPixelBufferGetHeightOfPlane(imageBuffer, 0);
            bytesPerRow = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, 0);
            
        } else { // expect kCVPixelFormatType_32BGRA
            
            format_opencv = CV_8UC4;
            
            bufferAddress = CVPixelBufferGetBaseAddress(imageBuffer);
            width = CVPixelBufferGetWidth(imageBuffer);
            height = CVPixelBufferGetHeight(imageBuffer);
            bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
            
        }
        
        // delegate image processing to the delegate
        cv::Mat image((int)height, (int)width, format_opencv, bufferAddress, bytesPerRow);
        
        // process the image
        if ([self.delegate respondsToSelector:@selector(processImage:)]) {
            [self.delegate processImage:image];
        }
        
        // cleanup
        CGColorSpaceRelease(colorSpace);
        CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    }
}

@end