//
//  TapFilterDownsampler.cpp
//  downsampler
//
//  Created by Guangwei Wang on 11/19/15.
//  Copyright © 2015 Guangwei Wang. All rights reserved.
//

#include <stdio.h>
#include <iostream>
#include "downsampler.h"
#include "measure_time.h"

using namespace std;

TapFilterDownsampler::TapFilterDownsampler() {
  m_bCreatTapFilterDownsampler = false;
  m_pTempBuffer = NULL;
}

TapFilterDownsampler::~TapFilterDownsampler() {
  DestroyTapFilterDownsampler();
}

void TapFilterDownsampler::CreatTapFilterDownsampler() {
  if (!m_bCreatTapFilterDownsampler) {
    m_bCreatTapFilterDownsampler = true;
    
    m_pTempBuffer = new int [m_sSrcFrame.iStrideY * m_sSrcFrame.iHeightY];
    if (!m_pTempBuffer ) {
      cout << "malloc tap-filter Downsampler memory error!" << endl;
      exit(1);
    }
  }else {
    cout << "can not creat an exist tap-filter Downsampler!" << endl;
    exit(1);
  }
}

void TapFilterDownsampler::DestroyTapFilterDownsampler() {
  if (m_bCreatTapFilterDownsampler) {
    m_bCreatTapFilterDownsampler = false;
    
    if (m_pTempBuffer) {
      delete m_pTempBuffer;
      m_pTempBuffer = NULL;
    }
    
  }else {
    cout << "can not destroy TestDownsampler agin!" << endl;
    exit(1);
  }
}

void TapFilterDownsampler::InitDownsampler(SDownsamplerCtx* pDownsamplerCtx) {
  GetOption(pDownsamplerCtx);
  CreatDownsampler(pDownsamplerCtx);
  CreatTapFilterDownsampler();
}

void TapFilterDownsampler::Processing () {
  long long iStartTime = 0;
  
  iStartTime = WelsTime();
  Tap6Downsampler();
  m_iTotalTime += WelsTime() - iStartTime;
}

