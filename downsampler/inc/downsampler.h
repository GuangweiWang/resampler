#ifndef _DOWN_SAMPLER_H_
#define _DOWN_SAMPLER_H_

#include "my_typedefs.h"
#include <stdio.h>

//define macros
#ifndef WELS_ROUND
#define WELS_ROUND(x) ((int32_t)(0.5+(x)))
#endif//WELS_ROUND

#define WELS_MAX(x, y) ((x) > (y) ? (x) : (y))
#define WELS_MIN(x, y) ((x) < (y) ? (x) : (y))

#define WELS_CLAMP(x, minv, maxv)  WELS_MIN(WELS_MAX(x, minv), maxv)

//global data struct
typedef struct {
  uint8_t*		pInfileName;
  uint8_t*		pOutfileName;

  int32_t		iSrcWidth;
  int32_t		iSrcHeight;
  int32_t		iDstWidth;
  int32_t		iDstHeight;

  int32_t		iDownsampleMethod;
  int32_t		iDownsampleMode;
  int32_t		iInputFrameNum;
  int32_t		iOutputFrameNum;
}SDownsamplerCtx;

typedef struct {
  //frame data
  uint8_t*		pDataY;
  uint8_t*		pDataU;
  uint8_t*		pDataV;

  //stride
  uint32_t		iStrideY;
  uint32_t		iStrideU;
  uint32_t		iStrideV;

  //width
  uint32_t		iWidthY;
  uint32_t		iWidthU;
  uint32_t		iWidthV;

  //height
  uint32_t		iHeightY;
  uint32_t		iHeightU;
  uint32_t		iHeightV;

  //frame size
  //uint32_t		iFrameSizeY;
  //uint32_t		iFrameSizeU;
  //uint32_t		iFrameSizeV;

}SYUVFrame;

typedef enum {
  BILINEAR_DOWN_SAMPLER		= 0,
  JSVM_DOWN_SAMPLER			= 1,
  TAP_DOWN_SAMPLER			= 2,
  UNDEFINED_DOWN_SAMPLER	= 9
}EDownsampleMethod;

typedef enum {
  SIMPLE		= 0,
  COMPLEX		= 1,
  UNDEFINED		= 9
}EDownsampleMode;

class Downsampler {
public:
  Downsampler();
  virtual ~Downsampler();

public:
  void          CreatDownsampler(SDownsamplerCtx* pDownsamplerCtx);
  void			GetOption(SDownsamplerCtx* pDownsamplerCtx);
  void			DestroyDownsampler();
  virtual void	InitDownsampler(SDownsamplerCtx* pDownsamplerCtx);

public:
  int32_t		ReadFrameFromFile(FILE* fIn);
  int32_t		WriteFrameToFile(FILE* fout);
  virtual void			Processing();

public:
  bool					m_bInitDone;
  bool                  m_bCreatDone;
  long long				m_iStartTime;
  long long				m_iTotalTime;
  uint32_t				m_iInputFrameNum;
  uint32_t				m_iOutputFrameNum;
  uint32_t				m_iReadFrameNum;
  uint32_t				m_iWriteFrameNum;
  EDownsampleMethod		m_eDownsampleMethod;
  EDownsampleMode		m_eDownsampleMode;

  SYUVFrame		m_sSrcFrame;
  SYUVFrame 	m_sDstFrame;
};//father class end

//bilinear downsampler functions
class BilinearDownsampler: public Downsampler {
  
public:
  BilinearDownsampler();
  ~BilinearDownsampler();
  
public:
  void InitDownsampler(SDownsamplerCtx* pDownsamplerCtx);
  void Processing ();

private:
  void    BilinearDownsamplerHalf_c(uint8_t* pDst,const int32_t kiDstStride, uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight);
  
  void    BilinearDownsamplerOneThird_c(uint8_t* pDst,const int32_t kiDstStride, uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight);
  
  void    BilinearDownsamplerQuarter_c(uint8_t* pDst,const int32_t kiDstStride, uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight);
  
  void    GeneralBilinearFastDownsampler_c(uint8_t* pDst, const int32_t kiDstStride,const int32_t kiDstWidth, const int32_t kiDstHeight,uint8_t* pSrc, const int32_t kiSrcStride,const int32_t kiSrcWidth, const int32_t kiSrcHeight);
  
