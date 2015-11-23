
#include "downsampler.h"
#include "measure_time.h"

BilinearDownsampler::BilinearDownsampler() {
  
}

BilinearDownsampler::~BilinearDownsampler() {
  
}

void BilinearDownsampler::InitDownsampler(SDownsamplerCtx* pDownsamplerCtx) {
  GetOption(pDownsamplerCtx);
  CreatDownsampler(pDownsamplerCtx);
}

void BilinearDownsampler::Processing () {

  uint32_t iSrcWidth = m_sSrcFrame.iWidthY;
  uint32_t iDstWidth = m_sDstFrame.iWidthY;
  uint32_t iSrcHeight = m_sSrcFrame.iHeightY;
  uint32_t iDstHeight = m_sDstFrame.iHeightY;
  
  if(iSrcWidth/2 == iDstWidth && iSrcHeight/2 == iDstHeight) {
    //down scale == 2, use specific function
    m_iStartTime = WelsTime();
    BilinearDownsamplerHalf_c(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                              m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                              m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
    BilinearDownsamplerHalf_c(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                              m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                              m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
    BilinearDownsamplerHalf_c(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                              m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                              m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
    m_iTotalTime += WelsTime() - m_iStartTime;
    
  }else if(iSrcWidth/3 == iDstWidth && iSrcHeight/3 == iDstHeight) {
    //down scale == 3,use specific function
    m_iStartTime = WelsTime();
    BilinearDownsamplerOneThird_c(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                                  m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                                  m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
    BilinearDownsamplerOneThird_c(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                                  m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                                  m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
    BilinearDownsamplerOneThird_c(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                                  m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                                  m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
    m_iTotalTime += WelsTime() - m_iStartTime;
    
  }else if(iSrcWidth/4 == iDstWidth && iSrcHeight/4 == iDstHeight) {
    //down scale == 4, use specific function
    m_iStartTime = WelsTime();
    BilinearDownsamplerQuarter_c(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                                 m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                                 m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
    BilinearDownsamplerQuarter_c(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                                 m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                                 m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
    BilinearDownsamplerQuarter_c(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                                 m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                                 m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
    
    m_iTotalTime += WelsTime() - m_iStartTime;
  }else {
    // use general function
    m_iStartTime = WelsTime();
    GeneralBilinearAccurateDownsampler_c(m_sDstFrame.pDataY, m_sDstFrame.iStrideY,
                                         m_sDstFrame.iWidthY, m_sDstFrame.iHeightY,
                                         m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY,
                                         m_sSrcFrame.iWidthY, m_sSrcFrame.iHeightY);
    GeneralBilinearAccurateDownsampler_c(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
                                         m_sDstFrame.iWidthU, m_sDstFrame.iHeightU,
                                         m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
                                         m_sSrcFrame.iWidthU, m_sSrcFrame.iHeightU);
    GeneralBilinearAccurateDownsampler_c(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
                                         m_sDstFrame.iWidthV, m_sDstFrame.iHeightV,
                                         m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
                                         m_sSrcFrame.iWidthV, m_sSrcFrame.iHeightV);
    m_iTotalTime += WelsTime() - m_iStartTime;
  }
}


void BilinearDownsampler::BilinearDownsamplerHalf_c(uint8_t* pDst,const int32_t kiDstStride, uint8_t* pSrc, const int32_t kiSrcStride,const int32_t kiSrcWidth, const int32_t kiSrcHeight)
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

void BilinearDownsampler::BilinearDownsamplerOneThird_c(unsigned char* dst,const int dst_stride, unsigned char* src, const int src_stride,const int src_width, const int src_height)
{
  unsigned char *dst_line = dst;
  unsigned char *src_line = src;
  const int src_stridex3 = src_stride * 3;
  const int dst_width = src_width/3;
  const int dst_height = src_height/3;
  int j = 0, i = 0;
  
  for (j = 0; j < dst_height; j++)
  {
    for (i = 0; i < dst_width; i++)
    {
      const int src_x = i * 3 ;
      const int temp_row1 = (src_line[src_x] + src_line[src_x + 1] + 1) >> 1;
      const int temp_row2 = (src_line[src_x + src_stride] + src_line[src_x + src_stride + 1] + 1) >> 1;
      
      dst_line[i] = (unsigned char)((temp_row1 + temp_row2 + 1) >> 1);
    }
    dst_line += dst_stride;
    src_line += src_stridex3;
  }
}

void BilinearDownsampler::BilinearDownsamplerQuarter_c(unsigned char* dst,const int dst_stride, unsigned char* src, const int src_stride,const int src_width, const int src_height)

{
  unsigned char *dst_line = dst;
  unsigned char *src_line = src;
  const int src_stride4 = src_stride << 2;
  const int dst_width = src_width >> 2;
  const int dst_height = src_height >> 2;
  int j = 0, i = 0;
  
  for (j = 0; j < dst_height; j++)
  {
    for (i = 0; i < dst_width; i++)
    {
      const int src_x = i << 2;
      const int temp_row1 = (src_line[src_x] + src_line[src_x + 1] + 1) >> 1;
      const int temp_row2 = (src_line[src_x + src_stride] + src_line[src_x + src_stride + 1] + 1) >> 1;
      
      dst_line[i] = (unsigned char)((temp_row1 + temp_row2 + 1) >> 1);
    }
    dst_line += dst_stride;
    src_line += src_stride4;
  }
}

void BilinearDownsampler::GeneralBilinearFastDownsampler_c(uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth, const int32_t kiDstHeight,uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight)
{
  const uint32_t kuiScaleBitWidth = 16, kuiScaleBitHeight = 15;
  const uint32_t kuiScaleWidth = (1 << kuiScaleBitWidth), kuiScaleHeight = (1 << kuiScaleBitHeight);
  int32_t fScalex = WELS_ROUND((float)kiSrcWidth / (float)kiDstWidth * kuiScaleWidth);
  int32_t fScaley = WELS_ROUND((float)kiSrcHeight / (float)kiDstHeight * kuiScaleHeight);
  uint32_t x;
  int32_t iYInverse, iXInverse;
  
  uint8_t* pByDst = pDst;
  uint8_t* pByLineDst = pDst;
  
  iYInverse = 1 << (kuiScaleBitHeight - 1);
  for (int32_t i = 0; i < kiDstHeight - 1; i++) {
    int32_t iYy = iYInverse >> kuiScaleBitHeight;
    int32_t fv = iYInverse & (kuiScaleHeight - 1);
    
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;
    
    pByDst = pByLineDst;
    iXInverse = 1 << (kuiScaleBitWidth - 1);
    for (int32_t j = 0; j < kiDstWidth - 1; j++) {
      int32_t iXx = iXInverse >> kuiScaleBitWidth;
      int32_t iFu = iXInverse & (kuiScaleWidth - 1);
      
      uint8_t* pByCurrent = pBySrc + iXx;
      uint8_t a, b, c, d;
      
      a = *pByCurrent;
      b = *(pByCurrent + 1);
      c = *(pByCurrent + kiSrcStride);
      d = *(pByCurrent + kiSrcStride + 1);
      
      x = (((uint32_t)(kuiScaleWidth - 1 - iFu)) * (kuiScaleHeight - 1 - fv) >> kuiScaleBitWidth) * a;
      x += (((uint32_t)(iFu)) * (kuiScaleHeight - 1 - fv) >> kuiScaleBitWidth) * b;
      x += (((uint32_t)(kuiScaleWidth - 1 - iFu)) * (fv) >> kuiScaleBitWidth) * c;
      x += (((uint32_t)(iFu)) * (fv) >> kuiScaleBitWidth) * d;
      x >>= (kuiScaleBitHeight - 1);
      x += 1;
      x >>= 1;
      //x = (((__int64)(SCALE_BIG - 1 - iFu))*(SCALE_BIG - 1 - fv)*a + ((__int64)iFu)*(SCALE_BIG - 1 -fv)*b + ((__int64)(SCALE_BIG - 1 -iFu))*fv*c +
      // ((__int64)iFu)*fv*d + (1 << (2*SCALE_BIT_BIG-1)) ) >> (2*SCALE_BIT_BIG);
      x = WELS_CLAMP(x, 0, 255);
      *pByDst++ = (uint8_t)x;
      
      iXInverse += fScalex;
    }
    *pByDst = *(pBySrc + (iXInverse >> kuiScaleBitWidth));
    pByLineDst += kiDstStride;
    iYInverse += fScaley;
  }
  
  // last row special
  {
    int32_t iYy = iYInverse >> kuiScaleBitHeight;
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;
    
    pByDst = pByLineDst;
    iXInverse = 1 << (kuiScaleBitWidth - 1);
    for (int32_t j = 0; j < kiDstWidth; j++) {
      int32_t iXx = iXInverse >> kuiScaleBitWidth;
      *pByDst++ = *(pBySrc + iXx);
      
      iXInverse += fScalex;
    }
  }
}

void BilinearDownsampler::GeneralBilinearAccurateDownsampler_c(uint8_t* pDst, const int32_t kiDstStride,
const int32_t kiDstWidth, const int32_t kiDstHeight, uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight)
{
  const int32_t kiScaleBit = 15;
  const int32_t kiScale = (1 << kiScaleBit);
  int32_t iScalex = WELS_ROUND((float)kiSrcWidth / (float)kiDstWidth * kiScale);
  int32_t iScaley = WELS_ROUND((float)kiSrcHeight / (float)kiDstHeight * kiScale);
  int64_t x;
  int32_t iYInverse, iXInverse;
  
  uint8_t* pByDst = pDst;
  uint8_t* pByLineDst = pDst;
  
  iYInverse = 1 << (kiScaleBit - 1);
  for (int32_t i = 0; i < kiDstHeight - 1; i++) {
    int32_t iYy = iYInverse >> kiScaleBit;
    int32_t iFv = iYInverse & (kiScale - 1);
    
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;
    
    pByDst = pByLineDst;
    iXInverse = 1 << (kiScaleBit - 1);
    for (int32_t j = 0; j < kiDstWidth - 1; j++) {
      int32_t iXx = iXInverse >> kiScaleBit;
      int32_t iFu = iXInverse & (kiScale - 1);
      
      uint8_t* pByCurrent = pBySrc + iXx;
      uint8_t a, b, c, d;
      
      a = *pByCurrent;
      b = *(pByCurrent + 1);
      c = *(pByCurrent + kiSrcStride);
      d = *(pByCurrent + kiSrcStride + 1);
      
      x = (((int64_t)(kiScale - 1 - iFu)) * (kiScale - 1 - iFv) * a + ((int64_t)iFu) * (kiScale - 1 - iFv) * b + ((int64_t)(
                                                                                                                            kiScale - 1 - iFu)) * iFv * c +
           ((int64_t)iFu) * iFv * d + (int64_t)(1 << (2 * kiScaleBit - 1))) >> (2 * kiScaleBit);
      x = WELS_CLAMP(x, 0, 255);
      *pByDst++ = (uint8_t)x;
      
      iXInverse += iScalex;
    }
    *pByDst = *(pBySrc + (iXInverse >> kiScaleBit));
    pByLineDst += kiDstStride;
    iYInverse += iScaley;
  }
  
  // last row special
  {
    int32_t iYy = iYInverse >> kiScaleBit;
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;
    
    pByDst = pByLineDst;
    iXInverse = 1 << (kiScaleBit - 1);
    for (int32_t j = 0; j < kiDstWidth; j++) {
      int32_t iXx = iXInverse >> kiScaleBit;
      *pByDst++ = *(pBySrc + iXx);
      
      iXInverse += iScalex;
    }
  }
}

