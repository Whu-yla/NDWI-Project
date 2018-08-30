#include <iostream>
#include <fstream>
#include <cstdlib>
#include "gdal_priv.h"
#include "ogr_spatialref.h"
//#include "ImageProcess.h"

using namespace std;
float PI = 3.141592654F;
bool ParamRead(const char* inFileName, float gain[], float bias[])
{
	char buffer[1024];
	ifstream in;
	in.open(inFileName, ios::in);
	if (!in.eof())
	{
		//±íÊŸžÃÐÐ×Ö·ûŽïµœ1024žö»òÓöµœ»»ÐÐŸÍœáÊø
		in.getline(buffer, 1024, '\n');

		char* pStart = buffer;
		char* pEnd;
		for (int i = 0; i < 4; i++)
		{
			gain[i] = strtof(pStart, &pEnd);
			pStart = pEnd;
		}

		in.getline(buffer, 1024, '\n');
		pStart = buffer;
		for (int i = 0; i < 4; i++)
		{
			bias[i] = strtof(pStart, &pEnd);
			pStart = pEnd;
		}
	}
	else
	{
		in.close();
		return false;
	}
	in.close();
	return true;
}

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
int StatisticsSmall(const char *fileName, float smallDN[])
{
	//ÊäÈëÊýŸÝÎªÔ­ÊŒÓ°Ïñ
	GDALDataset *sourceDataset;
	GDALAllRegister();
	cout << "Reading image..." << endl;
	sourceDataset = (GDALDataset *)GDALOpen(fileName, GA_ReadOnly);
	if (sourceDataset == NULL)
	{
		cout << "File open failed!" << endl;
		return 1;
	}

	int imgSizeX = sourceDataset->GetRasterXSize();
	int imgSizeY = sourceDataset->GetRasterYSize();

	cout << "Image Processing..." << endl;
	float *bufferBlock = (float *)CPLMalloc(sizeof(float) * imgSizeX);
	for (int i = 0; i < 4; i++)
	{
		GDALRasterBand *rasterBand = sourceDataset->GetRasterBand(i + 1);
		float smallestDN = 1023;
		for (int j = 0; j < imgSizeY; j++)
		{
			rasterBand->RasterIO(GF_Read, 0, j, imgSizeX, 1, bufferBlock, imgSizeX, 1, GDT_Float32, 0, 0);
			for (int k = 0; k < imgSizeX; k++)
			{
				if (bufferBlock[k] < smallestDN)
				{
					smallestDN = bufferBlock[k];
				}
			}
		}
		smallDN[i] = smallestDN;
	}

	//ÊÍ·Å×ÊÔŽ
	CPLFree(bufferBlock);
	GDALClose(sourceDataset);
	return 0;
}

float JulianDay(float year, float month, float day)
{
	float julianDay = day - 32075 + (1461 * (year + 4800 + (month - 14) / 12) / 4)
		+ (367 * ((month - 2 - (month - 14) / 12) / 12))
		- ((3 * (year + 4900 + (month - 14) / 12) / 100) / 4);
	return julianDay;
}
float EarthSunDistance(float julianDay)
{
	float distance = 1 - 0.01674F * cos(0.9856F * (julianDay - 4) * PI / 180);
	return distance;
}

//Lhazel=LMINI+QCAL*(LMAXL-LMINI)/QCALMAX-0.01*ESUNI*COS£²(SZ)/(ŠÐ*D£²)
//lhazel[]Êý×éÖÐŽæŽ¢×Å×îÖÕµÄŒÆËãœá¹û
int Lhazel(float qcal[], float qcalmax, float gain[], float bias[], float sz, float esuni[], float distance, float lhazel[])
{
	float lmini[4];
	for (int i = 0; i < 4; i++)
	{
		lmini[i] = bias[i];
	}
	float lmaxi[4];
	for (int i = 0; i < 4; i++)
	{
		lmaxi[i] = bias[i] + (gain[i] * 1023);
	}

	for (int i = 0; i < 4; i++)
	{
		lhazel[i] = (lmini[i] + qcal[i] * (lmaxi[i] - lmini[i]) / qcalmax)
			- (0.01F * esuni[i] * pow(cos(sz), 2) / (PI * pow(distance, 2)));
	}//不同的头文件包含了对同一个函数的声明，但是有的有external "C"声明，有的没有
	return 0;
}

