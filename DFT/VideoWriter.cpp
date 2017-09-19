#include <opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int isequal(Mat a, Mat b)
{
	if (a.size() != b.size())
	{
		cout << "two Mat size is not same..." << endl;
		return 0;
	}

	int width = a.cols;
	int height = a.rows;

	Mat c;
	absdiff(a, b, c);
	
	if (countNonZero(c) == 0)
	{
		return 1; // a is equal to b
	}
	else
	{
		return 0;
	}
}

void printMat(Mat src )
{
	cout << "nchannels = " << src.channels() << endl;
	if (src.channels()==1)
	{
		for (int i = 0; i < src.rows; i++)
		{
			for (int j = 0; j < src.cols; j++)
			{
				//cout << src.at<float>(i, j) << "\t";
				printf("%10.2f", src.at<float>(i, j));
			}
			cout << endl;
		}
	}
	else
	{
		vector<Mat> mv;
		
		split(src, mv);

		for (int c = 0; c < mv.size(); c++)
		{
			cout << "chanel [" << c << "]:" << endl;

			for (int i = 0; i < mv[c].rows; i++)
			{
				for (int j = 0; j < mv[c].cols; j++)
				{
					printf("%10.2f", mv[c].at<float>(i, j)); 
					//<< mv[c].at<float>(i, j) << "\t";
				}
				cout << endl;
			}
		}
	}

	cout << "Size : " << src.size() << endl;
	cout << "-----------" << endl;


}

Mat calc_complex_multiply(Mat src1, Mat src2)
{
	vector<Mat> mv1, mv2;

	split(src1, mv1);
	split(src2, mv2);

	Mat ac, bd, bc, ad;
	multiply(mv1[0], mv2[0], ac);
	multiply(mv1[1], mv2[1], bd, -1.0);
	multiply(mv1[1], mv2[0], bc);
	multiply(mv1[0], mv2[1], ad);

	add(ac, bd, ac);
	add(bc, ad, bc);

	vector<Mat> vm;
	vm.push_back(ac);
	vm.push_back(bc);
	Mat conv;
	merge(vm, conv);

	return conv;
}

