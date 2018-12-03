/*
 * client.cpp
 *
 *  Created on: Nov 29, 2018
 *      Author: 4205_13
 */

#include <stdio.h>
#include <bitset>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <ctime>
#include <math.h>
#include <sstream>
#include <dirent.h>

using namespace cv;
using namespace std;

// Video Formats
const string vidFormat1 = "320 x 240";
const string vidFormat2 = "800 x 600";
const string vidFormat3 = "960 x 720";
const string vidFormat4 = "1280 x 960";

// Image Box Attributes
string showTitle = "";
double textSize = 1.0;
int frameLineWidth = 5;

// Information Carrier
uchar *iptr;
int bytes = 0;
uint32_t clientStatus = 0;
uint32_t serverStatus = 0;

// RGB Image Attributes
int widthRGB = 1280;
int heightRGB = 960;
Mat imgRGB = Mat::zeros(heightRGB,widthRGB,CV_8UC3);
int imgSize;

// HOG Image Attributes
int scanMatrixWidth = 16;
int widthHOG = widthRGB/scanMatrixWidth;
int heightHOG = heightRGB/scanMatrixWidth;
int pixelCountHOG = widthHOG*heightHOG;
Mat imgHOG = Mat::zeros(heightHOG,widthHOG, CV_8UC1);

// Histogram Attributes
int histSize = 8; // Number of Bins
float range[] = { 0, 256 } ; // Set the range of values (upper bound exclusive)
const float* histRange = { range };

// Face Detect Attributes
double faceDetectThreshold = 0.8;
vector<string> names;

// Face Image Attributes
double goldenRatio = 1.618;
int heightFace = 4*round(heightHOG/5.0);
int widthFace = round(heightFace/goldenRatio);
vector<Mat> imgFaces;


/**
	\fn changeVidFormat(int input)
	\brief Change le format de l'image et change le uint32_t envoye au serveur pour l'informer de ce changement.
	\param input 
	Peut avoir une valeur de [1,4] pointant vers une des quatres options de format video
	**/
void changeVidFormat(int input){

	// Update Width and Height
	if(input == 1){

		// if it was already this width
		if(widthRGB == 320){	
			return;		
		}

		widthRGB = 320;
		heightRGB = 240;
		scanMatrixWidth = 2;
		textSize = 0.5;
		frameLineWidth = 2;
		showTitle = vidFormat1;

	}else if(input == 2){

		// if it was already this width
		if(widthRGB == 800){	
			return;		
		}

		widthRGB = 800;
		heightRGB = 600;
		scanMatrixWidth = 4;
		textSize = 0.8;
		frameLineWidth = 5;
		showTitle = vidFormat2;

	}else if(input == 3){

		// if it was already this width
		if(widthRGB == 960){	
			return;		
		}

		widthRGB = 960;
		heightRGB = 720;
		scanMatrixWidth = 8;
		textSize = 1;
		frameLineWidth = 10;
		showTitle = vidFormat3;

	}else if(input == 4){

		// if it was already this width
		if(widthRGB == 1280){	
			return;		
		}

		widthRGB = 1280;
		heightRGB = 960;
		scanMatrixWidth = 16;
		textSize = 1.5;
		frameLineWidth = 20;
		showTitle = vidFormat4;
	}

	// Update Status Integer
	clientStatus = (clientStatus << 4) >> 4;	// clears first 4 bits
	clientStatus = clientStatus + (1 << (32-input));	// updates the right bit
	
	// Close Image Show Box
	destroyAllWindows();

	// Update Image
	imgRGB = Mat::zeros(heightRGB,widthRGB, CV_8UC3);
	imgSize = imgRGB.total() * imgRGB.elemSize();
	iptr = imgRGB.data;
	bytes = 0;

	widthHOG = widthRGB/scanMatrixWidth;
	heightHOG = heightRGB/scanMatrixWidth;
	pixelCountHOG = widthHOG*heightHOG;
	imgHOG = Mat::zeros(heightHOG,widthHOG, CV_8UC1);

	if (!imgRGB.isContinuous()) {
		imgRGB = imgRGB.clone();
	}
}

/**
	\fn start()
	\brief Tells the server that the client is ready to receive captures
	**/
void start(){
	// the 5th bit indicate if we carry on or quit
	clientStatus &= ~(1UL << 32-5);
}

/**
	\fn stop()
	\brief Tells the server that the client wants to stop receiving captures
	**/
