#Efficient Implementation of Correlation Filter Based Tracker in ios 	 
Wenda Wang, Yu-Te Cheng	

##Result:
Video Demo: https://www.youtube.com/watch?v=rQPlwaHLW9o

Performance:

| Pathc Size | FPS |
| ---------- | --- |
| 64X64      | 750 |
| 128X128    | 300 |
| 256X256    | 125 |
| 512X512    | 38  |
											
##Summary: 
We try to implement the correlation filter based tracker in ios. Since the computation of correlation is just element-wise multiplication in frequency domain, the filter can run at very high frame rate, which is well-suited for mobile application. We also try to leverage the existing fast implementation of FFT and other linear algebra operations exist in ios framework.
					
##Background: 
The correlation filter based tracker requires considerable matrix operation in both training and tracking process. Armadillo and Eigen have various efficient classes, built-in functions for vectors and matrices, as well as matrix decomposition provided through the integration of LAPACK, OpenBLAS, etc. We will take advantage of these tools to speed up the tracker and achieve high performance at mobile platforms. 

Even though Visual tracking is relatively mature techniques in computer vision, there are still few applications in tracking taken into our daily life. For us, visual tracking has such potential and is able to make great impact to our world. Sports fans can use this technology to track their favorite players during the games, parents can keep the precious moment by tracking and recording their lovely children play. With a smart-phone with high speed camera and rapidly growing algorithms in tracking area, now, we can try something different in mobiles and in our lives.
					
##Challenge:
- Efficient implementation 

With some search, we find some existing implementation in python, matlab and java. Our goal will be to translate it into efficient c++ implementation for ios

- Theoretical Understanding

The correlation tracking bring connection between traditional signal processing techniques like FFT with more latest machine learning algorithm. Also correlation itself is interesting, it’s counterpart convolution is used in deep network. A deeper understand of correlation, and why it works will be a good study topic.

##Goals & Deliverables:
- Must have: Implementation of MOSSE filter
- Nice to have: Implementation of Kernelized Correlation filter

Success Metrics:
- Tracker works at least at 30fps
- Tracker can achieve similar performance (at least 80%) as reported in paper under the same datasets
- Show a live video, that tracker can track object (face, body….) in real life scene  		 							

##Schedule: 
CheckPoint1:

UI: Completed 
System Integration with Tracker, UI and camera: Completed

CheckPoint2:

In addition to the scheduled task, we also finished to UI implementation and System Integration. Now, We are ahead of the schedule, so we can focus on improving the performance of the current correlation filter.

CheckPoint3:

Finish the accelearation using vDSP and high frame rate camera




						
					 				
			
		

