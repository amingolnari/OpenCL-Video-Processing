
// You might need to change this header based on your install:

#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <time.h>
#include <windows.h>
#include <cstring>
#include <vector>
#include <math.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv\cv.hpp>
#include <opencv2\videoio\videoio.hpp>
#include <opencv.hpp>

#ifdef _DEBUG
#pragma comment (lib, "opencv_world330d.lib")
#else
#pragma comment (lib, "opencv_world330.lib")
#endif


#define FILEname "Kayak_HD.mp4" // "D:\\Learn English - Documents\\01.Intro\\ActionVocabulary1.mp4"
#define KERNEL "videoProc"
#define KERNELFILE "videoProc.cl"
#define NUMofFRAME 500
/*
#define SUCCESS 0
#define FAILURE 1
*/


using namespace std;
using namespace cv;

#pragma  comment(lib, "OpenCl.lib")
static void check_error(cl_int error, char* name) {
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Non-successful return code %d for %s.  Exiting.\n", error, name);
		return;
	}
}

//in second
inline double StopWatch(int start0stop1 = 0, bool showMessage = false)
{
	static LARGE_INTEGER swFreq = { 0, 0 }, swStart, swStop;
	static const double TwoPow32 = pow(2.0, 32.0);
	if (!swFreq.LowPart)
		QueryPerformanceFrequency(&swFreq);
	double result = -1;
	if (start0stop1 == 0)	QueryPerformanceCounter(&swStart);
	else {
		QueryPerformanceCounter(&swStop);
		if (swFreq.LowPart == 0 && swFreq.HighPart == 0) return -1;
		else {
			result = (double)((swStop.HighPart - swStart.HighPart)*TwoPow32 + swStop.LowPart - swStart.LowPart);
			if (result < 0) result += TwoPow32;
			result /= (swFreq.LowPart + swFreq.HighPart*TwoPow32);
		}
		if (showMessage) {
			char s[25] = { 0 };
			sprintf(s, "Time (s): %.4f", result);
			MessageBox(NULL, s, "Elapsed Time", 0);
		}
	}
	return result;
}


int main() {
	/* CPU 0 and GPU 1*/
#if 1
	/* GPU */
	cl_platform_id plat[2];
	cl_device_id dev;
	cl_context ctx;
	cl_command_queue comq;
	cl_program prog;
	cl_kernel kernel;
	cl_int err;
	size_t global_size[2];
	size_t workunit = 64;
	cl_mem inBuf, outBuf;
	FILE *prog_handle;
	size_t prog_size;
	char *prog_buf;
	size_t localws[2] = { 32, 32 };
	size_t origin[3], region[3];

	/* Image data */
	clGetPlatformIDs(2, plat, NULL);
	clGetDeviceIDs(plat[0], CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
	ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
	if (err < 0) {
		printf("\n Err : %d", err);
		getchar();
		return -1;
	}
	prog_handle = fopen(KERNELFILE, "rb");
	if (prog_handle == NULL) {
		printf("File can not open ... !! ");
		getchar();
		return -1;
	}
	fseek(prog_handle, 0, SEEK_END);
	prog_size = ftell(prog_handle);
	fseek(prog_handle, 0, SEEK_SET);
	prog_buf = (char*)malloc(prog_size + 1);
	prog_buf[prog_size] = '\0';
	fread(prog_buf, sizeof(char), prog_size, prog_handle);
	// printf("\n\n\t%s", prog_buf);
	fclose(prog_handle);

	prog = clCreateProgramWithSource(ctx, 1, (const char**)&prog_buf,
		&prog_size, &err);
	if (err < 0) {
		printf("\n Err : %d", err);
		getchar();
		return -1;
	}
	err = clBuildProgram(prog, 0, NULL, NULL, NULL, NULL);
	free(prog_buf);
	if (err < 0)
	{
		size_t log_size = 0;
		clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 0, 0, &log_size);
		char* log_str = (char*)malloc(log_size + 1);
		clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, log_size + 1, log_str, 0);
		printf("LOG FILE contains:\n%s", log_str);
		getchar();
		return -1;
	}

	kernel = clCreateKernel(prog, KERNEL, &err);
	if (err < 0) {
		printf("Couldn't create a kernel: %d", err);
		getchar();
		return -1;
	}
	comq = clCreateCommandQueue(ctx, dev, 0, &err);
	if (err < 0) {
		perror("Couldn't create a command queue");
		getchar();
		return -1;
	}

	/* VideoCapture */
	cv::Mat input, output;
	cv::VideoCapture caP(FILEname);
	if (!caP.isOpened()) {
		printf(" Video Can Not Opened ... !!");
		getchar();
		return -1;
	}
	int cunt = 0;

	double rate = caP.get(CV_CAP_PROP_FPS);
	int Width = caP.get(CV_CAP_PROP_FRAME_WIDTH);
	int Height = caP.get(CV_CAP_PROP_FRAME_HEIGHT);
	int dly = 1000 / rate;
	printf("\n FRAME WIDTH : %d \n FRAME HEIGHT : %d \n dly : %d", Width, Height, dly);
	printf("\n press esc to exit ");
	cl_image_format img_format;
	cl_mem inputIm, outputIm;
	img_format.image_channel_order = CL_RGBA;
	// img_format.image_channel_order = CL_LUMINANCE; /*LUMINANCE for Gray Scale*/
	img_format.image_channel_data_type = CL_UNORM_INT8;
	origin[0] = 0; origin[1] = 0; origin[2] = 0;
	region[0] = Width; region[1] = Height; region[2] = 1;
	global_size[0] = Width; global_size[1] = Height;

	StopWatch(0);
	outputIm = clCreateImage2D(ctx,
		CL_MEM_WRITE_ONLY, &img_format, Width, Height, 0, NULL, &err);
	caP >> input;
	if (input.empty()) {
		printf(" Capture Is Empty ... !!");
		getchar();
		return -1;
	}
	cvtColor(input, input, CV_BGR2RGBA);
	output = input.clone();

	inputIm = clCreateImage2D(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&img_format, Width, Height, 0, (void*)input.data, &err);
	if (err < 0) {
		perror("Couldn't create the image object");
		getchar();
		return -1;
	}
	printf("\n\n***   GPU   ***");
	while (cunt < NUMofFRAME) {
		caP >> input;
		if (input.empty()) {
			printf(" Capture Is Empty ... !!");
			getchar();
			return -1;
		}
		namedWindow("Input Capture", 0);
		cv::imshow("Input Capture", input);
		cvtColor(input, input, CV_BGR2RGBA);
		//cvtColor(output, output, CV_RGB2GRAY);
		//cvtColor(input, input, CV_RGB2GRAY);
		clEnqueueWriteImage(comq, inputIm, CL_TRUE, origin,
			region, 0, 0, (void*)input.data, 0, NULL, NULL);

		err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputIm);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputIm);
		if (err < 0) {
			printf("Couldn't set a kernel argument");
			getchar();
			return -1;
		}

		err = clEnqueueNDRangeKernel(comq, kernel, 2, NULL, global_size,
			NULL, 0, NULL, NULL);
		if (err < 0) {
			perror("Couldn't enqueue the kernel");
			getchar();
			return -1;
		}

		err = clEnqueueReadImage(comq, outputIm, CL_TRUE, origin,
			region, 0, 0, (void*)output.data, 0, NULL, NULL);
		if (err < 0) {
			perror("Couldn't read from the image object");
			getchar();
			return -1;
		}

		clFinish(comq);
		namedWindow("Output Capture", 0);
		cv::imshow("Output Capture", output);
		cv::waitKey(1);
		cunt++;
	}

	clReleaseMemObject(inputIm);
	clReleaseMemObject(outputIm);
	clReleaseCommandQueue(comq);
	clReleaseKernel(kernel);
	clReleaseProgram(prog);
	clReleaseContext(ctx);
	StopWatch(1, 1);