void stop(){
	// the 5th bit indicate if we carry on or quit
	clientStatus |= 1UL << (32-5);
}

/**
	\fn enableCamera(bool enable)
	\brief Tells the server that the client wants to enable/disable the camera
	\param enable
	If true, enables the camera
	**/
void enableCamera(bool enable){
	// the 6th bit indicate if we want to receive img

	if(enable){
		clientStatus |= 1UL << (32-6);
	}else{
		clientStatus &= ~(1UL << 32-6);
	}
}

/**
	\fn saveFaceHOG(string faceName)
	\brief Saves the HOG of the cropped face image in the program folder
	\param faceName
	The name given to the saved image
	**/
bool saveFaceHOG(string faceName){

	int startFrameX = round((widthHOG-widthFace)/2.0);
	int startFrameY = round((heightHOG-heightFace)/2.0);

	// Setup a rectangle to define the region of interest
	Rect myROI(startFrameX, startFrameY, widthFace, heightFace);

	// Crop the full image to that image contained by the rectangle myROI
	Mat imgFace = imgHOG(myROI).clone();
	imgFaces.push_back(imgFace);

	// Save image
	string fileName = faceName + ".png";
	string filePath = "faces/" + fileName;

	return imwrite(filePath,imgFace);
}


/**
	\fn generateHOG()
	\brief Generates the HOG from the RGB image
	**/
void generateHOG(){

	// Initialize Process Matrices
	Mat imgGrayscale = Mat::zeros(heightRGB, widthRGB, CV_8UC1);
	Mat imgGradient = Mat::zeros(heightRGB, widthRGB, CV_8UC1);
	
	// Initialize Scanning Matrix
	int scanMatrix[8];

	// Convert RGB to Grayscale
	cvtColor(imgRGB,imgGrayscale, CV_RGB2GRAY);

	// Check if conversion was successful
	if (imgGrayscale.data == NULL){
		cout << "Couldn't convert to grayscale" << endl;
		return;
	}

	// Scanning Matrix goes CHOO CHOO!
	// We skip the borders [1,N-1]
	for(int i = 1 ; i < heightRGB-1; i++){
		for(int j = 1 ; j < widthRGB-1 ; j++){

			scanMatrix[3] = (int) imgGrayscale.at<uchar>(i-1,j-1);
			scanMatrix[2] = (int) imgGrayscale.at<uchar>(i-1,j);
			scanMatrix[1] = (int) imgGrayscale.at<uchar>(i-1,j+1);

			scanMatrix[4] = (int) imgGrayscale.at<uchar>(i,j-1);
			scanMatrix[0] = (int) imgGrayscale.at<uchar>(i,j+1);

			scanMatrix[5] = (int) imgGrayscale.at<uchar>(i+1,j-1);
			scanMatrix[6] = (int) imgGrayscale.at<uchar>(i+1,j);
			scanMatrix[7] = (int) imgGrayscale.at<uchar>(i+1,j+1);

			// We set the Gradient Image (i,j) to be the index of darkest surrounding pixel
			imgGradient.at<uchar>(i,j) = distance(scanMatrix, max_element(scanMatrix,scanMatrix+8));
		}
	}

	int globalI = 0;
	int globalJ = 0;
	int angleTotal = 0;
	int angleMean = 0;
	for(int k = 0 ; k < pixelCountHOG ; k++){

		// Scan through sub images
		for(int i = 0 ; i < scanMatrixWidth; i++){
			for(int j = 0 ; j < scanMatrixWidth; j++){
				// Sum neighboring angles
				angleTotal += imgGradient.at<uchar>(globalI+i,globalJ+j);
			}
		}

		// We take the mean of angleTotal
		angleMean = round(angleTotal/(1.0*(scanMatrixWidth*scanMatrixWidth)));

		// Set The HOG (i,j) to the local average on the RGB image
		imgHOG.at<uchar>(globalI/scanMatrixWidth, globalJ/scanMatrixWidth) = angleMean*32;

		// Reset angleTotal
		angleTotal = 0;

		// Increment global i
		globalI += scanMatrixWidth;

		// If we reached the max i we go back to the top and increment column (j)
		if(globalI >= imgGradient.rows){
			globalI = 0;
			globalJ += scanMatrixWidth;
		}			
	}	
}


/**
	\fn showHOG()
	\brief Shows the HOG in a new window
	**/