void TapFilterDownsampler::FlushBuffer(unsigned char* pDst, int iDstStride,
                                       unsigned char* pSrc, int iSrcStride,
                                       int iWidth, int iHeight) {
  for (int i = 0; i < iHeight; i++) {
    memcpy(pDst, pSrc, iWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
  
}

void TapFilterDownsampler::Tap13Downsampler() {
  
  int ratio = m_sSrcFrame.iWidthY / m_sDstFrame.iWidthY;
  
  if (ratio == 2) {
    Tap13DownsamplerHalf(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                         m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                         m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
    
    Tap13DownsamplerHalf(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                         m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                         m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
    Tap13DownsamplerHalf(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                         m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                         m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
  }else if (ratio == 4) {
    
    Tap13DownsamplerHalf(m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                         m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                         m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
    Tap13DownsamplerHalf(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                         m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                         m_sSrcFrame.iWidthY >> 1, m_sSrcFrame.iHeightY >> 1);
    
    Tap13DownsamplerHalf(m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                         m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                         m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
    Tap13DownsamplerHalf(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                         m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                         m_sSrcFrame.iWidthU >> 1, m_sSrcFrame.iHeightU >> 1);
    
    Tap13DownsamplerHalf(m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                         m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                         m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
    Tap13DownsamplerHalf(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                         m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                         m_sSrcFrame.iWidthV >> 1, m_sSrcFrame.iHeightV >> 1);
  }else {
    cout << "this ratio not support yet!" << endl;
  }
  
}

void TapFilterDownsampler::Tap8Downsampler() {
  Tap8DownsamplerHalf(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                      m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                      m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
  Tap8DownsamplerHalf(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                      m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                      m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
  Tap8DownsamplerHalf(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                      m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                      m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
}

void TapFilterDownsampler::Tap6Downsampler() {
  Tap6DownsamplerHalf(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                      m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                      m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
  Tap6DownsamplerHalf(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                      m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                      m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
  Tap6DownsamplerHalf(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                      m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                      m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
}

void TapFilterDownsampler::Tap13DownsamplerHalf(unsigned char* pDst, int iDstStride,
                                                unsigned char* pSrc, int iSrcStride,
                                                int iSrcWidth, int iSrcHeight) {
  int iDstWidth = iSrcWidth >> 1;
  int iDstHeight = iSrcHeight >> 1;
  const int iFilterCoeff13[13] = {2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2};
  int i = 0, j = 0, k = 0, iIndex = 0;
  
  unsigned char* pTempSrc = pSrc;
  unsigned char* pTempDst = pDst;
  int*           pTemp    = m_pTempBuffer;
  int iTemp = 0;
  
  for (j = 0; j < iSrcHeight; j++) {
    memset(pTemp, 0, iDstWidth * sizeof(int));
    for (i = 0; i < iDstWidth; i++) {
      for (k = 0; k < 13; k++) {
        iIndex = WELS_CLAMP(2*i - 6 + k, 0, iSrcWidth - 1);
        pTemp[i] += iFilterCoeff13[k] * pTempSrc[iIndex];
      }
    }
    pTemp += iSrcStride;
    pTempSrc += iSrcStride;
  }
  
  pTemp = m_pTempBuffer;
  for (i = 0; i < iDstWidth; i++) {
    for (j = 0; j < iDstHeight; j++) {
      iTemp = 0;
      for (k = 0; k < 13; k++) {
        iIndex = WELS_CLAMP(2*j - 6 + k, 0, iSrcHeight - 1);
        iTemp += iFilterCoeff13[k] * pTemp[iIndex * iSrcStride];
      }
      iTemp = (iTemp + 2048) >> 12;
      pTempDst[j*iDstStride + i] = WELS_CLAMP(iTemp, 0, 255);
    }
    pTemp += 1;
  }
}

void TapFilterDownsampler::Tap8DownsamplerHalf(unsigned char* pDst, int iDstStride,
                                               unsigned char* pSrc, int iSrcStride,
                                               int iSrcWidth, int iSrcHeight) {
  int iDstWidth = iSrcWidth >> 1;
  int iDstHeight = iSrcHeight >> 1;
  const int iFilterCoeff8[8] = {-8,   0,  24,  48,  48,  24,   0,  -8};
  int i = 0, j = 0, k = 0, iIndex = 0;
  
  unsigned char* pTempSrc = pSrc;
  unsigned char* pTempDst = pDst;
  int*           pTemp    = m_pTempBuffer;
  int iTemp = 0;
  
  for (j = 0; j < iSrcHeight; j++) {
    memset(pTemp, 0, iDstWidth * sizeof(int));
    for (i = 0; i < iDstWidth; i++) {
      for (k = 0; k < 8; k++) {
        iIndex = WELS_CLAMP(2 * i - 3 + k, 0, iSrcWidth - 1);
        pTemp[i] += iFilterCoeff8[k] * pTempSrc[iIndex];
      }
    }
    pTemp += iSrcStride;
    pTempSrc += iSrcStride;
  }
  
  pTemp = m_pTempBuffer;
  for (i = 0; i < iDstWidth; i++) {
    for (j = 0; j < iDstHeight; j++) {
      iTemp = 0;
      for (k = 0; k < 8; k++) {
        iIndex = WELS_CLAMP(2 * j - 3 + k, 0, iSrcHeight - 1);
        iTemp += iFilterCoeff8[k] * pTemp[iIndex * iSrcStride];
      }
      iTemp = (iTemp + 8192) >> 14;
      pTempDst[j*iDstStride + i] = WELS_CLAMP(iTemp, 0, 255);
    }
    pTemp += 1;
  }
}

void TapFilterDownsampler::Tap6DownsamplerHalf(unsigned char* pDst, int iDstStride,
                                               unsigned char* pSrc, int iSrcStride,
                                               int iSrcWidth, int iSrcHeight) {
  int iDstWidth = iSrcWidth >> 1;
  int iDstHeight = iSrcHeight >> 1;
  const int iFilterCoeff6[6] = {1, -5, 20, 20, -5, 1};
  //const int iFilterCoeff8[8] = {-8,   0,  24,  48,  48,  24,   0,  -8};
  int i = 0, j = 0, k = 0, iIndex = 0;
  
  unsigned char* pTempSrc = pSrc;
  unsigned char* pTempDst = pDst;
  int*           pTemp    = m_pTempBuffer;
  int iTemp = 0;
  
  for (j = 0; j < iSrcHeight; j++) {
    memset(pTemp, 0, iDstWidth * sizeof(int));
    for (i = 0; i < iDstWidth; i++) {
      for (k = 0; k < 6; k++) {
        iIndex = WELS_CLAMP(2 * i - 2 + k, 0, iSrcWidth - 1);
        pTemp[i] += iFilterCoeff6[k] * pTempSrc[iIndex];
      }
    }
    pTemp += iSrcStride;
    pTempSrc += iSrcStride;
  }
  
  pTemp = m_pTempBuffer;
  for (i = 0; i < iDstWidth; i++) {
    for (j = 0; j < iDstHeight; j++) {
      iTemp = 0;
      for (k = 0; k < 6; k++) {
        iIndex = WELS_CLAMP(2 * j - 2 + k, 0, iSrcHeight - 1);
        iTemp += iFilterCoeff6[k] * pTemp[iIndex * iSrcStride];
      }
      iTemp = (iTemp + 512) >> 10;
      //iTemp = (iTemp + 8192) >> 14;
      pTempDst[j*iDstStride + i] = WELS_CLAMP(iTemp, 0, 255);
    }
    pTemp += 1;
  }
}