//ŠÑ=ŠÐ*D2*(LsatI-LhazeI)/ESUNI*COS2(SZ)
//distanceÈÕµØÌìÎÄµ¥Î»ŸàÀë
//lhazelŽóÆø²ã¹âÆ×·øÉäÖµ
//esuniŽóÆø¶¥²ãÌ«Ñô·øÕÕ¶È£šESUNI£©ŽÓÒ£žÐÈšÍþµ¥Î»¶šÆÚ²â¶š²¢¹«²ŒµÄÐÅÏ¢ÖÐ»ñÈ¡
//szÌ«ÑôÌì¶¥œÇ
extern "C"{
int AtmosphericCorrection(float year, float month, float day, float sz,
	const char* strGain, const char* strBias,
	const char* orthorectificationFileName, const char *radiationFileName, const char *reflectivityFileName)
{
	// ÏÂÃæÒ»¶ÎŽúÂëŽŠÀíŽ«ÈëµÄ²ÎÊý£¬ŽÓ×Ö·ûŽ®ÖÐ¶ÁÈ¡ÊýŸÝ×ªÎªÊý×Ö
	float gain[4];
	float bias[4];
	vector<string> vetGain;
	vector<string> vetBias;
	const char* delim = ",";
	StrSplit(strGain, delim, vetGain);
	if (4 == vetGain.size())
		Str2Float(vetGain, gain);
	else
		//不同的头文件包含了对同一个函数的声明，但是有的有external "C"声明，有的没有
		return 1;
	StrSplit(strBias, delim, vetBias);
	if (4 == vetBias.size())
		Str2Float(vetBias, bias);
	else
		return 1;

	// °ÑÔ­ÀŽµ¥¶À·Ö¿ªŒÆËãµÄ²œÖèÈ«²¿ºÏ²¢µœAtmosphericCorrectionº¯ÊýÖÐ
	float distance = EarthSunDistance(JulianDay(year, month, day));
	float qcal[4];
	sz = sz * PI / 180;
	float esuni[4] = { 1968.12F, 1841.69F, 1540.30F, 1069.53F };
	float lhazel[4];
	StatisticsSmall(orthorectificationFileName, qcal);
	Lhazel(qcal, 1023, gain, bias, sz, esuni, distance, lhazel);


	//ÊäÈëÊýŸÝÎª·øÉä¶š±êºóµÄÓ°Ïñ
	GDALDataset *inputDataset;
	GDALDataset *outputDataset;
	GDALAllRegister();
	cout<<"Reading image..."<<endl;
	inputDataset = (GDALDataset *)GDALOpen(radiationFileName,GA_ReadOnly);
	if (inputDataset == NULL)
	{
		cout<<"File open failed!"<<endl;
		return 1;
	}

	int imgSizeX = inputDataset->GetRasterXSize();
	int imgSizeY = inputDataset->GetRasterYSize();

	GDALDriver *gdalDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (gdalDriver == NULL)
	{ 
		cout<<"File create failed!"<<endl;
		return 1;
	}
	outputDataset = gdalDriver->Create(reflectivityFileName,imgSizeX,imgSizeY,4,GDT_Float32,NULL);

	//»ñÈ¡ÊäÈëÊýŸÝµÄµØÀí±ä»¯ÐÅÏ¢
	double goeInformation[6];
	inputDataset->GetGeoTransform(goeInformation);
	//¶ÁÈ¡ÊäÈëÊýŸÝµÄµØÀíÐÅÏ¢²¢ÐŽÈëÊä³öÎÄŒþ
	const char * gdalProjection = inputDataset->GetProjectionRef();

	outputDataset->SetGeoTransform(goeInformation);
	outputDataset->SetProjection(gdalProjection);

	cout<<"Image Processing..."<<endl;
	float *bufferBlock = (float *)CPLMalloc(sizeof(float) * imgSizeX);
	for (int i = 0; i < 4; i++)
	{
		GDALRasterBand *inputRasterBand = inputDataset->GetRasterBand(i + 1);
		GDALRasterBand *outputRasterBand = outputDataset->GetRasterBand(i + 1);
		for (int j = 0; j < imgSizeY; j++)
		{
			inputRasterBand->RasterIO(GF_Read,0,j,imgSizeX,1,bufferBlock,imgSizeX,1,GDT_Float32,0,0);
			for (int k = 0; k < imgSizeX; k++)
			{
				bufferBlock[k] = PI * pow(distance,2) * (bufferBlock[k] - lhazel[i]) / esuni[i] * pow(cos(sz),2);
			}
			outputRasterBand->RasterIO(GF_Write,0,j,imgSizeX,1,bufferBlock,imgSizeX,1,GDT_Float32,0,0);
		}
		cout<<"Band"<<i + 1<<" fininshed!"<<endl;
	}

	//ÊÍ·Å×ÊÔŽ
	CPLFree(bufferBlock);
	GDALClose(inputDataset);
	GDALClose(outputDataset);
	return 0;
}
}
