#include "FastDPM.h"
#include <conio.h>
#include <fstream>
#include <iostream>
#include <sstream>

vector<string>  yuStdDirFiles( string DirName, vector<string> FileExtensions );
string trim_file_name( string FileName, int FLAG );

int	main()
{
	
	/************************************************************************/
	/*  my input test                                                       */
	/************************************************************************/

	string my_img_dir = "D:/CodeBook/C++/FastDPM/voc-dpm-voc-release5.02/VOCdata/VOCdevkit/VOC2007/";
	string test_file = "ImageSets/Main/test.txt";
	string  jpegsPath = "JPEGImages/";
	vector<string>	imgnames;
	imgnames.clear();

	ifstream infile;
	infile.open((my_img_dir + test_file).c_str(),ios::in);
	assert(infile.is_open());

	string s;
	
	while (!infile.eof())
	{
		getline(infile, s);
		s = my_img_dir + jpegsPath + s + ".jpg";
		imgnames.push_back(s);
	}
	infile.close();

	/* II. Perform Part_Model based detection */
	int sumPeople = 0;
	FastDPM	PM("model_5000.txt");
	PM.model.interval = 10;
	bool	flag = false;
	//VideoCapture cap("D:/CodeBook/C++/cross_line_double_person/0901video/videoÕý³£2_91.avi");
	////VideoCapture cap(1);
	//int vIndex = 0;
	//string vPrefix = "D:/CodeBook/C++/cross_line_double_person/0901video/VideoCapture/vn3_";
	//string vSuffix = ".jpg";
	//while (1)
	//{
	//	Mat img_uint8, temp;
	//	cap >> temp;
	//	if (temp.empty())
	//	{
	//		cap.release();
	//		break;
	//	}
	//	img_uint8 = temp(Rect(0, 0, 352, 288));

	//	double angle = -90;
	//	Point2f center(img_uint8.cols / 2, img_uint8.rows / 2);
	//	Mat rot = getRotationMatrix2D(center, angle, 1);
	//	Rect bbox = RotatedRect(center, img_uint8.size(), angle).boundingRect();

	//	rot.at<double>(0, 2) += bbox.width / 2.0 - center.x;
	//	rot.at<double>(1, 2) += bbox.height / 2.0 - center.y;

	//	warpAffine(img_uint8, img_uint8, rot, bbox.size());

	//	imshow("FAST_DPMs", img_uint8); waitKey(1);

	//	// Write Frame file
	//	/*vIndex++;
	//	stringstream ss;
	//	ss << vIndex;
	//	string strIndex;
	//	ss >> strIndex;

	//	string img_name = vPrefix + strIndex + vSuffix;
	//	imwrite(img_name.c_str(), img_uint8);*/

	//	Mat	img = PM.prepareImg(img_uint8);
	//	PM.detect(img, 0, true, true);
	//	cout << "------------------------------------------------------------" << endl;
	//	if (PM.detections.empty())
	//		continue;

	//	//sumPeople++;
	//	//cout << "detection : " << sumPeople << " / " << imgnames.size() - 1500 << endl;

	//	flag = true;
	//	char key = waitKey(1);
	//	if (key == 27)
	//		break;
	//	

	//}
	//cout << "Write Done!!" << endl;

	//return 1;

	for( unsigned i=0; i<imgnames.size(); i++ ){
		string	img_name = imgnames[i];
		Mat	img_uint8 = imread( img_name.c_str() );	
		//Mat img_uint8;
		//cv::resize(img_uint81, img_uint8,Size(176,144));

		if( img_uint8.empty() ){
			cout<<"Cannot get image "<<img_name<<endl;
			getchar();
			return -2;
		}
		cout<<"Processing "<<trim_file_name(img_name,0)<<endl;
		Mat	img = PM.prepareImg( img_uint8 );

		int strIndex = img_name.find(".jpg");

		string detectedName = img_name.assign(img_name.c_str(), strIndex);
		detectedName += "_det.jpg";
	
		//PM.detect( img, 0.7, true, true ,detectedName)	 ;	
		PM.detect(img, 0, true, true);
		cout<<"------------------------------------------------------------"<<endl;

		if( PM.detections.empty() )
			continue;

		sumPeople++;
		cout << "detection : " << sumPeople << " / " << imgnames.size()  << endl;

		flag = true;
		char key = waitKey(1);
		if( key==27 )
			break;
	}
	cout<<"Finished!"<<endl;
	if( flag )
		waitKey();
	else
		_getch();

	return	0;
}