void showHOG(){
	namedWindow("HOG", WINDOW_NORMAL);
	resizeWindow("HOG", widthRGB/5, heightRGB/5);
	imshow("HOG",imgHOG);
}

/**
	\fn calculateHist(Mat imgToHist)
	\brief Calculates the Histogram of the provided image
	\param imgToHist
	The image that we want to analyze
	**/
Mat calculateHist(Mat imgToHist){

	bool uniform = true; 
	bool accumulate = false;
	
	/// Compute the histograms
	Mat hist;
	calcHist( &imgToHist, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );

	return hist.clone();
}


/**
	\fn faceDetect()
	\brief Function that scans the stream and compares a region of interest with the saved faces histogram
	**/
void faceDetect(){
	
	// Resized Face Image
	Mat imgFaceResized;
	double scaleFactor = 1.0;
	int widthFaceResized = 0;
	int heightFaceResized = 0;
	int pixelCountFaceResized = 0;

	// Fit Variables
	double score = 0;
	vector<double> maxScore;
	vector<int> maxI;
	vector<int> maxJ;
	vector<double> maxScaleFactor;

	for(int p = 0 ; p < imgFaces.size() ; p++){

		int maxI_temp = 0;
		int maxJ_temp = 0;
		int maxScale_temp = 0;
		double maxScore_temp = 0;

		// Resize Face
		for(int k = 0 ; k < 10 ; k++){
			
			// We set the scale factor [0.7;1.3]
			scaleFactor = 0.7+0.05*k;

			// Resize Face
			widthFaceResized = round(widthFace*scaleFactor);
			heightFaceResized = round(heightFace*scaleFactor);
			pixelCountFaceResized = widthFaceResized*heightFaceResized;
			Size size(widthFaceResized,heightFaceResized);
			resize(imgFaces.at(p),imgFaceResized,size);

			// On divise l'image en 4 fractions
			int halfWidth = round(widthFaceResized/2.0);
			int thirdHeight = round(heightFaceResized/3.0);
			Rect topleftRect(0,0,halfWidth, thirdHeight);
			Rect toprightRect(halfWidth,0,halfWidth-1, thirdHeight);	
			Rect bottomleftRect(0,thirdHeight,halfWidth, 2*thirdHeight-1);
			Rect bottomrightRect(halfWidth,thirdHeight,halfWidth-1, 2*thirdHeight-1);	
			Mat topleftFaceImg = imgFaceResized(topleftRect).clone();
			Mat toprightFaceImg = imgFaceResized(toprightRect).clone();	
			Mat bottomleftFaceImg = imgFaceResized(bottomleftRect).clone();
			Mat bottomrightFaceImg = imgFaceResized(bottomrightRect).clone();

			// Scan through HOG image to see where face fits best
			for(int i = 0 ; i < widthHOG-widthFaceResized; i+=5){
				for(int j = 0 ; j < heightHOG-heightFaceResized; j+=5){
					
					// Move a face sized cropping rectangle over the HOG image
					Rect myROI(i, j, widthFaceResized, heightFaceResized);
					Mat croppedImg = imgHOG(myROI).clone();

					Mat topleftCroppedImg = croppedImg(topleftRect).clone();
					Mat toprightCroppedImg = croppedImg(toprightRect).clone();
					Mat bottomleftCroppedImg = croppedImg(bottomleftRect).clone();
					Mat bottomrightCroppedImg = croppedImg(bottomrightRect).clone();
					
					// Compare the histogram of the face and the cropped rectangle
					Mat histDiffTopLeft;
					Mat histDiffTopRight;
					Mat histDiffBottomLeft;
					Mat histDiffBottomRight;
					absdiff(calculateHist(topleftCroppedImg),calculateHist(topleftFaceImg),histDiffTopLeft);
					absdiff(calculateHist(toprightCroppedImg),calculateHist(toprightFaceImg),histDiffTopRight);
					absdiff(calculateHist(bottomleftCroppedImg),calculateHist(bottomleftFaceImg),histDiffBottomLeft);
					absdiff(calculateHist(bottomrightCroppedImg),calculateHist(bottomrightFaceImg),histDiffBottomRight);
					
					// Calculate Score
					score = 1 - (sum(histDiffTopLeft)[0] + sum(histDiffTopRight)[0] + sum(histDiffBottomLeft)[0] + sum(histDiffBottomRight)[0])/(1.0*pixelCountFaceResized);

					// If we have a better fit
					if(score > maxScore_temp && score > faceDetectThreshold){
						maxI_temp = i;
						maxJ_temp = j;
						maxScale_temp = scaleFactor;
						maxScore_temp = score;
					}			
				}
			}
		}

		maxI.push_back(maxI_temp);
		maxJ.push_back(maxJ_temp);
		maxScaleFactor.push_back(maxScale_temp);
		maxScore.push_back(maxScore_temp);
	}

	for(int i = 0 ; i < maxI.size() ; i++){

		// Add rectangle
		int X1 = round((maxI.at(i)/(1.0*widthHOG))*widthRGB);
		int Y1 = round((maxJ.at(i)/(1.0*heightHOG))*heightRGB);
		int X2 = round(X1 + ((widthFace*maxScaleFactor.at(i))/(1.0*widthHOG))*widthRGB);
		int Y2 = round(Y1 + ((heightFace*maxScaleFactor.at(i))/(1.0*heightHOG))*heightRGB);
		
		rectangle(imgRGB,Point(X1, Y1),Point(X2, Y2),Scalar(255, 255, 0), frameLineWidth);
		stringstream nametag;
		nametag << names.at(i) << ", score = " << maxScore.at(i);
		putText(imgRGB, nametag.str(), cvPoint(X1,Y1-25), FONT_HERSHEY_COMPLEX_SMALL, textSize, cvScalar(255,255,0), 1, CV_AA);		

		
	}
}

