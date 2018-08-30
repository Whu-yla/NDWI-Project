#include <iostream>
#include "gdal_priv.h"
#include "ogr_spatialref.h"

using namespace std;

//NDWI计算
//NDWI=(Green-NIR)/(Green+NIR)=(Band2-Band4)/(Band2+Band4)
extern "C"{
int NDWI(float threshold, const char *inFileName, const char *outFileName)
{
	//输入数据为裁剪以后的数据
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

	int imgSizeX = inputDataset->GetRasterXSize();
	int imgSizeY = inputDataset->GetRasterYSize();

	const char *imgFormat = "GTiff";
	GDALDriver *gdalDriver = GetGDALDriverManager()->GetDriverByName(imgFormat);
	if (gdalDriver == NULL)
	{ 
		cout<<"File create failed!"<<endl;
		return 1;
	}

	outputDataset = gdalDriver->Create(outputFileName,imgSizeX,imgSizeY,1,GDT_Float32,NULL);

	//获取输入数据的地理变化信息
	double goeInformation[6];
	inputDataset->GetGeoTransform(goeInformation);
	//读取输入数据的地理信息并写入输出文件
	const char * gdalProjection = inputDataset->GetProjectionRef();

	outputDataset->SetGeoTransform(goeInformation);
	outputDataset->SetProjection(gdalProjection);

	cout<<"Image Processing..."<<endl;
	//取得绿波段和近红外波段
	GDALRasterBand *raseterBandGreen = inputDataset->GetRasterBand(2);
	GDALRasterBand *raseterBandNIR = inputDataset->GetRasterBand(4);
	GDALRasterBand *outputRasterBand = outputDataset->GetRasterBand(1);
	//申请存储空间，为一行的大小
	float *bufferBlockGreen = (float *)CPLMalloc(sizeof(float) * imgSizeX);
	float *bufferBlockNIR = (float *)CPLMalloc(sizeof(float) * imgSizeX);
	float *outputBufferBlock = (float *)CPLMalloc(sizeof(float) * imgSizeX);

	//进行NDWI的计算
	for (int i = 0; i < imgSizeY; i++)
	{
		raseterBandGreen->RasterIO(GF_Read,0,i,imgSizeX,1,bufferBlockGreen,imgSizeX,1,GDT_Float32,0,0);
		raseterBandNIR->RasterIO(GF_Read,0,i,imgSizeX,1,bufferBlockNIR,imgSizeX,1,GDT_Float32,0,0);
		for (int j = 0; j < imgSizeX; j++)
		{
			outputBufferBlock[j] = (bufferBlockGreen[j] - bufferBlockNIR[j]) / (bufferBlockGreen[j] + bufferBlockNIR[j]);
			// 20141008 0.03
			// 20141119 0.1
			if (outputBufferBlock[j] <= threshold)
			{
				outputBufferBlock[j] = 0;
			} 
		}
		outputRasterBand->RasterIO(GF_Write,0,i,imgSizeX,1,outputBufferBlock,imgSizeX,1,GDT_Float32,0,0);
	}
	//释放资源
	CPLFree(bufferBlockGreen);
	CPLFree(bufferBlockNIR);
	CPLFree(outputBufferBlock);
	GDALClose(inputDataset);
	GDALClose(outputDataset);
	return 0;
}}