  void    GeneralBilinearAccurateDownsampler_c(uint8_t* pDst, const int32_t kiDstStride,const int32_t kiDstWidth, const int32_t kiDstHeight,uint8_t* pSrc, const int32_t kiSrcStride,const int32_t kiSrcWidth, const int32_t kiSrcHeight);
};

//bicubic downsampler
class BicubicDownsampler: public Downsampler {
public:
  BicubicDownsampler();
  ~BicubicDownsampler();
  
public:
  void CreatBicubicDownsampler();
  void DestroyBicubicDownsampler();
  
public:
  void Processing ();
  void InitDownsampler(SDownsamplerCtx* pDownsamplerCtx);

private:
  void BicubicInterpolation(unsigned char* pDst, int iDstStride,
                            int iDstWidth, int iDstHeight,
                            unsigned char* pSrc, int iSrcStride,
                            int iSrcWidth, int iSrcHeight);
private:
  bool          m_bInitBicubicDownsamplerDone;
  bool          m_bCreatBicubicDownsamplerDone;
  int*          m_pTempBuffer;
};

//jsvm downsampler
class BenchmarkDownsampler: public Downsampler {
public:
  BenchmarkDownsampler();
  ~BenchmarkDownsampler();
  
public:
  void CreatBenchmarkDownsampler();
  void DestroyBenchmarkDownsampler();
  
public:
  void InitDownsampler(SDownsamplerCtx* pDownsampler);
  void Processing ();
  
private:
  void FlushBuffer(unsigned char* pDst, int iDstStride,
                   unsigned char* pSrc, int iSrcStride,
                   int iWidth, int iHeight);
  
  void Tap13DownsamplerHalf(unsigned char* pDst, int iDstStride,
                              unsigned char* pSrc, int iSrcStride,
                              int iSrcWidth, int iSrcHeight);
  void Tap8DownsamplerHalf(unsigned char* pDst, int iDstStride,
                            unsigned char* pSrc, int iSrcStride,
                            int iSrcWidth, int iSrcHeight);
  void Tap6DownsamplerHalf(unsigned char* pDst, int iDstStride,
                           unsigned char* pSrc, int iSrcStride,
                           int iSrcWidth, int iSrcHeight);
  void Tap13Downsampler();
  void Tap8Downsampler();
  void Tap6Downsampler();
  
private:
  bool    m_bCreatBenchmarkDownsampler;
  int*    m_pTempBuffer;
};

//my testdownsampler
class TestDownsampler: public Downsampler {
public:
  TestDownsampler();
  ~TestDownsampler();
  
public:
  void Processing ();
  void InitDownsampler(SDownsamplerCtx* pDownsamplerCtx);
  void CreatTestDownsampler();
  void DestroyTestDownsampler();
  
private:
  void Tab8DownsamplerHalf(unsigned char* pDst, int iDstStride,
                           unsigned char* pSrc, int iSrcStride,
                           int iSrcWidth, int iSrcHeight);
  void BilinearDownsamplerHalf_c(unsigned char* pDst, int kiDstStride,
                                 unsigned char* pSrc, int kiSrcStride,
                                 int kiSrcWidth, int kiSrcHeight);
  
private:
  bool          m_bCreatTestDownsamplerDone;
  int*          m_pTempBuffer;
};

//tap filter downsampler
class TapFilterDownsampler: public Downsampler {
public:
  TapFilterDownsampler();
  ~TapFilterDownsampler();
  
public:
  void CreatTapFilterDownsampler();
  void DestroyTapFilterDownsampler();
  
public:
  void InitDownsampler(SDownsamplerCtx* pDownsampler);
  void Processing ();
  
private:
  void FlushBuffer(unsigned char* pDst, int iDstStride,
                   unsigned char* pSrc, int iSrcStride,
                   int iWidth, int iHeight);
  
  void Tap13DownsamplerHalf(unsigned char* pDst, int iDstStride,
                            unsigned char* pSrc, int iSrcStride,
                            int iSrcWidth, int iSrcHeight);
  void Tap8DownsamplerHalf(unsigned char* pDst, int iDstStride,
                           unsigned char* pSrc, int iSrcStride,
                           int iSrcWidth, int iSrcHeight);
  void Tap6DownsamplerHalf(unsigned char* pDst, int iDstStride,
                           unsigned char* pSrc, int iSrcStride,
                           int iSrcWidth, int iSrcHeight);
  void Tap13Downsampler();
  void Tap8Downsampler();
  void Tap6Downsampler();
  
private:
  bool    m_bCreatTapFilterDownsampler;
  int*    m_pTempBuffer;
};
#endif
