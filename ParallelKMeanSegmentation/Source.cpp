#include <iostream>
#include <math.h>
#include <mpi.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//Read Image and save it to local arrayss************************	
	//Read Image and save it to local arrayss

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

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
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
	MyNewImage.Save("C:\\Users\\Aly Hany\\Downloads\\Output" + index + ".png");
	cout << "result Image Saved " << index << endl;
}
int minOfThree(int a, int b, int c) {
	if (a < c && a < b) return a;
	if (c < a && c < b) return c;
	return b;
}
void findMinMax(int arr[], int size, int& min, int& max) {
	min = arr[0]; // Initialize min to the first element
	max = arr[0]; // Initialize max to the first element

	for (int i = 1; i < size; i++) { // Start from the second element
		if (arr[i] < min) {
			min = arr[i];
		}
		if (arr[i] > max) {
			max = arr[i];
		}
	}
}
void ImageGrayScaleSegmentationParallized(int* OriginalImage, int Width, int Height) {
	cout << "Hello" << endl;
}
void ImageGrayScaleSegmentation(int* OriginalImage, int Width, int Height) {
	int* SegmentedImage = new int[Width * Height];
	
	for (int i = 0; i <(Width * Height); i++) {
		SegmentedImage[i] = OriginalImage[i] ;
		
	}
	int min = 0;
	int max = 0;
	findMinMax(OriginalImage, Width * Height,min,max);
	int clusters[3] = {min,(max+min)/2,max};
	int counter = 0;
	for (int i = 0; i < (Width * Height); i++) {
		int DistanceToFirstCluster = abs(SegmentedImage[i] - clusters[0]);
		int DistanceToSecondCluster =abs(SegmentedImage[i] - clusters[1]);
		int DistanceToThirdCluster =abs(SegmentedImage[i] - clusters[2]);
		if(SegmentedImage[i]<max/3)
		{
			SegmentedImage[i] = min;
		}
		else if(SegmentedImage[i]<2*max/3) {
		SegmentedImage[i] = max/2;
		}
	else{
	    SegmentedImage[i] = max;
		}
		counter = i;
	}
	createImage(SegmentedImage,Width,Height, 1);
}
int main()
{
	MPI_Init(NULL, NULL);
	int rank, size;
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s;
    double TotalTime = 0.0;

	System::String^ imagePath;
	std::string img;
	img = "C:\\Users\\Aly Hany\\Downloads\\images.jpeg";

	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	start_s = clock();
	ImageGrayScaleSegmentation(imageData, ImageWidth, ImageHeight);
	stop_s = clock();
	double SequentialTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "Time sequentially : " << SequentialTime << " ms" << endl;

	// Measure parallelized execution time
	start_s = clock();
	ImageGrayScaleSegmentationParallized(imageData, ImageWidth, ImageHeight);
	stop_s = clock();
	double ParallelTime = (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "Time parallelized : " << ParallelTime << " ms" << endl;
	free(imageData);

	MPI_Finalize();
	return 0;

}