#else
/* VideoCapture */
	cv::Mat input, output;
	cv::VideoCapture caP(FILEname);
	if (!caP.isOpened()) {
		printf(" Video Can Not Opened ... !!");
		getchar();
		return -1;
	}
	int cunt = 0;

	double rate = caP.get(CV_CAP_PROP_FPS);
	int Width = caP.get(CV_CAP_PROP_FRAME_WIDTH);
	int Height = caP.get(CV_CAP_PROP_FRAME_HEIGHT);
	int dly = 1000 / rate;
	printf("\n FRAME WIDTH : %d \n FRAME HEIGHT : %d \n dly : %d", Width, Height, dly);
	printf("\n press esc to exit  \n");
	float sobx[3][3] = { {1, 0, -1}, {2, 0, -2}, {1, 0, -1} };
	float soby[3][3] = { {1, 2, 1}, {0, 0, 0}, {-1, -2, -1} };

	float pix1[3], pix2[3], pix[3];
	int k1, k2;
	printf("\n\n***   CPU   ***");
	StopWatch(0);
	while (cunt < NUMofFRAME) {
		caP >> input;
		if (input.empty()) {
			printf(" Capture Is Empty ... !!");
			getchar();
			return -1;
		}
		//cvtColor(input, input, CV_RGB2GRAY);
		output = input.clone();
		for (int w = 1; w < input.rows-1; w++) {
			for (int h = 1; h < input.cols-1; h++) {
				k2 = 0;
				pix1[0] = 0.0f, pix1[1] = 0.0f, pix1[2] = 0.0f;
				pix2[0] = 0.0f, pix2[1] = 0.0f, pix2[2] = 0.0f;
				for (int i = w-1; i < w+2; i++) {
					k1 = 0;
					for (int j = h-1; j < h+2; j++) {
						//pix1 += (input.at<uchar>(i, j) * sobx[k1][k2]);
						//pix2 += (input.at<uchar>(i, j) * soby[k1][k2]);
						pix1[0] += (input.at<Vec3b>(i, j)[0] * sobx[k1][k2]);
						pix2[0] += (input.at<Vec3b>(i, j)[0] * soby[k1][k2]);
						pix1[1] += (input.at<Vec3b>(i, j)[1] * sobx[k1][k2]);
						pix2[1] += (input.at<Vec3b>(i, j)[1] * soby[k1][k2]);
						pix1[2] += (input.at<Vec3b>(i, j)[2] * sobx[k1][k2]);
						pix2[2] += (input.at<Vec3b>(i, j)[2] * soby[k1][k2]);
						k1++;
					}
					k2++;
				}
				pix[0] = cv::max(pix1[0], pix2[0]);
				pix[1] = cv::max(pix1[1], pix2[1]);
				pix[2] = cv::max(pix1[2], pix2[2]);
				output.at<Vec3b>(w, h)[0] = cv::max(pix[0], 0.0f);
				output.at<Vec3b>(w, h)[1] = cv::max(pix[1], 0.0f);
				output.at<Vec3b>(w, h)[2] = cv::max(pix[2], 0.0f);
				//output.at<uchar>(w, h) = cv::max(pix, 0.0f);
			}
		}

		namedWindow("Input Capture", 0);
		cv::imshow("Input Capture", input);
		namedWindow("Output Capture", 0);
		cv::imshow("Output Capture", output);
		cv::waitKey(1);
		cunt++;
	}
	StopWatch(1, 1);
#endif
	//getchar();
	return 0;
}