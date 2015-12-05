//
//  ViewController.h
//  MosseTracker
//
//  Created by Wenda Wang on 11/20/15.
//  Copyright © 2015 Wenda Wang. All rights reserved.
//

#import "MyVideoCamera.h"
#import <UIKit/UIKit.h>
#import <opencv2/videoio/cap_ios.h>
#import <opencv2/opencv.hpp>

using namespace cv;

@interface ViewController : UIViewController<MyVideoCameraDelegate>

@end

