// OpenCVApplication.cpp : Defines the entry point for the console application.
// JPEG Compression, Decompression and Compression Rate Program
// Sources: 
// https://www.tutorialspoint.com/dip/introduction_to_jpeg_compression.htm
// http://www0.cs.ucl.ac.uk/teaching/GZ05/07-images.pdf
//

#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>

wchar_t* projectPath;

/*
Converts a JPEG BGR image to JPEG YUV image
*/
Mat convertToYUV(Mat mat) {
	Mat srcYUV;
	cvtColor(mat, srcYUV, COLOR_BGR2YCrCb);
	return srcYUV;
}

/*
Pad the image to have it's width and height to the nearest multiple of 8
*/
Mat padImageMultiple8(Mat mat) {
	Mat srcFloat, dst;
	int rows = (mat.rows + 7) / 8 * 8;
	int cols = (mat.cols + 7) / 8 * 8;

	mat.convertTo(srcFloat, CV_32F);
	copyMakeBorder(srcFloat, dst, 0, rows - mat.rows, 0, cols - mat.cols, BORDER_CONSTANT, 0);

	return dst;
}

/*
Modify the range from [0, 255] to [-128, 127] in a JPEG image
*/
Mat modifyRange(Mat mat) {
	Mat res = mat.clone();

	for (int row = 0; row < mat.rows; row += 8) {
		for (int col = 0; col < mat.cols; col += 8) {
			for (int i = 0; i < 8; ++i) {
				for (int j = 0; j < 8; ++j) {
					for (int c = 0; c < 3; ++c) {
						res.at<Vec3f>(row + i, col + j)[c] -= 128.0f;
					}
				}
			}
		}
	}

	return res;
}

/*
Util function for computing the FDCT for an image. Sets the alpha value at a given index.
*/

void setAlpha(float& a, int i) {
	if (i == 0) {
		a = sqrt(1.0f / 8.0f);
	}
	else {
		a = sqrt(2.0f / 8.0f);
	}
}

/*
Compute the FDCT (forward discrete cosine transform) for an image
*/

Mat computeFDCTMatrix(Mat mat) {
	Mat res = Mat::zeros(mat.rows, mat.cols, CV_32FC3);
	float alpha_u = 0.0f, alpha_v = 0.0f, u = 0.0f, v = 0.0f, sum = 0.0f;

	for (int row = 0; row < mat.rows; row += 8) {
		for (int col = 0; col < mat.cols; col += 8) {
			for (int u = 0; u < 8; ++u) {
				for (int v = 0; v < 8; ++v) {
					setAlpha(alpha_u, u);
					setAlpha(alpha_v, v);

					for (int c = 0; c < 3; ++c) {
						sum = 0.0f;
						for (int x = 0; x < 8; ++x) {
							for (int y = 0; y < 8; ++y) {
								sum += mat.at<Vec3f>(row + x, col + y)[c] * 
									cos(CV_PI / 8 * (x + 1.0f / 2.0f) * u) * 
									cos(CV_PI / 8 * (y + 1.0f / 2.0f) * v);
							}
						}
						res.at<Vec3f>(row + u, col + v)[c] = alpha_u * alpha_v * sum;
					}
				}
			}
		}
	}

	return res;
}

/*
From the FDCT matrix, divide it with a given 8x8 quantization (luminance) matrix
*/

Mat computeQuantizedMatrix(Mat fdctMatrix, Mat quantizationMatrix) {
	Mat res = Mat::zeros(fdctMatrix.rows, fdctMatrix.cols, CV_32FC3);
	
	for (int row = 0; row < fdctMatrix.rows; ++row) {
		for (int col = 0; col < fdctMatrix.cols; ++col) {
			for (int c = 0; c < 3; ++c) {
				res.at<Vec3f>(row, col)[c] = round(fdctMatrix.at<Vec3f>(row, col)[c] / 
					quantizationMatrix.at<Vec3f>(row % 8, col % 8)[c]);
			}
		}
	}

	return res;
}

void testModifyRange() {
	Mat src, dst;
	char fname[MAX_PATH];

	while (openFileDlg(fname)) {
		src = imread(fname, IMREAD_COLOR);
		dst = modifyRange(padImageMultiple8(convertToYUV(src)));
		waitKey(0);
	}
}

void testComputeFDCTMatrix() {
	Mat src, dst;
	char fname[MAX_PATH];

	while (openFileDlg(fname)) {
		src = imread(fname, IMREAD_COLOR);
		dst = computeFDCTMatrix(modifyRange(padImageMultiple8(convertToYUV(src))));
		waitKey(0);
	}
}

void testComputeQuantizedMatrix() {
	Mat src, dst, quantizationMatrix;
	char fname[MAX_PATH];
	quantizationMatrix = Mat::ones(8, 8, CV_32FC3);

	while (openFileDlg(fname)) {
		src = imread(fname, IMREAD_COLOR);
		dst = computeQuantizedMatrix(computeFDCTMatrix(modifyRange(padImageMultiple8(convertToYUV(src)))), quantizationMatrix);
		waitKey(0);
	}
}

int main() 
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);
    projectPath = _wgetcwd(0, 0);

	int op;
	do
	{
		system("cls");
		destroyAllWindows();
		printf("Menu:\n");
		printf(" 1 - Test modifyRange\n");
		printf(" 2 - Test computeFDCTMatrix\n");
		printf(" 3 - Test computeQuantizationMatrix\n");
		printf(" 0 - Exit\n\n");
		printf("Option: ");
		scanf("%d",&op);
		switch (op)
		{
			case 1:
				testModifyRange();
				break;
			case 2:
				testComputeFDCTMatrix();
				break;
			case 3:
				testComputeQuantizedMatrix();
				break;
		}
	}
	while (op!=0);
	return 0;
}