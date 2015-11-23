//
//  bicubic_downsampler.cpp
//  downsampler
//
//  Created by Guangwei Wang on 11/18/15.
//  Copyright © 2015 Guangwei Wang. All rights reserved.
//

#include <stdio.h>
#include <iostream>
#include "downsampler.h"
#include "measure_time.h"

using namespace std;

BicubicDownsampler::BicubicDownsampler() {
  m_bInitBicubicDownsamplerDone = false;
  m_bCreatBicubicDownsamplerDone = false;
  m_pTempBuffer = NULL;

}

BicubicDownsampler::~BicubicDownsampler() {
  DestroyBicubicDownsampler();
}

void BicubicDownsampler::CreatBicubicDownsampler() {
  if (!m_bCreatBicubicDownsamplerDone) {
    m_bCreatBicubicDownsamplerDone = true;
    
    m_pTempBuffer = new int [m_sSrcFrame.iStrideY * m_sSrcFrame.iHeightY];
    if (!m_pTempBuffer) {
      std::cout << "malloc TestDownsampler memory error!" << std::endl;
      exit(1);
    }
  }else {
    std::cout << "can not creat an exist TestDownsampler!" << std::endl;
    exit(1);
  }
}

void BicubicDownsampler::DestroyBicubicDownsampler() {
  if (m_bCreatBicubicDownsamplerDone) {
    m_bCreatBicubicDownsamplerDone = false;

    if (m_pTempBuffer) {
      delete m_pTempBuffer;
      m_pTempBuffer = NULL;
    }
  }else {
    std::cout << "can not destroy TestDownsampler agin!" << std::endl;
    exit(1);
  }
}

void BicubicDownsampler::InitDownsampler(SDownsamplerCtx* pDownsamplerCtx) {
  GetOption(pDownsamplerCtx);
  CreatDownsampler(pDownsamplerCtx);
  CreatBicubicDownsampler();  
}

void BicubicDownsampler::Processing() {
  long long iStartTime = 0;
  iStartTime = WelsTime();
  BicubicInterpolation(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                       m_sDstFrame.iWidthY, m_sDstFrame.iHeightY,
                       m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                       m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
  BicubicInterpolation(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                       m_sDstFrame.iWidthU, m_sDstFrame.iHeightU,
                       m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                       m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
  BicubicInterpolation(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                       m_sDstFrame.iWidthV, m_sDstFrame.iHeightV,
                       m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                       m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
  m_iTotalTime += WelsTime() - iStartTime;
}

void BicubicDownsampler::BicubicInterpolation(unsigned char* pDst, int iDstStride,
                                           int iDstWidth, int iDstHeight,
                                           unsigned char* pSrc, int iSrcStride,
                                           int iSrcWidth, int iSrcHeight) {
  float AC[4] = {-0.125, 0.625, 0.625, -0.125 }; //downsacale == 2 || == 4
  int B[4][4] = {0};
  int iScale = 2;
  int iScaleSuf = (iScale == 2 ? 0:1);
  float fTempArray[4] = {0.0,};
  float fTempResult = 0.0;
  
  int i,j;
  int iIndexX[4], iIndexY[4];
  int iTemp[16];
  for (j = 0; j < iDstHeight; j++) {
    for (i = 0; i < iDstWidth; i++) {
      iIndexX[0] = WELS_CLAMP(iScale*i + iScaleSuf - 1, 0, iSrcWidth - 1);
      iIndexX[1] = WELS_CLAMP(iScale*i    , 0, iSrcWidth - 1);
      iIndexX[2] = WELS_CLAMP(iScale*i + iScaleSuf + 1, 0, iSrcWidth - 1);
      iIndexX[3] = WELS_CLAMP(iScale*i + iScaleSuf + 2, 0, iSrcWidth - 1);
      
      iIndexY[0] = WELS_CLAMP(iScale*j + iScaleSuf - 1, 0, iSrcHeight - 1);
      iIndexY[1] = WELS_CLAMP(iScale*j    , 0, iSrcHeight - 1);
      iIndexY[2] = WELS_CLAMP(iScale*j + iScaleSuf + 1, 0, iSrcHeight - 1);
      iIndexY[3] = WELS_CLAMP(iScale*j + iScaleSuf + 2, 0, iSrcHeight - 1);
      
      B[0][0] = iTemp[0] = pSrc[iIndexY[0] * iSrcStride + iIndexX[0]];
      B[0][1] = iTemp[1] = pSrc[iIndexY[0] * iSrcStride + iIndexX[1]];
      B[0][2] = iTemp[2] = pSrc[iIndexY[0] * iSrcStride + iIndexX[2]];
      B[0][3] = iTemp[3] = pSrc[iIndexY[0] * iSrcStride + iIndexX[3]];
      
      B[1][0] = iTemp[4 ] = pSrc[iIndexY[1] * iSrcStride + iIndexX[0]];
      B[1][1] = iTemp[5 ] = pSrc[iIndexY[1] * iSrcStride + iIndexX[1]];
      B[1][2] = iTemp[6 ] = pSrc[iIndexY[1] * iSrcStride + iIndexX[1]];
      B[1][3] = iTemp[7 ] = pSrc[iIndexY[1] * iSrcStride + iIndexX[3]];
      
      B[2][0] = iTemp[8 ] = pSrc[iIndexY[2] * iSrcStride + iIndexX[0]];
      B[2][1] = iTemp[9 ] = pSrc[iIndexY[2] * iSrcStride + iIndexX[1]];
      B[2][2] = iTemp[10] = pSrc[iIndexY[2] * iSrcStride + iIndexX[1]];
      B[2][3] = iTemp[11] = pSrc[iIndexY[2] * iSrcStride + iIndexX[3]];
      
      B[3][0] = iTemp[12] = pSrc[iIndexY[3] * iSrcStride + iIndexX[0]];
      B[3][1] = iTemp[13] = pSrc[iIndexY[3] * iSrcStride + iIndexX[1]];
      B[3][2] = iTemp[14] = pSrc[iIndexY[3] * iSrcStride + iIndexX[1]];
      B[3][3] = iTemp[15] = pSrc[iIndexY[3] * iSrcStride + iIndexX[3]];
      
      //      cout << iTemp[0] << "," << iTemp[1] << "," << iTemp[2] << "," << iTemp[3] << endl;
      //      cout << iTemp[4] << "," << iTemp[5] << "," << iTemp[6] << "," << iTemp[7] << endl;
      //      cout << iTemp[8] << "," << iTemp[9] << "," << iTemp[10] << "," << iTemp[11] << endl;
      //      cout << iTemp[12] << "," << iTemp[13] << "," << iTemp[14] << "," << iTemp[15] << endl;
      
      for (int n = 0; n < 4; n++) {
        fTempArray[n] = 0.0;
        for (int m = 0; m < 4; m++) {
          fTempArray[n] += AC[m] * B[m][n];
        }
      }
      
      fTempResult = 0.0;
      for (int n = 0; n < 4; n++) {
        fTempResult += fTempArray[n] * AC[n];
      }
      
      if (fTempResult > B[1][1]) {
        fTempResult = B[1][1];
        //fTempResult -= 2;
      }
      
      pDst[j * iDstStride + i] = WELS_CLAMP((int)fTempResult, 0, 255);

    }
  }
}