Mat calc_dft(Mat img)
{
	int M = getOptimalDFTSize(img.rows);
	int N = getOptimalDFTSize(img.cols);
	Mat padded;
	copyMakeBorder(img, padded, 0, M - img.rows, 0, N - img.cols, BORDER_CONSTANT, Scalar::all(0));

	Mat planes[] = { Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F) };
	Mat complexImg;
	merge(planes, 2, complexImg);

	double t;
	t = getTickCount();
	dft(complexImg, complexImg);
	t = ((double)getTickCount() - t)*1000. / getTickFrequency();
	cout << "   -- dft once time : " << t << endl;
	return complexImg;
}
// conv_fft2 function
Mat paddMat(Mat src1, Mat kernel){

	double t;
	Mat k,src;
	src1.copyTo(src);
	t = getTickCount();     // -- step 1 : transpose 180 and padd zeros ----
	//transpose k -180
	flip(kernel, k, -1);

	int src_width = src.cols;
	int src_height = src.rows;

	int mmid_width = k.cols / 2;
	int mmid_height = k.rows / 2;

	int sum_w = src_width + mmid_width;
	int sum_h = src_height + mmid_height;


	copyMakeBorder(src, src, 0, mmid_height, 0, mmid_width, cv::BORDER_CONSTANT, Scalar(0));
	t = ((double)getTickCount() - t)*1000. / getTickFrequency(); // -- step 1 :end ------
	cout << "step 1 transpose 180 and padd zeros run time : " << t << " ms." << endl;

#ifdef DEBUG
	cout << "  src_padded >>>>" << endl;
	printMat(src);

#endif // DEBUG
	t = getTickCount();     // -- step 2 : prepare kernel for dft ----
	auto mc_w = 1 + mmid_width;
	auto mc_h = 1 + mmid_height;

	Mat top_left_k = k(Rect(0, 0, mmid_width, mmid_height));
	Mat bottom_left_k = k(Rect(0, mmid_height, mmid_width, k.rows-mmid_height));
	Mat top_Right_k = k(Rect(mmid_width, 0, k.cols - mmid_width, mmid_height));
	Mat bottom_Right_k = k(Rect(mmid_width, mmid_height, k.cols - mmid_width, k.rows - mmid_height));

#ifdef DEBUG
	cout << "top Left :" << top_left_k << "Size :"<< top_left_k.size()<<endl;
	cout << "-------------" << endl;

	cout << "bottom_left_k :" << bottom_left_k << "Size :" << bottom_left_k.size() << endl;
	cout << "-------------" << endl;
	cout << "top_Right_k :" << top_Right_k << "Size :" << top_Right_k.size() << endl;
	cout << "-------------" << endl;
	cout << "bottom_Right_k :" << bottom_Right_k << "Size :" << bottom_Right_k.size() << endl;
	cout << "-------------" << endl;
#endif // DEBUG

	Mat k_new = Mat::zeros(src.size(), src.type());
	Rect top_left_k_new = Rect(0, 0, bottom_Right_k.cols, bottom_Right_k.rows);
	Rect top_Right_k_new = Rect(k_new.cols - bottom_left_k.cols, 0, bottom_left_k.cols, bottom_left_k.rows);
	Rect bottom_left_k_new = Rect(0, k_new.rows - 1, top_Right_k.cols, top_Right_k.rows);
	Rect bottom_Right_k_new = Rect(k_new.cols - top_left_k.cols, k_new.rows - top_left_k.rows, top_left_k.cols, top_left_k.rows);

	Mat roi1(k_new, top_left_k_new);
	add(roi1, bottom_Right_k, roi1);

	Mat roi2(k_new, top_Right_k_new);
	add(roi2, bottom_left_k,roi2);

	Mat roi3(k_new, bottom_left_k_new);
	add(roi3, top_Right_k, roi3);

	Mat roi4(k_new, bottom_Right_k_new);
	add(roi4, top_left_k, roi4);
	t = ((double)getTickCount() - t)*1000. / getTickFrequency(); // -- step 2 :end ------
	cout << "step 2 prepare kernel for dft run time : " << t << " ms." << endl;

#ifdef DEBUG
	cout << "  k_new >>>>" << endl;
	printMat(k_new);
#endif // DEBUG

	t = getTickCount();     // -- step 3 : calc DFT for src and kernel ----
	Mat dft_src = calc_dft(src);
	Mat dft_k = calc_dft(k_new);
	t = ((double)getTickCount() - t)*1000. / getTickFrequency(); // -- step 3 :end ------
	cout << "step 3 calc DFT for src and kernel run time : " << t << " ms." << endl;

	t = getTickCount();     // -- step 4 : multiply two complex from DFT ----
	Mat dft_y_test= calc_complex_multiply(dft_src, dft_k);
	t = ((double)getTickCount() - t)*1000. / getTickFrequency(); // -- step 4 :end ------
	cout << "step 4 multiply two complex from DFT run time : " << t << " ms." << endl;

	
#ifdef DEBUG
	cout << "dft_y_test :" << endl;
	printMat(dft_y_test);
#endif // DEBUG
	t = getTickCount();     // -- step 5 : IDFT ----
	Mat y;
	idft(dft_y_test, y, cv::DFT_SCALE | cv::DFT_REAL_OUTPUT );
	y = y(Rect(0, 0, src_width, src_height));
	t = ((double)getTickCount() - t)*1000. / getTickFrequency(); // -- step 5 :end ------
	cout << "step 5 IDFT run time : " << t << " ms." << endl;
#ifdef DEBUG	
	//cout << "y  = " << endl;
	//printMat(y);
	//cout << "Done!" << endl;
#endif
	return y;
}
// filter2D function
Mat filter_2d(Mat src, Mat k)
{
	Mat dst;

	filter2D(src, dst, src.depth(), k, Point(-1, -1), 0, BORDER_CONSTANT);
	return dst;
}
// native function
Mat native_conv(Mat src, Mat k)
{
	int src_width = src.cols;
	int src_height = src.rows;
	int k_width = k.cols;
	int k_height = k.rows;

	int mmid_width = k.cols / 2;
	int mmid_height = k.rows / 2;
	Mat y(src.size(), src.type());

	copyMakeBorder(src, src, mmid_height, mmid_height, mmid_width, mmid_width, cv::BORDER_CONSTANT, Scalar(0));

	for (int i = 1; i <= src_height; i++)
	{
		for (int j = 1; j <= src_width;j++)
		{
			auto tmp_sum = 0;
			for (int ki = 0; ki < k_height; ki++)
			{
				for (int kj = 0; kj < k_width;kj++)
				{
					tmp_sum += k.at<float>(ki, kj) * src.at<float>(i + ki - 1, j + kj - 1);
				}
			}
			y.at<float>(i - 1, j - 1) = tmp_sum;
		}
	}
	
	return y;

}
Mat compute_conv(Mat src, Mat kernel)
{
	Mat k;
	flip(kernel, k, -1);
	Size dftsize;
	dftsize.width = getOptimalDFTSize(src.cols + k.cols - 1);
	dftsize.height = getOptimalDFTSize(src.rows + k.rows - 1);

	Mat tempSrc(dftsize, src.type(), Scalar::all(0));
	Mat tempk(dftsize, k.type(), Scalar::all(0));

	Mat roiSrc(tempSrc, Rect(0, 0, src.cols, src.rows));
	src.copyTo(roiSrc);
	Mat roiK(tempk, Rect(0, 0, k.cols, k.rows));
	k.copyTo(roiK);

	cout << "tempSrc(before): " << endl;
	printMat(tempSrc);
	cout << "tempk (before):" << endl;
	printMat(tempk);

	dft(tempSrc, tempSrc, 0, src.rows);
	dft(tempk, tempk, 0, k.rows);

	cout << "tempSrc(after): " << endl;
	printMat(tempSrc);
	cout << "tempk (after):" << endl;
	printMat(tempk);


	mulSpectrums(tempSrc, tempk, tempSrc, DFT_COMPLEX_OUTPUT);
	idft(tempSrc, tempSrc, cv::DFT_SCALE | cv::DFT_REAL_OUTPUT, tempSrc.rows);
	//dft(tempSrc, tempSrc, DFT_INVERSE + DFT_SCALE, c.rows);
	Mat c(src.size(), src.type());
	tempSrc(Rect(0, 0, c.cols, c.rows)).copyTo(c);
	return c;
	
}
int WriteVideo(){
	VideoCapture cap(1);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 1280);//
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);//

	double fps = cap.get(CV_CAP_PROP_FPS);


	VideoWriter writer;
	writer.open("D:/jobs/Video.avi", CV_FOURCC('M', 'J', 'P', 'G'), fps, Size(704, 288), true);
	Mat frame;

	if (writer.isOpened())
	{
		return 1;
	}

	while (true)
	{
		cap >> frame;
		resize(frame, frame, Size(704, 288));
		if (frame.empty())break;

		imshow("src", frame);
		writer << frame;
		char c = waitKey(10);
		if (c == 27)
		{
			break;
		}
	}

	cap.release();
	writer.release();

	cout << "Writer end" << endl;

	VideoCapture cap1("D:/jobs/Video.avi");

	while (true)
	{
		cap1 >> frame;
		imshow("frame", frame);

		char c = waitKey(0);
		if (c == 27)
		{
			break;
		}

	}
	cap1.release();
	return 1;

	// create Mat to test
	Mat test(Size(3, 3), CV_32FC1);

	for (int i = 0; i < test.rows; i++)
	{
		for (int j = 0; j < test.cols; j++)
		{
			test.at<float>(i, j) = i * 10;
		}
	}

	for (int i = 0; i < test.rows; i++)
	{
		for (int j = 0; j < test.cols; j++)
		{
			cout << test.at<float>(i, j) << "  ";
		}
		cout << endl;
	}
	cout << "-----------" << endl;
	Mat m;
	Mat mask_m = test>10;
	test.copyTo(m, mask_m);

	for (int i = 0; i < test.rows; i++)
	{
		for (int j = 0; j < test.cols; j++)
		{
			cout << m.at<float>(i, j) << "  ";
		}
		cout << endl;
	}

}
int main(){

	// create Mat to test
	Mat test(Size(640, 480), CV_32FC1);

	for (int i = 0; i < test.rows; i++)
	{
		for (int j = 0; j < test.cols; j++)
		{
			test.at<float>(i, j) = i+1;
		}
	}
	if (test.rows<20 || test.cols <20)
		printMat(test);
	Mat k(Size(15,15),CV_32FC1);
	int indexs = 1;
	for (int i = 0; i < k.rows; i++, indexs++)
	{
		for (int j = 0; j < k.cols; j++, indexs++)
		{
			k.at<float>(i, j) = indexs;
		}
	}

	for (int i = 0; i < k.rows; i++)
	{
		for (int j = 0; j < k.cols; j++)
		{
			cout << k.at<float>(i, j) << "  ";
		}
		cout << endl;
	}
	cout << "---------------------------------" << endl;
	Mat native_y;
	cout << "native function result " << endl;	
	double t = getTickCount();
	native_y = native_conv(test, k);
	t = ((double)getTickCount() - t)*1000. / getTickFrequency();
	if (test.rows < 20 || test.cols < 20)
		printMat(native_y);
	cout << "native function  run time : " << t << " ms." << endl << endl;

	

	//cout << "---------------------------------" << endl;
	//cout << "conv_fft2 function result " << endl;
	//t = getTickCount();
	//Mat result =  paddMat(test, k);
	//t = ((double)getTickCount() - t)*1000. / getTickFrequency();
	////printMat(result);
	//cout << "conv_fft2  run time : " << t <<" ms."<< endl<<endl;

	//
	cout << "---------------------------------" << endl;
	cout << "filter2D function result " << endl;
	t = getTickCount();
	Mat filter_y = filter_2d(test, k);
	t = ((double)getTickCount() - t)*1000. / getTickFrequency();
	if (test.rows < 20 || test.cols < 20)
		printMat(filter_y);
	cout << "filter2D function  run time : " << t << " ms." << endl << endl;


	int f;// = isequal(native_y, result);
	/*if (f)
	{
		cout << "native result is equal to conv_fft2 result" << endl;
	}
	else
	{
		cout << "native result is not equal to conv_fft2 result" << endl;
	}
*/

	f = isequal(native_y, filter_y);
	if (f)
	{
		cout << "native result is equal to filter_y result" << endl;
	}
	else
	{
		cout << "native result is not equal to filter_y result" << endl;
	}


	/*cout << "---------------------------------" << endl;
	cout << "conv_dft function result " << endl;
	Mat y_conv = compute_conv(test, k);
	printMat(y_conv);
	*/

	 //printMat(k);


	return 1;

}