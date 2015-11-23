
#ifndef _DOWN_CONVERT_
#define _DOWN_CONVERT_


#include "ResizeParameters.h"

#ifndef  gMax
#define  gMax(x,y)   ((x)>(y)?(x):(y))
#define  gMin(x,y)   ((x)<(y)?(x):(y))
#endif

class DownConvert
{
public:
  //========================
  // general main functions
  //========================
  DownConvert   ();
  ~DownConvert  ();
  bool  init    ( int iMaxWidth, int iMaxHeight, int iMaxMargin = 0 );
  void  destroy ();

  //=====================================
  // main functions for DownConvert Tool
  //=====================================
  void  cropping                    ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters );
  void  upsamplingDyadic            ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters );
  void  upsamplingLanczos           ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters );
  void  upsampling6tapBilin         ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters );
  void  upsamplingSVC               ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters,  bool  bBotCoincided = false );
  void  downsamplingDyadic          ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters );
  void  downsamplingSVC             ( unsigned char*          pucBufferY,    int   iStrideY,
                                      unsigned char*          pucBufferU,    int   iStrideU,
                                      unsigned char*          pucBufferV,    int   iStrideV,
                                      ResizeParameters*       pcParameters,  bool  bBotCoincided = false );


  //==========================
  // general helper functions
  //==========================
  //--- delete buffers ---
  void  xDestroy                    ();
  //--- general clipping ---
  int   xClip                       ( int                     iValue,
                                      int                     imin,
                                      int                     imax );
  //--- SVC normative intra upsampling ---
  void  xCompIntraUpsampling        ( ResizeParameters*       pcParameters,
                                      bool                    bChroma,
                                      bool                    bBotFlag,
                                      bool                    bVerticalInterpolation,
                                      int                     iMargin = 0 );
  void  xVertIntraUpsampling        ( int  iBaseW,   int  iBaseH,
                                      int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                      int  iYBorder, bool bBotFlag, bool bChromaFilter );
  void  xBasicIntraUpsampling       ( int  iBaseW,   int  iBaseH,   int  iCurrW,   int  iCurrH,
                                      int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                      int  iShiftX,  int  iShiftY,  int  iScaleX,  int  iScaleY,
                                      int  iOffsetX, int  iOffsetY, int  iAddX,    int  iAddY,
                                      int  iDeltaX,  int  iDeltaY,  int  iYBorder, bool bChromaFilter, int iMargin );

  //=======================================
  // helper functions for DownConvert Tool
  //=======================================
  //--- initialization ---
  void  xInitLanczosFilter          ();
  //--- place to and get from image buffer ---
  void  xCopyToImageBuffer          ( unsigned char*        pucSrc,
                                      int                   iWidth,
                                      int                   iHeight,
                                      int                   iStride );
  void  xCopyFromImageBuffer        ( unsigned char*        pucDes,
                                      int                   iWidth,
                                      int                   iHeight,
                                      int                   iStride );
  void  xInitializeWithValue        ( unsigned char*        pucBuffer,
                                      int                   iWidth,
                                      int                   iHeight,
                                      int                   iStride,
                                      unsigned char         cValue );
  //--- dyadic upsampling ---
  void  xCompUpsamplingDyadic       ( int                   iBaseW,
                                      int                   iBaseH,
                                      bool                  bChroma );
  //--- Lanczos upsampling ---
  void  xCompUpsamplingLanczos      ( ResizeParameters*     pcParameters,
                                      bool                  bChroma );
  void  xUpsamplingDataLanczos      ( int                   iInLength,
                                      int                   iOutLength,
                                      long                  spos );
  void  xGetNumDenomLanczos         ( int                   iInWidth,
                                      int                   iOutWidth,
                                      int&                  riNumerator,
                                      int&                  riDenominator );
  long  xGetFilterLanczos           ( long                  x );
  //--- 6-tap + bilinear upsampling ---
  void  xCompUpsampling6tapBilin    ( ResizeParameters*     pcParameters,
                                      bool                  bChroma );
  void  xUpsamplingData6tapBilin    ( int                   iInLength,
                                      int                   iOutLength );
  //--- dyadic downsampling ---
  void  xCompDownsamplingDyadic     ( int                   iCurrW,
                                      int                   iCurrH );
  //--- SVC non-normative downsampling ---
  void  xCompDownsampling           ( ResizeParameters*     pcParameters,
                                      bool                  bChroma,
                                      bool                  bBotFlag,
                                      bool                  bVerticalDownsampling );
  void  xVertDownsampling           ( int                   iBaseW,
                                      int                   iBaseH,
                                      bool                  bBotFlag );
  void  xBasicDownsampling          ( int  iBaseW,   int  iBaseH,   int  iCurrW,   int  iCurrH,
                                      int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                      int  iShiftX,  int  iShiftY,  int  iScaleX,  int  iScaleY,
                                      int  iAddX,    int  iAddY,    int  iDeltaX,  int  iDeltaY );


private:
  //===== member variables =====
  int         m_iImageStride;
  int*        m_paiImageBuffer;
  int*        m_paiTmp1dBuffer;
  long*       m_padFilter;
  int*        m_aiTmp1dBufferInHalfpel;
  int*        m_aiTmp1dBufferInQ1pel;
  int*        m_aiTmp1dBufferInQ3pel;
  int*        m_paiTmp1dBufferOut;
};



#endif // _DOWN_CONVERT_


