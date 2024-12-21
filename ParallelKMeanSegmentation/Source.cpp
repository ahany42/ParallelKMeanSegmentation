#include <iostream>
#include <math.h>
#include <mpi.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* InputImage(int* w, int* h, System::String^ imagePath)
{
	int* input;
	int OriginalImageWidth, OriginalImageHeight;
	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3);

		}

	}
	return input;
}


void CreateImage(int* image, int width, int height)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{

			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("C:\\Users\\Aly Hany\\Downloads\\Output"  + ".png");
	cout << "result Image Saved " <<endl;
}
int MinOfThree(int a, int b, int c) {
	if (a < c && a < b) return a;
	if (c < a && c < b) return c;
	return b;
}
void FindMinMax(int arr[], int size, int& min, int& max) {
	min = arr[0];
	max = arr[0];

	for (int i = 1; i < size; i++) {
		if (arr[i] < min) {
			min = arr[i];
		}
		if (arr[i] > max) {
			max = arr[i];
		}
	}

}
int* ImageGrayScaleSegmentation(int* OriginalLocalImage, int NumberOfPxs, int min, int max) {
	int* SegmentedImage = new int[NumberOfPxs];
	for (int i = 0; i < (NumberOfPxs); i++) {
		SegmentedImage[i] = OriginalLocalImage[i];
	}
	int clusters[3] = { min,(max + min) / 2,max };
	for (int i = 0; i < (NumberOfPxs); i++) {
		int DistanceToFirstCluster = abs(SegmentedImage[i] - clusters[0]);
		int DistanceToSecondCluster = abs(SegmentedImage[i] - clusters[1]);
		int DistanceToThirdCluster = abs(SegmentedImage[i] - clusters[2]);
		if (SegmentedImage[i] < clusters[2] / 3)
		{
			SegmentedImage[i] = clusters[0];
		}
		else if (SegmentedImage[i] < 2 * clusters[2] / 3) {
			SegmentedImage[i] = clusters[1];
		}
		else {
			SegmentedImage[i] = clusters[2];
		}
	}
	return SegmentedImage;
}
int main()
{
	MPI_Init(NULL, NULL);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int ImageWidth = 4, ImageHeight = 4;
	int start_s, stop_s;
	double TotalTime = 0.0;
	int* imageData;
	System::String^ imagePath;
	string img;
	img = "C:\\Users\\Aly Hany\\Downloads\\Input.jpg";
	imagePath = marshal_as<System::String^>(img);
	imageData = InputImage(&ImageWidth, &ImageHeight, imagePath);
	
	//Parallized Code
	start_s = clock();

	int localMin = INT_MAX;
	int localMax = INT_MIN;
	int globalMin;
	int globalMax;
	int segmentSize = (ImageHeight * ImageWidth) / size;
	int* localImage = new int[segmentSize];
	int* globalImage =nullptr;

	if (rank == 0) {
		globalImage = new int[ImageHeight * ImageWidth];
	}

	MPI_Scatter(imageData, segmentSize, MPI_INT, localImage, segmentSize, MPI_INT, 0, MPI_COMM_WORLD);
	FindMinMax(localImage, segmentSize, localMin, localMax);
	MPI_Allreduce(&localMin, &globalMin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
	MPI_Allreduce(&localMax, &globalMax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
	localImage = ImageGrayScaleSegmentation(localImage, segmentSize, globalMin, globalMax);
	MPI_Gather(localImage, segmentSize, MPI_INT, globalImage, segmentSize, MPI_INT, 0, MPI_COMM_WORLD);

	stop_s = clock();
	if (rank == 0) {
		CreateImage(globalImage, ImageWidth, ImageHeight);
		double ParallelTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "Time parallelized : " << ParallelTime << " ms" << endl;

		//Sequential Time
		start_s = clock();
		int min = 0;
		int max = 0;
		FindMinMax(imageData, ImageWidth * ImageHeight, min, max);
		ImageGrayScaleSegmentation(imageData, (ImageWidth * ImageHeight), min, max);
		stop_s = clock();
		double SequentialTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "Time Sequential : " << SequentialTime << " ms" << endl;
		cout << "Speed Up Using " << size << " Threads is (Sequential Time/ Parallel Time) " << SequentialTime / ParallelTime << endl;
	}
	
	free(imageData);

	MPI_Finalize();
	return 0;

}





