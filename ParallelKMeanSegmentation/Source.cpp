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
	int clusters[3];
	//intially random number between min and max
	clusters[0] = rand() % max + min ;	
	clusters[1] = rand() % max + min ;
	clusters[2] = rand() % max + min ;
	int previousClusters[3] = { 0,0,0 };
	int sum[3] = { 0, 0, 0 };
	int count[3] = { 0, 0, 0 };
	int iterations = 100;
	for (int i = 0; i < iterations; i++) {
		for (int j = 0; j < NumberOfPxs; j++) {
			int DistanceToFirstCluster = abs(OriginalLocalImage[j] - clusters[0]);
			int DistanceToSecondCluster = abs(OriginalLocalImage[j] - clusters[1]);
			int DistanceToThirdCluster = abs(OriginalLocalImage[j] - clusters[2]);

			if (DistanceToFirstCluster <= DistanceToSecondCluster && DistanceToFirstCluster <= DistanceToThirdCluster) {
				OriginalLocalImage[i] = clusters[0];
				sum[0] += OriginalLocalImage[i];
				count[0]++;
			}
			else if (DistanceToSecondCluster <= DistanceToFirstCluster && DistanceToSecondCluster <= DistanceToThirdCluster) {
				OriginalLocalImage[i] = clusters[1];
				sum[1]= OriginalLocalImage[i];
				count[1]++;
			}
			else {
				OriginalLocalImage[i] = clusters[2];
				sum[2] += OriginalLocalImage[i];
				count[2]++;
			}
		}

		for (int c = 0; c < 3; c++) {
			if (count[c] > 0) {
				previousClusters[c] = clusters[c];
				clusters[c] = sum[c] / count[c]; 
			}
		}
		if (clusters[0] == previousClusters[0] && clusters[1] == previousClusters[1] && clusters[2] == previousClusters[2]) {
			break;
		}
	}

	return OriginalLocalImage;
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
	img = "C:\\Users\\Aly Hany\\Downloads\\input.jpg";
	imagePath = marshal_as<System::String^>(img);
	imageData = InputImage(&ImageWidth, &ImageHeight, imagePath);
	
	
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

	//Parallized Code
	start_s = clock();
	MPI_Request scatterRequest;
	MPI_Iscatter(imageData, segmentSize, MPI_INT, localImage, segmentSize, MPI_INT, 0, MPI_COMM_WORLD,&scatterRequest);
	MPI_Wait(&scatterRequest, MPI_STATUS_IGNORE);

	FindMinMax(localImage, segmentSize, localMin, localMax);
    MPI_Allreduce(&localMin, &globalMin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
	MPI_Allreduce(&localMax, &globalMax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

	localImage = ImageGrayScaleSegmentation(localImage, segmentSize, globalMin,globalMax);
	MPI_Request gatherRequest;
	MPI_Igather(localImage, segmentSize, MPI_INT, globalImage, segmentSize, MPI_INT, 0, MPI_COMM_WORLD,&gatherRequest);
	MPI_Wait(&gatherRequest, MPI_STATUS_IGNORE);
	stop_s = clock(); cout << "local min " << localMin << endl;

	if (rank == 0) {
		CreateImage(globalImage, ImageWidth, ImageHeight);
		double ParallelTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "Time parallelized : " << ParallelTime << " ms" << endl;

		//Sequential Code
		start_s = clock();
		int sequentialMin = 0;
		int sequentialMax = 0;
		FindMinMax(imageData, ImageWidth * ImageHeight, sequentialMin, sequentialMax);
		ImageGrayScaleSegmentation(imageData, (ImageWidth * ImageHeight), sequentialMin, sequentialMax);
		stop_s = clock();
		double SequentialTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "Time Sequential : " << SequentialTime << " ms" << endl;
		cout << "Speed Up Using " << size << " Threads is (Sequential Time/ Parallel Time) " << SequentialTime / ParallelTime << endl;
	}
	
	free(imageData);

	MPI_Finalize();
	return 0;

}





