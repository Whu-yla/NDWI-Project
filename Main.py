#import the modules
import os
from ctypes import *
import xml.dom.minidom
import cv2
import numpy as np


# unzip tar.gz
ImageTarPath = "/home/wu/GF1"
ImageTarFileName = "GF1_WFV3_E116.6_N28.9_20170121_L1A0002136848.tar.gz"

#define the AOI
AOI = "/home/wu/Yilean/PoYangHuAOI/PoYangHuAOI.shp"

#define the threshold(0.05-0.1)
threshold = 0.05

#get the filename without ".tar.gz"
Number = ImageTarFileName.find('.tar.gz')
NewFileName = ImageTarFileName[: Number]

#create new folder use the filename named
os.system("mkdir -p /home/wu/datas/" + NewFileName)

#tar -zxvf to the newe folder
os.system("tar -zxvf" + ImageTarPath + "/" + ImageTarFileName + " -C " + "/home/wu/datas/" + NewFileName)

#get the XML
XML = "/home/wu/datas/" + NewFileName + "/" + NewFileName + ".xml"

#open the XML and get the receivetime with year,month,day
dom = xml.dom.minidom.parse(XML)
root = dom.documentElement
cc = root.getElementsByTagName('ReceiveTime')
c1=cc[0]
data = c1.firstChild.data
Number1 = data.find('-')
Number2 = data.find('-',Number1+1)
Number3  = data.find(' ')

year = float(data[:Number1])
month = float(data[Number1+1:Number2])
day = float(data[Number2+1:Number3])

#connect to the libraries
liba = cdll.LoadLibrary("/home/wu/CppTest/RadioCalibration.so")
libb = cdll.LoadLibrary("/home/wu/CppTest/AtmosphericCorrection.so")
libc = cdll.LoadLibrary("/home/wu/CppTest/NDWI.so")

# #Orthorectification
NewPath = " /home/wu/datas/" + NewFileName + "/"
os.system("gdalwarp -rpc " + NewPath + NewFileName + ".tiff" + NewPath + "ort.tiff")

#RadiometicCalibration
liba.RadiometicCalibration("0.1713F,0.16F,0.1497F,0.1435F","0.0f, 0.0f, 0.0f, 0.0f","/home/wu/datas/" + NewFileName + "/" + "ort.tiff","/home/wu/datas/" + NewFileName + "/" + "RadioCorrect.tiff")

#AtmosphericCorrection
libb.AtmosphericCorrection.argtypes = (c_float,c_float,c_float,c_float,c_char_p,c_char_p,c_char_p,c_char_p,c_char_p)
libb.AtmosphericCorrection(year,month,day,50.5447*3.141592654/180.0,"0.1713F,0.16F,0.1497F,0.1435F","0.0f, 0.0f, 0.0f, 0.0f","/home/wu/datas/" + NewFileName + "/" +"ort.tiff","/home/wu/datas/" + NewFileName + "/" + "RadioCorrect.tiff","/home/wu/datas/" + NewFileName + "/reflectivity.tiff")

#ImageCut
# os.system("gdalwarp -cutline " + AOI + " -dstnodata -9999.0 -crop_to_cutline " + "/home/wu/datas/" + NewFileName + "/reflectivity.tiff" + " /home/wu/datas/" + NewFileName +'/' + NewFileName +  "_subset.tiff")

# #NDWI calculate
# libc.NDWI.argtypes = (c_char_p,c_char_p)
# libc.NDWI("/home/wu/datas/" + NewFileName + "/" + NewFileName + "_subset.tiff","/home/wu/datas/" + NewFileName + "/" + NewFileName + "_NDWI.tiff")
#
# #Delete the files of processing
# os.system("rm -f"  + NewPath + "ort.tiff " +NewPath + "RadioCorrect.tiff " + NewPath + "reflectivity.tiff " + NewPath + NewFileName + "_subset.tiff")

#calculate the ndwi without subset
libc.NDWI.argtypes = (c_float,c_char_p,c_char_p)
libc.NDWI(threshold, "/home/wu/datas/" + NewFileName + "/" + "reflectivity.tiff","/home/wu/datas/" + NewFileName + "/" + NewFileName + "_NDWI.tiff")
# image = cv2.imread("/home/wu/datas/" + NewFileName + "/" + NewFileName + "_NDWI.tiff")
# result = cv2.threshold(image,0,1,cv2.THRESH_OTSU)

#Delete the files of processing
os.system("rm -f"  + NewPath + "ort.tiff " +NewPath + "RadioCorrect.tiff " + NewPath + "reflectivity.tiff")