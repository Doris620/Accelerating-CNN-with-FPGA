/*
 *
 * This file is part of the open-source SeetaFace engine, which includes three modules:
 * SeetaFace Detection, SeetaFace Alignment, and SeetaFace Identification.
 *
 * This file is part of the SeetaFace Identification module, containing codes implementing the
 * face identification method described in the following paper:
 *
 *
 *   VIPLFaceNet: An Open Source Deep Face Recognition SDK,
 *   Xin Liu, Meina Kan, Wanglong Wu, Shiguang Shan, Xilin Chen.
 *   In Frontiers of Computer Science.
 *
 *
 * Copyright (C) 2016, Visual Information Processing and Learning (VIPL) group,
 * Institute of Computing Technology, Chinese Academy of Sciences, Beijing, China.
 *
 * The codes are mainly developped by Wanglong Wu(a Ph.D supervised by Prof. Shiguang Shan)
 *
 * As an open-source face recognition engine: you can redistribute SeetaFace source codes
 * and/or modify it under the terms of the BSD 2-Clause License.
 *
 * You should have received a copy of the BSD 2-Clause License along with the software.
 * If not, see < https://opensource.org/licenses/BSD-2-Clause>.
 *
 * Contact Info: you can send an email to SeetaFace@vipl.ict.ac.cn for any problems.
 *
 * Note: the above information must be kept whenever or wherever the codes are used.
 *
 * -----------------------------------------------------------------------------------------------------
 * 
 * The FPGA acceleration parts of this file are developed by Xuanzhi LIU (Walker LAU).
 * 
 * This version accelerates all 7 CONV-layers of VIPLFaceNet.
 * 
 * If you want to get the latest version of this project or met any problems,
 * please go to <https://github.com/WalkerLau/Accelerating-CNN-with-FPGA> , 
 * I will try to help as much as I can.
 * 
 * You can redistribute this source codes and/or modify it under the terms of the BSD 2-Clause License.
 *
 * Note: the above information must be kept whenever or wherever the codes are used.
 * 
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "time.h"
#include <fstream>
#include <string>
#include <vector>
#include "ctime"
using namespace std;

/* chg 删
#ifdef _WIN32
#pragma once
#include <opencv2/core/version.hpp>

#define CV_VERSION_ID CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) \
  CVAUX_STR(CV_SUBMINOR_VERSION)

#ifdef _DEBUG
#define cvLIB(name) "opencv_" name CV_VERSION_ID "d"
#else
#define cvLIB(name) "opencv_" name CV_VERSION_ID
#endif //_DEBUG

#pragma comment( lib, cvLIB("core") )
#pragma comment( lib, cvLIB("imgproc") )
#pragma comment( lib, cvLIB("highgui") )

#endif //_WIN32

#if defined(__unix__) || defined(__APPLE__)

#ifndef fopen_s

#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL

#endif //fopen_s

#endif //__unix
*/

#if __SDSCC__
#undef __ARM_NEON__
#undef __ARM_NEON
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/core/core.hpp>
#define __ARM_NEON__
#define __ARM_NEON
#else
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/core/core.hpp>
#endif

#include "face_identification.h"
#include "common.h"

using namespace seeta;

#define TEST(major, minor) major##_##minor##_Tester()
#define EXPECT_NE(a, b) if ((a) == (b)) std::cout << "ERROR: "
#define EXPECT_EQ(a, b) if ((a) != (b)) std::cout << "ERROR: "

/* chg 测：改用绝对路径，用于下板使用。
#ifdef _WIN32
std::string DATA_DIR = "../../data/";
std::string MODEL_DIR = "../../model/";
#else
std::string DATA_DIR = "./data/";
std::string MODEL_DIR = "./model/";
#endif
*/
std::string DATA_DIR = "/media/card/data/";
std::string MODEL_DIR = "/media/card/model/";


