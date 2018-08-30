#include <iostream>
#include <fstream>
#include <cstdlib>
#include "gdal_priv.h"
#include "ogr_spatialref.h"

using namespace std;

void StrSplit(const string& str, const string& delim, vector<string>& result)
{
	size_t front = 0;
	size_t back = str.find_first_of(delim, front);
	while (back != string::npos)
	{
		result.push_back(str.substr(front, back - front));
		front = back + 1;
		back = str.find_first_of(delim, front);
	}
	if (back - front > 0)
	{
		result.push_back(str.substr(front, back - front));
	}
}

void Str2Float(vector<string>& vetNum, float numbers[])
{
	for (size_t i = 0; i < vetNum.size(); i++)
	{
		numbers[i] = atof(vetNum[i].c_str());
	}
}



extern "C"{
int RadiometicCalibration(const char* strGain, const char* strBias, const char *inFileName, const char *outFileName)
{
	const char *inputFileName = inFileName;
	const char *outputFileName = outFileName;

	GDALDataset *inputDataset;
	GDALDataset *outputDataset;

	GDALAllRegister();

	cout<<"Reading image..."<<endl;
	inputDataset = (GDALDataset *)GDALOpen(inputFileName,GA_ReadOnly);
	if (inputDataset == NULL)
	{
		cout<<"File open failed!"<<endl;
		return 1;
	}

	int bandCount = inputDataset->GetRasterCount();
	int imgSizeX = inputDataset->GetRasterXSize();
	int imgSizeY = inputDataset->GetRasterYSize();

	const char *imgFormat = "GTiff";
	GDALDriver *gdalDriver = GetGDALDriverManager()->GetDriverByName(imgFormat);
	if (gdalDriver == NULL)
	{ 
		cout<<"File create failed!"<<endl;
		return 1;
	}

	outputDataset = gdalDriver->Create(outputFileName,imgSizeX,imgSizeY,4,GDT_Float32,NULL);
	double goeInformation[6];
	inputDataset->GetGeoTransform(goeInformation);

	const char * gdalProjection = inputDataset->GetProjectionRef();

	outputDataset->SetGeoTransform(goeInformation);
	outputDataset->SetProjection(gdalProjection);

	float gain[4];
	float bias[4];
	vector<string> vetGain;
	vector<string> vetBias;
	const char* delim = ",";
	StrSplit(strGain, delim, vetGain);
	if (4 == vetGain.size())
		Str2Float(vetGain, gain);
	else
		return 1;
	StrSplit(strBias, delim, vetBias);
	if (4 == vetBias.size())
		Str2Float(vetBias, bias);
	else
		return 1;

	cout << "Image Processing..." << endl;
	for (int i = 1; i <= 4; i ++)
	{
		float *bufferBlock = (float *)CPLMalloc(sizeof(float) * imgSizeX);
		GDALRasterBand *inputRasterBand = inputDataset->GetRasterBand(i);
		GDALRasterBand *outputRasterBand = outputDataset->GetRasterBand(i);
		cout<<"Reading "<<"Band"<<i<<"..."<<endl;
		for (int j = 0; j < imgSizeY; j++)
		{
			inputRasterBand->RasterIO(GF_Read,0,j,imgSizeX,1,bufferBlock,imgSizeX,1,GDT_Float32,0,0);
			for (int k = 0; k < imgSizeX; k++)
			{
				bufferBlock[k] =  bufferBlock[k] * gain[i - 1] + bias[i -1];
			}
			outputRasterBand->RasterIO(GF_Write,0,j,imgSizeX,1,bufferBlock,imgSizeX,1,GDT_Float32,0,0);
		}
		CPLFree(bufferBlock);
		cout<<"Band"<<i<<" finished!"<<endl;
	}
	GDALClose(inputDataset);
	GDALClose(outputDataset);
	return 0;
}
}
int main()
{
	RadiometicCalibration("0.1713F,0.16F,0.1497F,0.1435F","0.0f, 0.0f, 0.0f, 0.0f","E:/WaterExtraction/01_Orthorectification.tif", 
		"E:/WaterExtraction/02_RadiometicCalibration.tiff");
	return 0;
}

