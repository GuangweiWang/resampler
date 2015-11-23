#include <iostream>
#include "downsampler.h"
#include "measure_time.h"

using namespace std;

TestDownsampler::TestDownsampler() {
  m_bCreatTestDownsamplerDone = false;
  m_pTempBuffer = NULL;
}

TestDownsampler::~TestDownsampler() {
  DestroyTestDownsampler();
}

void TestDownsampler::CreatTestDownsampler() {
  if (!m_bCreatTestDownsamplerDone) {
    m_bCreatTestDownsamplerDone = true;

    m_pTempBuffer = new int [m_sSrcFrame.iStrideY * m_sSrcFrame.iHeightY];
    if (!m_pTempBuffer) {
      cout << "malloc TestDownsampler memory error!" << endl;
      exit(1);
    }
  }else {
    cout << "can not creat an exist TestDownsampler!" << endl;
    exit(1);
  }
}

void TestDownsampler::DestroyTestDownsampler() {
  if (m_bCreatTestDownsamplerDone) {
    m_bCreatTestDownsamplerDone = false;
   
    if (m_pTempBuffer) {
      delete m_pTempBuffer;
      m_pTempBuffer = NULL;
    }
  }else {
    cout << "can not destroy TestDownsampler agin!" << endl;
    exit(1);
  }
}

void TestDownsampler::InitDownsampler(SDownsamplerCtx* pDownsamplerCtx) {
  GetOption(pDownsamplerCtx);
  CreatDownsampler(pDownsamplerCtx);
  CreatTestDownsampler();
}

void TestDownsampler::Processing () {
  //cout << "this is a test downsampler " << endl;
  long long iStartTime = 0;

  iStartTime = WelsTime();
  Tab8DownsamplerHalf(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                       m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                       m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
//  Tab13DownsamplerHalf(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
//                       m_sDstFrame.iWidthU, m_sDstFrame.iHeightU,
//                       m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
//                       m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
//  Tab13DownsamplerHalf(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
//                       m_sDstFrame.iWidthV, m_sDstFrame.iHeightV,
//                       m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
//                       m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);

  BilinearDownsamplerHalf_c(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                      m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                      m_sSrcFrame.iWidthU,m_sSrcFrame.iHeightU);
  BilinearDownsamplerHalf_c(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                            m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                            m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
  
  m_iTotalTime += WelsTime() - iStartTime;
}

void TestDownsampler::Tab8DownsamplerHalf(unsigned char* pDst, int iDstStride,
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

void TestDownsampler::BilinearDownsamplerHalf_c(uint8_t* pDst,const int32_t kiDstStride, uint8_t* pSrc, const int32_t kiSrcStride,const int32_t kiSrcWidth, const int32_t kiSrcHeight)
{
  uint8_t* pDstLine = pDst;
  uint8_t* pSrcLine = pSrc;
  const int32_t kiSrcStridex2 = kiSrcStride << 1;
  const int32_t kiDstWidth = kiSrcWidth >> 1;
  const int32_t kiDstHeight = kiSrcHeight >> 1;
  
  for (int32_t j = 0; j < kiDstHeight; j++) {
    for (int32_t i = 0; i < kiDstWidth; i++) {
      const int32_t kiSrcX = i << 1;
      const int32_t kiTempRow1 = (pSrcLine[kiSrcX] + pSrcLine[kiSrcX + 1] + 1) >> 1;
      const int32_t kiTempRow2 = (pSrcLine[kiSrcX + kiSrcStride] + pSrcLine[kiSrcX + kiSrcStride + 1] + 1) >> 1;
      
      pDstLine[i] = (uint8_t)((kiTempRow1 + kiTempRow2 + 1) >> 1);
    }
    pDstLine += kiDstStride;
    pSrcLine += kiSrcStridex2;
  }
}