void TEST(FaceRecognizerTest, CropFace) {
  FaceIdentification face_recognizer((MODEL_DIR + "seeta_fr_v1.0.bin").c_str());
  std::string test_dir = DATA_DIR + "test_face_recognizer/";
  /* data initialize */
  std::ifstream ifs;
  std::string img_name;
  FacialLandmark pt5[5];
  ifs.open(test_dir + "test_file_list.txt", std::ifstream::in);
  clock_t start, count = 0;
  int img_num = 0;

  while (ifs >> img_name) {
    img_num ++ ;
    // read image
    cv::Mat src_img = cv::imread(test_dir + img_name, 1);  // 读入3通道彩色图
    EXPECT_NE(src_img.data, nullptr) << "Load image error!";

    // ImageData store data of an image without memory alignment.
	  // ImageData 是自定义结构体，可存图像像素（uint8_t型）的指针（data），图像的长、宽、通道信息。
    ImageData src_img_data(src_img.cols, src_img.rows, src_img.channels());
    src_img_data.data = src_img.data;

    // 5 located landmark points (left eye, right eye, nose, left and right corner of mouse).
    // FacialLandmark 是一个结构体，存点的二维坐标。
    for (int i = 0; i < 5; ++ i) {
      ifs >> pt5[i].x >> pt5[i].y;
    }

    // Create a image to store crop face.
	  // 根据所载入model的配置参数来创建一个dst_img空矩阵。
    cv::Mat dst_img(face_recognizer.crop_height(),
					face_recognizer.crop_width(),
					CV_8UC(face_recognizer.crop_channels()));  // CV_[The number of bits per item][Signed or Unsigned][Type Prefix]C[The channel number]
    ImageData dst_img_data(dst_img.cols, dst_img.rows, dst_img.channels());
    dst_img_data.data = dst_img.data;
    /* Crop Face */
    start = clock();
    face_recognizer.CropFace(src_img_data, pt5, dst_img_data);	
    count += clock() - start;
    // Show crop face
    //    cv::imshow("Crop Face", dst_img);
    //    cv::waitKey(0);
    //    cv::destroyWindow("Crop Face");
  }
  ifs.close();
  std::cout << "Test successful!----CropFace \nAverage crop face time: "
    << 1000.0 * count / CLOCKS_PER_SEC / img_num << "ms" << std::endl;
}


void TEST(FaceRecognizerTest, ExtractFeature) {
  FaceIdentification face_recognizer((MODEL_DIR + "seeta_fr_v1.0.bin").c_str());
  std::string test_dir = DATA_DIR + "test_face_recognizer/";

  int feat_size = face_recognizer.feature_size();
  EXPECT_EQ(feat_size, 2048);

  FILE* feat_file = NULL;

  // Load features extract from caffe
  feat_file = fopen((test_dir + "feats.dat").c_str(), "rb"); 
  int n, c, h, w;
  EXPECT_EQ(fread(&n, sizeof(int), 1, feat_file), (unsigned int)1);
  EXPECT_EQ(fread(&c, sizeof(int), 1, feat_file), (unsigned int)1);
  EXPECT_EQ(fread(&h, sizeof(int), 1, feat_file), (unsigned int)1);
  EXPECT_EQ(fread(&w, sizeof(int), 1, feat_file), (unsigned int)1);
  float* feat_caffe = new float[n * c * h * w];
  float* feat_sdk = new float[n * c * h * w];
  EXPECT_EQ(fread(feat_caffe, sizeof(float), n * c * h * w, feat_file),
    n * c * h * w);
  EXPECT_EQ(feat_size, c * h * w);

  int cnt = 0;

  /* Data initialize */
  std::ifstream ifs(test_dir + "crop_file_list.txt");
  std::string img_name;

  clock_t start, count = 0;
  int img_num = 0, lb;
  double average_sim = 0.0;
  while (ifs >> img_name >> lb) {
    // read one image
    cv::Mat src_img = cv::imread(test_dir + img_name, 1);
    EXPECT_NE(src_img.data, nullptr) << "Load image error!";
	// resize注释:  resize(InputArray src, OutputArray dst, Size dsize)
    cv::resize(src_img, src_img, cv::Size(face_recognizer.crop_height(),
			   face_recognizer.crop_width()));

    // ImageData store data of an image without memory alignment.
    ImageData src_img_data(src_img.cols, src_img.rows, src_img.channels());
    src_img_data.data = src_img.data;

    /* Extract feature */
	  // 用本项目的方法重新提取feature值，并放到首地址为feat_sdk的动态内存中。
    start = clock();
    face_recognizer.ExtractFeature(src_img_data,	// 传进这个结构体的是src_img_data（ImageData）的引用。
      feat_sdk + img_num * feat_size);
    count += clock() - start;

    /* Caculate similarity*/
    // 比较两种方法提取到的feature值的余弦相似度。
    float* feat1 = feat_caffe + img_num * feat_size;
    float* feat2 = feat_sdk + img_num * feat_size;
    float sim = face_recognizer.CalcSimilarity(feat1, feat2);

    // chg 测：看一下这张图的准确率
    std::cout<<"Similarity of this pic is: "<<sim<<std::endl;
    // chg 测：显示读图张数
    std::cout<<"Finish extracting image -------------------------------------------------------------------------------------"<< img_num + 1 << std::endl;

    average_sim += sim;
    img_num ++ ;
  }
  ifs.close();

  average_sim /= img_num;
  if (1.0 - average_sim >  0.01) {
    std::cout<< "average similarity: " << average_sim << std::endl;
  }
  else {
    std::cout << "Test successful!----ExtractFeature\nAverage extract feature time: "
      << 1000.0 * count / CLOCKS_PER_SEC / img_num << "ms" << std::endl;
  }
  delete []feat_caffe;
  delete []feat_sdk;
}

