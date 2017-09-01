#include "FastDPM.h"
#include <conio.h>
#include <fstream>
#include <iostream>

vector<string>  yuStdDirFiles( string DirName, vector<string> FileExtensions );
string trim_file_name( string FileName, int FLAG );

int	main()
{
	/* I. Get images */
	string	img_dir = "D:/jobs/SouceImageData/dealed/";
	string extensions[] = { ".jpg" };
	vector<string>	img_extensions( extensions, extensions+1 );
	vector<string>	imgnames = yuStdDirFiles( img_dir, img_extensions );
	imgnames.clear();
	//imgnames.push_back("D:/CodeBook/C++/FastDPM/FastDPM-2015-Nov/FastDPM-2014-May/000084.jpg");



	/************************************************************************/
	/*  my input test                                                       */
	/************************************************************************/

	string my_img_dir = "D:/CodeBook/C++/FastDPM/voc-dpm-voc-release5.02/VOCdata/VOCdevkit/VOC2007/";
	string test_file = "ImageSets/Main/test.txt";
	string  jpegsPath = "JPEGImages/";

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

	//FastDPM	PM( "model_inria_14_2_features.txt" );
	FastDPM	PM("model_200.txt");

	bool	flag = false;
	for( unsigned i=0; i<imgnames.size()-1500; i++ ){
		string	img_name = imgnames[i];
		Mat	img_uint81 = imread( img_name.c_str() );	
		Mat img_uint8;
		cv::resize(img_uint81, img_uint8,Size(176,144));

		if( img_uint8.empty() ){
			cout<<"Cannot get image "<<img_name<<endl;
			getchar();
			return -2;
		}
		cout<<"Processing "<<trim_file_name(img_name,0)<<endl;
		Mat	img = PM.prepareImg( img_uint8 );
		PM.detect( img, -0.5f, true, true );	
		cout<<"------------------------------------------------------------"<<endl;
		if( PM.detections.empty() )
			continue;
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