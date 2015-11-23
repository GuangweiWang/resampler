//
//  main.cpp
//  downsampler
//
//  Created by Guangwei Wang on 11/14/15.
//  Copyright © 2015 Guangwei Wang. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <typeinfo>
#include <fstream>
#include "downsampler.h"

using namespace std;

//print usage for wrong parameters
void print_usage_and_exit(bool condition,
                         const char* message = "usage", bool manu = true) {
  if(condition) {
    if(manu) {
      cout << "Help: " << message << endl << endl;
      cout << "this downsampler only support ratio=1:2 " << endl;
      
      cout << "usage:" << endl;
      cout << "./downsampler win hin inyuv wout hout outyuv" ;
      cout << "[-method <0/1/2> [-mode <0/1>]]" << endl << endl;
      
      cout << "	win:	input width(luma samples)" << endl;
      cout << "	hin:	input height(luam samples)" << endl;
      cout << "	inyuv:	input yuvfile" << endl;
      cout << "	wout:	output width(luma samples)" << endl;
      cout << "	hout:	output height(luma samples)" << endl;
      cout << "	outyuv:	outout yuvfile" << endl;
      cout << "	-method: choose which downsampler to use(default 0)" << endl;
      cout << "		0: use bilinear downsampler(default)" << endl;
      cout << "		1: use JSVM downsampler" << endl;
      cout << "		2: use bicubic downsampler" << endl;
      cout << "     3: use hybrid downsampler" << endl;
      cout << "     4: use tap downsampler" << endl;
      cout << "	-mode: choose simple or complex algorithm(default 0)" << endl;
      cout << "     not support yet" << endl;
      cout << "		0: simple algorithm, speed faster" << endl;
      cout << "		1: complex algorithm, speed slower than 0" << endl << endl;
      
      cout << "example:" << endl;
      cout << "./downsampler.bin 1280 720 infile.yuv 320 180 outfile.yuv" << endl;
      
    }else{
      cout << "parameters " << message << " error!" << endl;
    }
    exit(1);
  }
}

void parseCmdLine(int argc, char* argv[], SDownsamplerCtx* pDownsamplerCtx){
  int i = 7;
  if (argc == 2) {
    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-help")) {
      print_usage_and_exit(true, "usage and help");
    }
    
  }
  
  if (argc < 7 || argc > 11){
    print_usage_and_exit(true, "wrong number of argument");
  }
  
  //parse basic parameters
  pDownsamplerCtx->iSrcWidth = atoi(argv[1]);
  pDownsamplerCtx->iSrcHeight = atoi(argv[2]);
  pDownsamplerCtx->pInfileName = (uint8_t*)argv[3];
  pDownsamplerCtx->iDstWidth = atoi(argv[4]);
  pDownsamplerCtx->iDstHeight = atoi(argv[5]);
  pDownsamplerCtx->pOutfileName = (uint8_t*)argv[6];
  
  //parse additional parameters
  while(i < argc) {
    if( !strcmp(argv[i], "-method"))
      pDownsamplerCtx->iDownsampleMethod = atoi(argv[++i]);
    else if(!strcmp(argv[i], "-mode"))
      pDownsamplerCtx->iDownsampleMode = atoi(argv[++i]);
    else {
      cout << "unsupport arguments: " << argv[i] << endl;
      exit (1);
    }
    i++;
  }
  
  //check parameters valid or not
  if(pDownsamplerCtx->iSrcWidth < pDownsamplerCtx->iDstWidth
     || pDownsamplerCtx->iSrcHeight < pDownsamplerCtx->iDstHeight) {
    print_usage_and_exit(true,
                        "input width/height should be larger than output!");
  }
  
  if(pDownsamplerCtx->iDownsampleMethod != 0
     && pDownsamplerCtx->iDownsampleMethod != 1
     && pDownsamplerCtx->iDownsampleMethod != 2
     && pDownsamplerCtx->iDownsampleMethod != 3
     && pDownsamplerCtx->iDownsampleMethod != 4){
    print_usage_and_exit(true, "input a wrong method!");
  }
  
  if(pDownsamplerCtx->iDownsampleMode != 0
     && pDownsamplerCtx->iDownsampleMode != 1)
    print_usage_and_exit(true, "input a wrong mode!");
  
  //for temp
  if (pDownsamplerCtx->iSrcWidth / 2 != pDownsamplerCtx->iDstWidth ||
      pDownsamplerCtx->iSrcHeight / 2 != pDownsamplerCtx->iDstHeight) {
    cout << "sorry: only support ratio = 1:2" << endl;
    exit(1);
  }
}

void ChooseDownsampler(SDownsamplerCtx* pDownsamplerCtx,
                       Downsampler** ppDownsampler) {
  //choose downsampler
  switch (pDownsamplerCtx->iDownsampleMethod) {
    case 0:
      cout << "choose bilinear downsampler " << endl;
      *ppDownsampler = new BilinearDownsampler();
      break;
      
    case 1:
      cout << "choose jsvm downsampler(tap8) " << endl;
      *ppDownsampler = new BenchmarkDownsampler();
      break;
      
    case 2:
      cout << "choose bicubic downsampler " << endl;
      *ppDownsampler = new BicubicDownsampler();
      break;
      
    case 3:
      cout << "choose hybrid downsampler " << endl;
      *ppDownsampler = new TestDownsampler();
      break;
      
    case 4:
      cout << "choose tap filter downsampler" << endl;
      *ppDownsampler = new TapFilterDownsampler();
      break;
      
    default:
      cout << "use default jsvm downsampler" << endl;
      *ppDownsampler = new BenchmarkDownsampler();
      break;
  }
}

int main(int argc, char* argv[]) {
  
  FILE*	fin;
  FILE* fout;
  SDownsamplerCtx	sDownsamplerCtx;
  SDownsamplerCtx*	pDownsamplerCtx = &sDownsamplerCtx;
  Downsampler*	pDownsampler = NULL;
  int iMaxFrameNum = 0;
  int iFileSize = 0;
  int iFrameSize = 0;
  
  memset(&sDownsamplerCtx, 0, sizeof(sDownsamplerCtx));
  
  parseCmdLine(argc, argv, pDownsamplerCtx);
  ChooseDownsampler(pDownsamplerCtx, &pDownsampler);
  
  fin = fopen((const char*)pDownsamplerCtx->pInfileName, "rb");
  if(!fin) {
    cout << "Error: can not open input file!" << endl;
    exit(1);
  }
  fseek(fin, 0L, SEEK_END);
  iFileSize = (int)ftell(fin);
  fseek(fin, 0L, SEEK_SET);
  
  fout = fopen((const char*)pDownsamplerCtx->pOutfileName, "wb");
  if(!fout) {
    cout << "Error: can not open output file!" << endl;
    exit(1);
  }
  
  //caculate max frame number
  iFrameSize = pDownsamplerCtx->iSrcWidth * pDownsamplerCtx->iSrcHeight * 1.5;
  iMaxFrameNum = iFileSize / iFrameSize;
  
  pDownsampler->InitDownsampler(pDownsamplerCtx);
  
  for(int i = 0; i < iMaxFrameNum; i++) {
    pDownsampler->ReadFrameFromFile(fin);
    pDownsampler->Processing();
    pDownsampler->WriteFrameToFile(fout);
  }
  
  //downsample end
  delete pDownsampler;
  fclose(fin);
  fclose(fout);
  
  return 0;
}