void TEST(FaceRecognizerTest, ExtractFeatureWithCrop) {
  FaceIdentification face_recognizer((MODEL_DIR + "seeta_fr_v1.0.bin").c_str());
  std::string test_dir = DATA_DIR + "test_face_recognizer/";

  int feat_size = face_recognizer.feature_size();
  EXPECT_EQ(feat_size, 2048);

  FILE* feat_file = NULL;

  // Load features extract from caffe
  //fopen_s(&feat_file, (test_dir + "feats.dat").c_str(), "rb");
  feat_file = fopen( (test_dir + "feats.dat").c_str(), "rb");	// chg 改
  int n, c, h, w;
  EXPECT_EQ(fread(&n, sizeof(int), 1, feat_file), (unsigned int)1);
  EXPECT_EQ(fread(&c, sizeof(int), 1, feat_file), (unsigned int)1);
  EXPECT_EQ(fread(&h, sizeof(int), 1, feat_file), (unsigned int)1);
  EXPECT_EQ(fread(&w, sizeof(int), 1, feat_file), (unsigned int)1);
  float* feat_caffe = new float[n * c * h * w];
  float* feat_sdk = new float[n * c * h * w];
  EXPECT_EQ(fread(feat_caffe, sizeof(float), n * c * h * w, feat_file),
    n * c * h * w);
  EXPECT_EQ(feat_size, c * h * w);

  int cnt = 0;

  /* Data initialize */
  std::ifstream ifs(test_dir + "test_file_list.txt");
  std::string img_name;
  FacialLandmark pt5[5];

  clock_t start, count = 0;
  int img_num = 0;
  double average_sim = 0.0;
  while (ifs >> img_name) {
    // read image
    cv::Mat src_img = cv::imread(test_dir + img_name, 1);
    EXPECT_NE(src_img.data, nullptr) << "Load image error!";

    // ImageData store data of an image without memory alignment.
    ImageData src_img_data(src_img.cols, src_img.rows, src_img.channels());
    src_img_data.data = src_img.data;

    // 5 located landmark points (left eye, right eye, nose, left and right
    // corner of mouse).
    for (int i = 0; i < 5; ++ i) {
      ifs >> pt5[i].x >> pt5[i].y;
    }

    /* Extract feature: ExtractFeatureWithCrop */
    start = clock();
    face_recognizer.ExtractFeatureWithCrop(src_img_data, pt5,
      feat_sdk + img_num * feat_size);
    count += clock() - start;

    /* Caculate similarity*/
    float* feat1 = feat_caffe + img_num * feat_size;
    float* feat2 = feat_sdk + img_num * feat_size;
    float sim = face_recognizer.CalcSimilarity(feat1, feat2);
    average_sim += sim;
    img_num ++ ;
  }
  ifs.close();
  average_sim /= img_num;
  if (1.0 - average_sim >  0.02) {
    std::cout<< "average similarity: " << average_sim << std::endl;
  }
  else {
    std::cout << "Test successful!----ExtractFeatureWithCrop\nAverage extract feature time: "
      << 1000.0 * count / CLOCKS_PER_SEC / img_num << "ms" << std::endl;
  }
  delete []feat_caffe;
  delete []feat_sdk;
}

int main(int argc, char* argv[]) {

  // chg 删：TEST(FaceRecognizerTest, CropFace);
  TEST(FaceRecognizerTest, ExtractFeature);
  // chg 删：TEST(FaceRecognizerTest, ExtractFeatureWithCrop);

  cin.get();

  return 0;
}
