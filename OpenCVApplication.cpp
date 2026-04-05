// OpenCVApplication.cpp : Defines the entry point for the console application.
// JPEG Compression, Decompression and Compression Rate Program
// Sources: 
// https://www.tutorialspoint.com/dip/introduction_to_jpeg_compression.htm
// http://www0.cs.ucl.ac.uk/teaching/GZ05/07-images.pdf
//

#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>
#include <fstream>

wchar_t* projectPath;

/*
Converts a JPEG RGB image to JPEG YUV image.
*/
Mat convertToYUV(Mat mat) {
	Mat srcYUV;
	cvtColor(mat, srcYUV, COLOR_BGR2YCrCb);
	return srcYUV;
}

/*
Converts a JPEG YUV image to JPEG BGR.
*/
Mat convertToBGR(Mat src) {
	Mat dst;
	cvtColor(src, dst, COLOR_YCrCb2BGR);
	return dst;
}

/*
Pad the image to have it's width and height to the nearest multiple of 8.
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
Modify the range from [0, 255] to [0 + k, 255 + k] in a JPEG image.
*/
Mat modifyRange(Mat mat, float k = -128.0f) {
	Mat res = mat.clone();

	for (int row = 0; row < mat.rows; row += 8) {
		for (int col = 0; col < mat.cols; col += 8) {
			for (int i = 0; i < 8; ++i) {
				for (int j = 0; j < 8; ++j) {
					for (int c = 0; c < 3; ++c) {
						res.at<Vec3f>(row + i, col + j)[c] += k;
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
Compute the FDCT (forward discrete cosine transform) for an image.
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
Computes the inverse discrete cosine transform (IDCT) for a frequency domain.
*/
Mat computeIFDCTMatrix(Mat mat) {
	Mat res = Mat::zeros(mat.rows, mat.cols, CV_32FC3);
	float alpha_u = 0.0f, alpha_v = 0.0f, u = 0.0f, v = 0.0f, sum = 0.0f;

	for (int row = 0; row < mat.rows; row += 8) {
		for (int col = 0; col < mat.cols; col += 8) {
			for (int x = 0; x < 8; ++x) {
				for (int y = 0; y < 8; ++y) {
					for (int c = 0; c < 3; ++c) {
						sum = 0.0f;
						for (int u = 0; u < 8; ++u) {
							for (int v = 0; v < 8; ++v) {
								setAlpha(alpha_u, u);
								setAlpha(alpha_v, v);
								sum += alpha_u * alpha_v * mat.at<Vec3f>(row + u, col + v)[c] *
									cos(CV_PI / 8 * (x + 1.0f / 2.0f) * u) *
									cos(CV_PI / 8 * (y + 1.0f / 2.0f) * v);
							}
						}
						res.at<Vec3f>(row + x, col + y)[c] = sum;
					}
				}
			}
		}
	}

	return res;
}

/*
From the FDCT matrix, divide it with a given 8x8 quantization (luminance) matrix.
*/
Mat computeQuantizedMatrix(Mat fdctMatrix, Mat quantizationMatrix) {
	Mat res = Mat::zeros(fdctMatrix.rows, fdctMatrix.cols, CV_32FC3);

	for (int row = 0; row < fdctMatrix.rows; ++row) {
		for (int col = 0; col < fdctMatrix.cols; ++col) {
			for (int c = 0; c < 3; ++c) {
				res.at<Vec3f>(row, col)[c] = round(fdctMatrix.at<Vec3f>(row, col)[c] /
					quantizationMatrix.at<float>(row % 8, col % 8));
			}
		}
	}

	return res;
}

/*
From the quantized matrix, multiply it with a given 8x8 quantization (luminance) matrix.
*/
Mat computeDequantizedMatrix(Mat quantized, Mat quantizationMatrix) {
	Mat res = Mat::zeros(quantized.rows, quantized.cols, CV_32FC3);

	for (int row = 0; row < quantized.rows; ++row) {
		for (int col = 0; col < quantized.cols; ++col) {
			for (int c = 0; c < 3; ++c) {
				res.at<Vec3f>(row, col)[c] = round(quantized.at<Vec3f>(row, col)[c] *
					quantizationMatrix.at<float>(row % 8, col % 8));
			}
		}
	}

	return res;
}

/*
Compute the compression rate.
*/
float computeCompressionRate(Mat src, Mat quantizedMatrix) {
	int originalSize = src.rows * src.cols * 3;
	int compressedSize = 0;

	for (int row = 0; row < quantizedMatrix.rows; ++row) {
		for (int col = 0; col < quantizedMatrix.cols; ++col) {
			for (int c = 0; c < 3; ++c) {
				float val = quantizedMatrix.at<Vec3f>(row, col)[c];
				if (val != 0.0f) {
					compressedSize++;
				}
			}
		}
	}

	float compressionRate = (float)originalSize / (float)compressedSize;
	return compressionRate;
}

/*
Apply the run-length encoding algorithm.
*/
std::vector<std::pair<float, int>> runLengthEncode(std::vector<float> input) {
	std::vector<std::pair<float, int>> encoded;
	if (input.empty()) return encoded;

	float current = input[0];
	int count = 1;

	for (int i = 1; i < input.size(); ++i) {
		if (input[i] == current) {
			count++;
		}
		else {
			encoded.push_back({ current, count });
			current = input[i];
			count = 1;
		}
	}

	encoded.push_back({ current, count });
	return encoded;
}

std::vector<std::pair<int, int>> generateZigZagOrder() {
	std::vector<std::pair<int, int>> order;

	for (int s = 0; s <= 14; ++s) {
		for (int i = 0; i <= s; ++i) {
			int j = s - i;
			if (i < 8 && j < 8) {
				if (s % 2 == 0)
					order.push_back({ i, j });
				else
					order.push_back({ j, i });
			}
		}
	}

	return order;
}

std::vector<float> extractZigZagBlock(Mat block, int channel) {
	std::vector<float> result;
	std::vector<std::pair<int, int>> zigzag = generateZigZagOrder();

	for (std::pair<int, int> p : zigzag) {
		result.push_back(block.at<Vec3f>(p.first, p.second)[channel]);
	}

	return result;
}

void encodeQuantizedMatrix(Mat quantizedMatrix, std::string fileName) {
	std::ofstream out(fileName);
	if (!out.is_open()) {
		std::cerr << "Cannot open file: " << fileName << "\n";
		return;
	}

	Mat block;
	std::vector<std::pair<int, int>> zigzag = generateZigZagOrder();
	std::vector<float> zigzagVals;
	std::vector<std::pair<float, int>> rle;

	for (int row = 0; row < quantizedMatrix.rows; row += 8) {
		for (int col = 0; col < quantizedMatrix.cols; col += 8) {
			for (int c = 0; c < 3; ++c) {
				block = quantizedMatrix(Rect(col, row, 8, 8));
				zigzagVals = extractZigZagBlock(block, c);
				rle = runLengthEncode(zigzagVals);

				out << "[" << row << "," << col << "," << c << "] -> ";
				for (std::pair<int, int> p : rle) {
					out << p.first << ":" << p.second << " ";
				}
				out << "\n";
			}
		}
	}

	out.close();
}

Mat decodeQuantizedMatrix(std::string fileName, int rows, int cols) {
	Mat quantizedMatrix = Mat::zeros(rows, cols, CV_32FC3);
	std::ifstream in(fileName);
	if (!in.is_open()) {
		std::cerr << "Cannot open file: " << fileName << "\n";
		return quantizedMatrix;
	}

	std::vector<std::pair<int, int>> zigzag = generateZigZagOrder();
	std::string line;

	while (std::getline(in, line)) {
		if (line.empty()) continue;

		int row, col, channel;
		sscanf(line.c_str(), "[%d,%d,%d]", &row, &col, &channel);

		int arrowPos = line.find("->");
		if (arrowPos == std::string::npos) continue;

		std::string rlePart = line.substr(arrowPos + 2);
		std::istringstream iss(rlePart);
		std::string token;

		std::vector<float> values;
		while (iss >> token) {
			int sep = token.find(":");
			if (sep == std::string::npos) continue;

			float val = std::stof(token.substr(0, sep));
			int count = std::stoi(token.substr(sep + 1));
			values.insert(values.end(), count, val);
		}

		for (int i = 0; i < values.size() && i < zigzag.size(); ++i) {
			quantizedMatrix.at<Vec3f>(row + zigzag[i].first, col + zigzag[i].second)[channel] = values[i];
		}
	}

	in.close();
	return quantizedMatrix;
}

/*
Apply the JPEG Compression algorithm
*/
void testJPEGCompression() {
	Mat src, padded, yuv, shifted, fdct, quantized, decoded, dequantized, idct, reconstructed;
	Mat compressed, finalImg;
	char fname[MAX_PATH];
	float rate, reduction;
	
	// Example luminance matrix
	float Q50[8][8] = {
		{16,11,10,16,24,40,51,61},
		{12,12,14,19,26,58,60,55},
		{14,13,16,24,40,57,69,56},
		{14,17,22,29,51,87,80,62},
		{18,22,37,56,68,109,103,77},
		{24,35,55,64,81,104,113,92},
		{49,64,78,87,103,121,120,101},
		{72,92,95,98,112,100,103,99}
	};
	Mat quantizationMatrix(8, 8, CV_32F, Q50);
	std::string fileName = "rle.txt";

	while (openFileDlg(fname)) {
		src = imread(fname, IMREAD_COLOR);
		yuv = convertToYUV(src);
		padded = padImageMultiple8(yuv);
		padded.convertTo(padded, CV_32FC3);
		shifted = modifyRange(padded);

		fdct = computeFDCTMatrix(shifted);

		quantized = computeQuantizedMatrix(fdct, quantizationMatrix);
		encodeQuantizedMatrix(quantized, fileName);
		decoded = decodeQuantizedMatrix(fileName, fdct.rows, fdct.cols);

		dequantized = computeDequantizedMatrix(quantized, quantizationMatrix);

		idct = computeIFDCTMatrix(dequantized);

		idct = modifyRange(idct, 128.0f);

		idct.convertTo(idct, CV_8UC3);
		reconstructed = convertToBGR(idct);
		//finalImg = reconstructed(Rect(0, 0, src.cols, src.rows));

		imshow("Original Image", src);

		normalize(quantized, compressed, 0, 255, NORM_MINMAX);
		compressed.convertTo(compressed, CV_8UC3);
		imshow("Compressed Image", compressed);
		imwrite("compressed.jpeg", compressed);

		imshow("Reconstructed Image", reconstructed);
		imwrite("reconstructed.jpeg", reconstructed);

		rate = computeCompressionRate(src, quantized);
		reduction = 100.0f * (1.0f - 1.0f / rate);

		printf("\n--- Compression ---\n");
		printf("Compression Rate: %.2f : 1 (original : compressed)\n", rate);
		printf("Size Reduction: %.2f%%\n", reduction);

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
		printf(" 1 - Test JPEG Compression algorithm\n");
		printf(" 0 - Exit\n\n");
		printf("Option: ");
		scanf("%d", &op);
		switch (op)
		{
		case 1:
			testJPEGCompression();
			break;
		}
	} while (op != 0);
	return 0;
}
