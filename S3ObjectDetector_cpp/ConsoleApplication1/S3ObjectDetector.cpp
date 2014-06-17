#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"


using namespace cv;



const int FRAME_SKIP = 50;
const int MIN_KEYPOINTS_FOUND = 1;
const int MIN_HESS = 500;



void magic(Mat img);
int vidStream(VideoCapture cap);
Mat makeCanvas(std::vector<cv::Mat>& vecMat, int windowHeight, int nRows);

//for some reason the image is only really ready after it's been passed to any method
void magic(Mat img){
	return;
}

int main()
{

    Mat object = imread("rasp3.jpg",CV_LOAD_IMAGE_GRAYSCALE);
    if( !object.data )
    {
        return -1;
    }

    //Detect the keypoints using SURF Detector
    SurfFeatureDetector detector( MIN_HESS );
    std::vector<KeyPoint> kp_object;

    detector.detect( object, kp_object );

    //Calculate descriptors (feature vectors)
    SurfDescriptorExtractor extractor;
    Mat des_object;

    extractor.compute( object, kp_object, des_object );

    //FlannBasedMatcher matcher;
	BFMatcher matcher(NORM_L1);

    VideoCapture cap("rtsp://10.0.0.8:1234/");
	//vidStream(cap);
    namedWindow("Good Matches");

    std::vector<Point2f> obj_corners(4);

    //Get the corners from the object
    obj_corners[0] = cvPoint(0,0);
    obj_corners[1] = cvPoint( object.cols, 0 );
    obj_corners[2] = cvPoint( object.cols, object.rows );
    obj_corners[3] = cvPoint( 0, object.rows );

    char key = 'a';
    int framecount = 0;
	bool not_found = true;
    while (not_found)
    {
        Mat frame;
        cap >> frame;
		
		waitKey(1);
		vector<Mat> images;
		Mat text(320,240, CV_8UC3, Scalar(255,255,255));
		images.push_back(frame);
		images.push_back(text);
		Mat canvas = makeCanvas(images,250,1);
        cv::imshow("Output Window", canvas);

        if (framecount < FRAME_SKIP)
        {
            framecount++;
            continue;
        }
		framecount = 0;

        Mat des_image, img_matches;
        std::vector<KeyPoint> kp_image;
        std::vector<vector<DMatch > > matches;
        std::vector<DMatch > good_matches;
        std::vector<Point2f> obj;
        std::vector<Point2f> scene;
        std::vector<Point2f> scene_corners(4);
        Mat H;
        Mat image;

        cvtColor(frame, image, CV_RGB2GRAY);
		waitKey(1);//magic(image);

        detector.detect( image, kp_image );
		if(kp_image.size() < MIN_KEYPOINTS_FOUND){
			continue;
		}
        extractor.compute( image, kp_image, des_image );

        matcher.knnMatch(des_object, des_image, matches, 2);

        for(int i = 0; i < min(des_image.rows-1,(int) matches.size()); i++) //THIS LOOP IS SENSITIVE TO SEGFAULTS
        {
            if((matches[i][0].distance < 0.6*(matches[i][1].distance)) && ((int) matches[i].size()<=2 && (int) matches[i].size()>0))
            {
                good_matches.push_back(matches[i][0]);
            }
        }

        //Draw only "good" matches
        drawMatches( object, kp_object, image, kp_image, good_matches, img_matches, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

        if (good_matches.size() >= 4)
        {
            for( int i = 0; i < good_matches.size(); i++ )
            {
                //Get the keypoints from the good matches
                obj.push_back( kp_object[ good_matches[i].queryIdx ].pt );
                scene.push_back( kp_image[ good_matches[i].trainIdx ].pt );
            }

            H = findHomography( obj, scene, CV_RANSAC );

            perspectiveTransform( obj_corners, scene_corners, H);

            //Draw lines between the corners (the mapped object in the scene image )
            line( img_matches, scene_corners[0] + Point2f( object.cols, 0), scene_corners[1] + Point2f( object.cols, 0), Scalar(0, 255, 0), 4 );
            line( img_matches, scene_corners[1] + Point2f( object.cols, 0), scene_corners[2] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
            line( img_matches, scene_corners[2] + Point2f( object.cols, 0), scene_corners[3] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
            line( img_matches, scene_corners[3] + Point2f( object.cols, 0), scene_corners[0] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
			not_found = false;
        }

        //Show detected matches
        imshow( "Good Matches", img_matches );
		
        key = waitKey(1);
    }
	vidStream(cap);
    return 0;
}



int vidStream(VideoCapture vcap) {
    cv::Mat image;

    /*const std::string videoStreamAddress = "rtsp://10.0.0.x:1234/"; 
    if(!vcap.open(videoStreamAddress)) {
        std::cout << "Error opening video stream or file" << std::endl;
        return -1;
    }*/

    cv::namedWindow("Output Window");

    for(;;) {
        if(!vcap.read(image)) {
            std::cout << "No frame" << std::endl;
            cv::waitKey();
        }
		vector<Mat> images;
		Mat text(320,240, CV_8UC3, Scalar(255,255,255));
		putText(text,"Raspberry Pi",Point(10,20),FONT_HERSHEY_PLAIN,1,false);
		putText(text,"HDMI, 2x USB, Ethernet",Point(10,40),FONT_HERSHEY_PLAIN,1,false);
		putText(text,"Analog Audio Video Ports",Point(10,60),FONT_HERSHEY_PLAIN,1,false);
		images.push_back(image);
		images.push_back(text);
		Mat canvas = makeCanvas(images,250,1);
        cv::imshow("Output Window", canvas);
        if(cv::waitKey(1) >= 0) break;
    }
	return 0;
}


Mat makeCanvas(std::vector<cv::Mat>& vecMat, int windowHeight, int nRows) {
            int N = vecMat.size();
            nRows  = nRows > N ? N : nRows; 
            int edgeThickness = 10;
            int imagesPerRow = ceil(double(N) / nRows);
            int resizeHeight = floor(2.0 * ((floor(double(windowHeight - edgeThickness) / nRows)) / 2.0)) - edgeThickness;
            int maxRowLength = 0;

            std::vector<int> resizeWidth;
            for (int i = 0; i < N;) {
                    int thisRowLen = 0;
                    for (int k = 0; k < imagesPerRow; k++) {
                            double aspectRatio = double(vecMat[i].cols) / vecMat[i].rows;
                            int temp = int( ceil(resizeHeight * aspectRatio));
                            resizeWidth.push_back(temp);
                            thisRowLen += temp;
                            if (++i == N) break;
                    }
                    if ((thisRowLen + edgeThickness * (imagesPerRow + 1)) > maxRowLength) {
                            maxRowLength = thisRowLen + edgeThickness * (imagesPerRow + 1);
                    }
            }
            int windowWidth = maxRowLength;
            cv::Mat canvasImage(windowHeight, windowWidth, CV_8UC3, Scalar(0, 0, 0));

            for (int k = 0, i = 0; i < nRows; i++) {
                    int y = i * resizeHeight + (i + 1) * edgeThickness;
                    int x_end = edgeThickness;
                    for (int j = 0; j < imagesPerRow && k < N; k++, j++) {
                            int x = x_end;
                            cv::Rect roi(x, y, resizeWidth[k], resizeHeight);
                            cv::Mat target_ROI = canvasImage(roi);
                            cv::resize(vecMat[k], target_ROI, target_ROI.size());
                            x_end += resizeWidth[k] + edgeThickness;
                    }
            }
            return canvasImage;
    }