/**
	\fn hasEnding(string const &fullString, string const &ending)
	\brief Checks if a string ends with a specific string
	**/
bool hasEnding(string const &fullString, string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

/**
	\fn importFaces()
	\brief Imports the faces from the faces folder
	**/
void importFaces(){

	DIR *dir;
	struct dirent *ent;
	vector<string> fileNames;

	if ((dir = opendir ("faces/")) == NULL) {
		return;
	}

	while ((ent = readdir (dir)) != NULL) {

		string fileName = ent->d_name;
		string filePath = "faces/" + fileName;
		
		// Check if ends with .png
		if(!hasEnding(fileName,".png")){
			continue;
		}

		// Read the file
		Mat im = imread(filePath, 2);
		
		if (im.empty()) {
			continue;
		}
		
		// Add to faces
		imgFaces.push_back(im);
		names.push_back(fileName);		
	}

	closedir (dir);
}

/**
\mainpage
Ce programme se connecte sur le port d'un serveur et recoit un stream video. Il peut changer le format du video. Il peut reconnaitre
des visages dans le video.
**/
int main(int argc, char *argv[]){

	// ------------------------------------------------------------------------
	// ------------------ On initie la connection au socket -------------------
	// ------------------------------------------------------------------------
	int sockfd, n;
	int portno = 4099;	// the port number
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		cout << "ERROR, couldn't open socket" << endl;
		return -1;
	}

	server = gethostbyname("192.168.7.2"); // the server's ip

	if (server == NULL) {
		cout << "ERROR, no such host" << endl;
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		cout << "ERROR, server script not running" << endl;
		return -1;
	}
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------


	// Show different video format available
	cout << "- Video format available -" << endl << "1. 320 x 240" << endl << "2. 800 x 600" << endl << "3. 960 x 720" << endl << "4. 1280 x 960" << endl;

	// By default start with format 1
	changeVidFormat(1);

	// Start
	start();

	bool grabIMG = false;
	bool saveIMG = false;
	bool IMGsaved = false;

	// Imports all saved faces
	importFaces();	

	while(1){

		while(1){

			string yesNo = "";
			cout << "Do you want to save another face (y/n)? ";
			cin >> yesNo;

			if(yesNo.compare("n") == 0){
				break;
			}

			if(yesNo.compare("y") != 0){
				cout << "Wrong input" << endl;
				break;
			}

			string name;
			cout << "Enter your name : ";
			cin >> name;
			names.push_back(name);

			cout << endl << "Put your face in the frame and press the spacebar" << endl;

			while (1) {

				// Wait for Server status
				if(recv(sockfd, &serverStatus, sizeof(uint32_t), 0) < 0){
						cout << "Couldnt receive server status" << endl;
						return -1; // error
				}

				enableCamera(true);

				// Send the client status
				n = send(sockfd, &clientStatus, sizeof(uint32_t), 0);
				if(n < 0){
					cout << "Couldnt send client status" << endl;
					return -1; // error
				}		

				// Receive image from server
				if ((bytes = recv(sockfd, (char *)iptr, imgSize, MSG_WAITALL)) == -1) {
					cout << "Error while recv frame" << endl;
					return -1;
				}

				// Add Face frame (20px border)
				int heightFaceFrame = 4*round(heightRGB/5.0);
				int widthFaceFrame = round(heightFaceFrame/goldenRatio);
				int X1 = round((widthRGB-widthFaceFrame)/2.0);
				int Y1 = round((heightRGB-heightFaceFrame)/2.0);
				int X2 = X1 + widthFaceFrame;
				int Y2 = Y1 + heightFaceFrame;
				rectangle(imgRGB,Point(X1, Y1),Point(X2, Y2),Scalar(255, 255, 0), frameLineWidth);	

				// Show Image received
				imshow(showTitle, imgRGB);
				
				// Wait for 30ms and listen if a key was pressed
				int keyPressed = (waitKey(30) % 256);
				if (keyPressed == 27){		// Esc pressed
					stop();
					cout << "Exiting program" << endl;
					return -1;

				}else if(keyPressed == 49){	// 1 pressed
					changeVidFormat(1);

				}else if(keyPressed == 50){	// 2 pressed
					changeVidFormat(2);

				}else if(keyPressed == 51){	// 3 pressed
					changeVidFormat(3);

				}else if(keyPressed == 52){	// 4 pressed
					changeVidFormat(4);

				}else if (keyPressed == 32){		// Space Bar pressed
					generateHOG();
					if(saveFaceHOG(name)){
						cout << name << "'s face saved!" << endl << endl;
						break;
					}else{
						cout << "Couldn't save Face, try again" << endl;
						return -1;
					}
				}	
			}
		}

		cout << endl << endl << "To add a new face press space bar" << endl;

		while (1) {

			// Wait for Server status
			if(recv(sockfd, &serverStatus, sizeof(uint32_t), 0) < 0){
					cout << "Couldnt receive server status" << endl;
					return -1; // error
			}

			//bitset<32> x(serverStatus);
			//cout << " Server : " << x << endl;
			

			// Check if server status is GUCCI
			grabIMG = false;

			if(((serverStatus >> (32-1)) & 1U) == 0){ // IDOWN
				enableCamera(false);
				
			}else if(((serverStatus >> (32-1)) & 1U) == 1){	// READY
				enableCamera(true);
				grabIMG = true;

				if((((serverStatus >> (32-2)) & 1U) == 1) && !saveIMG){ // PUSHB
					saveIMG = true;

					time_t t = time(0);   // get time now
					tm* now = std::localtime(&t);
					int hour = now->tm_hour;
					int min = now->tm_min;
					int sec = now->tm_sec;
					stringstream ss;
					ss << hour << "_" << min << "_" << sec << ".png";
					string imgName = ss.str();
					
					imwrite(imgName, imgRGB);

					cout << imgName << " was saved" << endl;

				}else if(((serverStatus >> (32-2)) & 1U) == 0){
					saveIMG = false;
				}

			}
			
			//bitset<32> y(clientStatus);
			//cout << " Client : " << y << endl;

			// Send the client status
			n = send(sockfd, &clientStatus, sizeof(uint32_t), 0);
			if(n < 0){
				cout << "Couldnt send client status" << endl;
				return -1; // error
			}

			if(!grabIMG){
				continue;
			}		

			// Receive image from server
			if ((bytes = recv(sockfd, (char *)iptr, imgSize, MSG_WAITALL)) == -1) {
				cout << "Error while recv frame" << endl;
				return -1;
			}

			generateHOG();
			faceDetect();
			
			// Show Image received
			imshow(showTitle, imgRGB);

			
			// Wait for 30ms and listen if a key was pressed
			int keyPressed = (waitKey(30) % 256);
			if (keyPressed == 27){		// Esc pressed
				stop();
				cout << "Exiting program" << endl;
				return -1;

			}else if(keyPressed == 49){	// 1 pressed
				changeVidFormat(1);

			}else if(keyPressed == 50){	// 2 pressed
				changeVidFormat(2);

			}else if(keyPressed == 51){	// 3 pressed
				changeVidFormat(3);

			}else if(keyPressed == 52){	// 4 pressed
				changeVidFormat(4);
			}else if(keyPressed == 32){	// 4 pressed
				break;
			}
		}
	}
	

	destroyAllWindows();
	close(sockfd);
	return 0;
}
