#include "../PM_type.h"

// takes a float color image and a bin size 
// returns HOG features
//Mat		features( const Mat &image, const int sbin );

//--------------------------------------------------------
// small value, used to avoid division by zero
#ifndef eps
#define eps	0.0001f
#endif

// unit vectors used to compute gradient orientation
const float	uu[9] = {1.0000f, 0.9397f, 0.7660f, 0.500f, 0.1736f, -0.1736f, -0.5000f, -0.7660f, -0.9397f};
const float	vv[9] = {0.0000f, 0.3420f, 0.6428f, 0.8660f, 0.9848f, 0.9848f, 0.8660f, 0.6428f, 0.3420f};

#ifndef	MIN
#define MIN(x,y)	 (x<y?x:y)
#endif
#ifndef MAX
#define MAX(x,y)	 (x>y?x:y)
#endif

namespace PM_type{

	// ��Ϊfeatpyramid()�жԸ���pyra.feat[i]����0/1��չ�ķ��˴���ʱ�䣬Ϊ�˼�С�����֧������ֱ�ӶԼ����feature��չ
	Mat		features( const Mat &image, const int sbin, const int pad[2] )
	{
		const int	dims[3] = { image.rows, image.cols, image.channels() };
		const int	row_step = image.cols * image.channels();
		const float * const imgpt = image.ptr<float>(0,0);
		if( image.type()!=CV_32FC3 )
			throw runtime_error("Invalid input");

		// memory for caching orientation histograms & their norms
		const int	blocks[2] = {  (int)(0.5f+(float)dims[0]/(float)sbin), (int)(0.5f+(float)dims[1]/(float)sbin) };
		const int	block_sz = blocks[0] * blocks[1];
		const int	visible[2] = { blocks[0]*sbin, blocks[1]*sbin };

		Mat		histmat[18];
		float		*hist[18];	
		int			hist_step[18];
		Mat		hist_tmp( blocks[0], blocks[1]*18, CV_32FC1, Scalar(0) );
		for( int i=0; i<18; i++ ){
			//histmat[i] = Mat::zeros( blocks[0], blocks[1], CV_32FC1 );
			histmat[i] = hist_tmp( Rect(i*blocks[1],0,blocks[1],blocks[0]) );
			hist[i] = histmat[i].ptr<float>(0,0);
			hist_step[i] = histmat[i].step1();
		}
		
		// 1. Calculate the original HOG
		for (int yy = 1; yy < visible[0]-1; yy++) { // each row
			int		y = MIN(yy,dims[0]-2);
			const float	*imgpt1 = imgpt + y*row_step;
			for (int xx = 1; xx < visible[1]-1; xx++) { // each col
				int		x = MIN(xx,dims[1]-2);

				// b-g-r color at (y,x) is { *s, *(s+1), *(s+2) }
				const float	*s = imgpt1 + x*dims[2];
				float Right[3] = { *(s+3), *(s+4), *(s+5) }; 
				float Left[3] = { *(s-3), *(s-2), *(s-1) };
				float Up[3] = { *(s-row_step), *(s+1-row_step), *(s+2-row_step) };
				float Down[3] = { *(s+row_step), *(s+1+row_step), *(s+2+row_step) };

				// gradients and amplitude of orientations of each channel
				float	dxs, dys, vs;
				float	dx, dy, v=-1;
				for( int i=0; i!=3; i++ ){
					dxs = Right[i] - Left[i];
					dys = Down[i] - Up[i];
					vs = dxs*dxs + dys*dys;
					if( vs>=v ){
						v = vs;
						dx = dxs;
						dy = dys;
					}
				}

				// snap to one of 18 orientations
				float best_dot = 0;
				int best_o = 0;
				for (int o = 0; o < 9; o++) {
					float dot = uu[o]*dx + vv[o]*dy;
					if (dot > best_dot) {
						best_dot = dot;
						best_o = o;
					} else if (-dot > best_dot) {
						best_dot = -dot;
						best_o = o+9;
					}
				}

				// add to 4 histograms around pixel using linear interpolation
				float xp = (xx+0.5f)/(float)sbin - 0.5f;
				float yp = (yy+0.5f)/(float)sbin - 0.5f;
				//int ixp = (int)floorf(xp),  iyp = (int)floorf(yp);
				int ixp = (int)xp, iyp = (int)yp;
				if( xp<0 && xp!=ixp )
					ixp--;
				if( yp<0 && yp!=iyp )
					iyp--;
				int ixp_p = ixp + 1, iyp_p = iyp + 1;
				float vx0 = xp-ixp, vy0 = yp-iyp;
				float vx1 = 1.f-vx0, vy1 = 1.f-vy0;
				v = sqrtf(v);

				if( ixp<0 ){
					ixp = 0;
					vx1 = 0;
				}
				if( ixp_p>=blocks[1] ){
					ixp_p = blocks[1] - 1;
					vx0 = 0;
				}
				if( iyp<0 ){
					iyp = 0;
					vy1 = 0;
				}
				if( iyp_p>=blocks[0] ){
					iyp_p = blocks[0] - 1;
					vy0 = 0;
				}

				float	*hist_pt = hist[best_o] + iyp*hist_step[best_o] + ixp;
				ixp_p = ixp_p - ixp;
				iyp_p = iyp_p - iyp;
				*(hist_pt) += vx1*vy1*v;
				*(hist_pt + ixp_p) += vx0*vy1*v;
				*(hist_pt + iyp_p*hist_step[best_o]) += vx1*vy0*v;
				*(hist_pt + iyp_p*hist_step[best_o] + ixp_p) += vx0*vy0*v;
			}
		}

		// 2. Compute energy in each block by summing over orientations
		Mat		normmat( blocks[0], blocks[1], CV_32FC1, Scalar(0) );
		float * const norm = normmat.ptr<float>(0,0);
		const int	norm_step = normmat.step1();

		Mat	histmat_insens[9]; // insensitive features
		float	*histinsens[9];
		int		histinsens_step[9];
		for( int i=0; i<9; i++ ){
			histmat_insens[i] = histmat[i] + histmat[i+9];
			accumulateSquare( histmat_insens[i], normmat );
			histinsens[i] = histmat_insens[i].ptr<float>(0,0);
			histinsens_step[i] = histmat_insens[i].step1();
		}

		// normalization coefficients
		Mat	Normmss(blocks[0]-1,blocks[1]-1,CV_32FC1);
		float * const Norms = Normmss.ptr<float>(0,0);
		const int	norms_step = Normmss.step1();
		for( int i=0; i<Normmss.rows; i++ ){
			float	*src = norm + i*norm_step;
			float	*dst = Norms + i*norms_step;
			for( int j=0; j<Normmss.cols; j++ ){
				float	tmp = *src + *(src+1) + *(src+norm_step) + *(src+norm_step+1) + eps;
				//*(dst++) = 1.f / sqrtf(tmp);
				*(dst++) = tmp;
				src++;
			}
		}
		cv::sqrt( Normmss, Normmss );
		Normmss = 1.f / Normmss;

		// 3. Reduced HOG features.
		int out[3];
		out[0] = MAX(blocks[0]-2, 0);
		out[1] = MAX(blocks[1]-2, 0);
		out[2] = 27+4+1; // 32 dimensional features
		Vec<float,32>	init_val;
		assert( out[2]==32 );
		init_val = 0;
		init_val[out[2]-1] = 1;
		Mat	feat_mat( out[0]+2*pad[0], out[1]+2*pad[1], CV_32FC(out[2]) );
		feat_mat.setTo( init_val );
		float * const feat_data = feat_mat.ptr<float>(pad[0],pad[1]);
		const int feat_step = feat_mat.step1();

		for( int y=0; y<out[0]; y++ ){
			float	*dst = feat_data + y*feat_step;
			float	*Normpt = Norms + y*norms_step;
			float	*histinsens0[9];
			for( int i=0; i<9; i++ )
				histinsens0[i] = histinsens[i] + (y+1)*histinsens_step[i];
			for( int x=0; x<out[1]; x++ ){
				// 			
				float	n4 = *Normpt;
				float	n2 = *(Normpt+1);
				float	n3 = *(Normpt+norms_step);
				float	n1 = *(Normpt+norms_step+1);
				Normpt++;

				// 18 contrast-sensitive features
				float	t1 = 0, t2 = 0, t3 = 0, t4 = 0;
				for( int i=0; i<18; i++ ){
					float	src = *( hist[i]+(y+1)*hist_step[i]+x+1 );
					float h1 = MIN(src * n1, 0.2f);
					float h2 = MIN(src * n2, 0.2f);
					float h3 = MIN(src * n3, 0.2f);
					float h4 = MIN(src * n4, 0.2f);
					*(dst++) = 0.5f * (h1 + h2 + h3 + h4);
					t1 += h1;
					t2 += h2;
					t3 += h3;
					t4 += h4;
				}

				// 9 contrast-insensitive features
				for( int i=0; i<9; i++ ){
					float	*src = ++histinsens0[i];
					//float	src = histmat_insens[i].at<float>(y+1,x+1);
					float h1 = MIN(*src * n1, 0.2f);
					float h2 = MIN(*src * n2, 0.2f);
					float h3 = MIN(*src * n3, 0.2f);
					float h4 = MIN(*src * n4, 0.2f);
					*(dst++) = 0.5f * (h1 + h2 + h3 + h4);
				}

				// 4 texture features
				*(dst++) = 0.2357f * t1;
				*(dst++) = 0.2357f * t2;
				*(dst++) = 0.2357f * t3;
				*(dst++) = 0.2357f * t4;

				// 1 truncation feature			
				*(dst++) = 0;

			}
		}

		return	feat_mat;
	}

}

/* ��μ���

	1. ʹ�ú꣨MIN,MAX����ʹ����������Ҫ��
	2. һ�η������ݿռ�ȶ�η������ݿռ�Ҫ�죺
		a. ���ͬ����С�ľ���--һ�η���һ����Ŀռ䣬ÿ������ֱ������ÿռ��һ���ӿ�
		b. �����ڲ�ʹ�õ��м���������С���䣬����������̬�����������С��䣬���������ߣ�����ʹ�þ�̬vector
	3. ��׼��ģ��stl���ṩ��һЩ���ú������� floorf(),sqrtf()��������Щ������Ч�ʲ��ߣ�����ʹ�ûᵼ�º��������½���
		������ԣ�����ʹ�����ͽ�ȡ(int())������floorf��ʹ��openCV�ṩ��cv::sqrt������һ�δ���������ݵ�sqrtf()������
			���⣬std::sqrtf()����Ҫ��std::sqrt()����Ҫ�죨�ڴ����ظ������£����Կ������Բ��죩��
	4. ������ʹ��ָ�����--ָ�������������ڴ���ʷ�����Ҫ����ʹ��openCV�ṩ�ľ���Ԫ�ػ�ȡ�ĺ�����

*/