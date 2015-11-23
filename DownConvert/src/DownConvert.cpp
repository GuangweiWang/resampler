
#include "DownConvert.h"
#include <math.h>
#include <string.h>

#define  TMM_TABLE_SIZE          512
#define  TMM_FILTER_WINDOW_SIZE  3
#define  NFACT                   12
#define  VALFACT                 (1l<<NFACT)
#define  MASKFACT                (VALFACT-1)

#ifndef gMax
#define gMax(x,y) ((x)>(y)?(x):(y))
#define gMin(x,y) ((x)<(y)?(x):(y))
#endif

//by guangwei for test
#include <iostream>
using namespace std;


//=================================================
//
//   G E N E R A L   M A I N   F U N C T I O N S
//
//=================================================

DownConvert::DownConvert()
: m_iImageStride            ( 0 )
, m_paiImageBuffer		      ( 0 )
, m_paiTmp1dBuffer		      ( 0 )
, m_padFilter			          ( 0 )
, m_aiTmp1dBufferInHalfpel  ( 0 )
, m_aiTmp1dBufferInQ1pel    ( 0 )
, m_aiTmp1dBufferInQ3pel    ( 0 )
, m_paiTmp1dBufferOut	      ( 0 )
{
}

DownConvert::~DownConvert()
{
  xDestroy();
}

bool
DownConvert::init( int iMaxWidth, int iMaxHeight, int iMaxMargin )
{
  xDestroy();

  iMaxWidth                += 2 * iMaxMargin;
  iMaxHeight               += 2 * iMaxMargin;
  int iPicSize              =   iMaxWidth * iMaxHeight;
  int iMaxDim               = ( iMaxWidth > iMaxHeight ? iMaxWidth : iMaxHeight );
  m_iImageStride            = iMaxWidth;
  m_paiImageBuffer          = new int [ iPicSize ];
  m_paiTmp1dBuffer          = new int [ iMaxDim ];
  m_padFilter               = new long[ TMM_TABLE_SIZE ];
  m_aiTmp1dBufferInHalfpel  = new int [ iMaxDim ];
  m_aiTmp1dBufferInQ1pel    = new int [ iMaxDim ];
  m_aiTmp1dBufferInQ3pel    = new int [ iMaxDim ];
  m_paiTmp1dBufferOut       = new int [ iMaxDim ];

  ROFRS( m_paiImageBuffer,          true );
  ROFRS( m_paiTmp1dBuffer,          true );
  ROFRS( m_padFilter,               true );
  ROFRS( m_aiTmp1dBufferInHalfpel,  true );
  ROFRS( m_aiTmp1dBufferInQ1pel,    true );
  ROFRS( m_aiTmp1dBufferInQ3pel,    true );
  ROFRS( m_paiTmp1dBufferOut,       true );

  xInitLanczosFilter();
  return false;
}

void
DownConvert::destroy()
{
  delete this;
}



//===========================================================================
//
//   M A I N   F U N C T I O N S   F O R   D O W N C O N V E R T   T O O L
//
//===========================================================================

void
DownConvert::cropping( unsigned char*    pucBufferY, int iStrideY,
                       unsigned char*    pucBufferU, int iStrideU,
                       unsigned char*    pucBufferV, int iStrideV,
                       ResizeParameters* pcParameters )
{
  int             iOutWidth   = pcParameters->m_iScaledRefFrmWidth;
  int             iOutHeight  = pcParameters->m_iScaledRefFrmHeight;
  int             iPosX       = pcParameters->m_iLeftFrmOffset;
  int             iPosY       = pcParameters->m_iTopFrmOffset;
  int             iGlobWidth  = pcParameters->m_iFrameWidth;
  int             iGlobHeight = pcParameters->m_iFrameHeight;
  unsigned char   cValue      = 128; // mid gray
  unsigned char*  pCropRegion = 0;

  //===== luma =====
  pCropRegion = &pucBufferY[ iPosY * iStrideY + iPosX ];
  xCopyToImageBuffer  ( pCropRegion, iOutWidth,  iOutHeight,  iStrideY         );
  xInitializeWithValue( pucBufferY,  iGlobWidth, iGlobHeight, iStrideY, cValue );
  xCopyFromImageBuffer( pucBufferY,  iOutWidth,  iOutHeight,  iStrideY         );

  //===== parameters for chroma =====
  iOutWidth   >>= 1;
  iOutHeight  >>= 1;
  iPosX       >>= 1;
  iPosY       >>= 1;
  iGlobWidth  >>= 1;
  iGlobHeight >>= 1;

  //===== chroma cb =====
  pCropRegion = &pucBufferU[ iPosY * iStrideU + iPosX ];
  xCopyToImageBuffer  ( pCropRegion, iOutWidth,  iOutHeight,  iStrideU         );
  xInitializeWithValue( pucBufferU,  iGlobWidth, iGlobHeight, iStrideU, cValue );
  xCopyFromImageBuffer( pucBufferU,  iOutWidth,  iOutHeight,  iStrideU         );

  //===== chroma cr =====
  pCropRegion = &pucBufferV[ iPosY * iStrideV + iPosX ];
  xCopyToImageBuffer  ( pCropRegion, iOutWidth,  iOutHeight,  iStrideV         );
  xInitializeWithValue( pucBufferV,  iGlobWidth, iGlobHeight, iStrideV, cValue );
  xCopyFromImageBuffer( pucBufferV,  iOutWidth,  iOutHeight,  iStrideV         );
}


void
DownConvert::upsamplingDyadic( unsigned char*    pucBufferY,   int iStrideY,
                               unsigned char*    pucBufferU,   int iStrideU,
                               unsigned char*    pucBufferV,   int iStrideV,
                               ResizeParameters* pcParameters )
{
  int iWidth  = pcParameters->m_iRefLayerFrmWidth;
  int iHeight = pcParameters->m_iRefLayerFrmHeight;
  int iRatio  = pcParameters->m_iFrameWidth / pcParameters->m_iRefLayerFrmWidth;
  int iStages = 0;
  while( iRatio > 1 )
  {
    iStages++;
    iRatio >>= 1;
  }

  for( int i = iStages; i > 0; i-- )
  {
    //===== luma =====
    xCopyToImageBuffer    ( pucBufferY, iWidth,      iHeight,      iStrideY );
    xCompUpsamplingDyadic (             iWidth,      iHeight,      false    );
    xCopyFromImageBuffer  ( pucBufferY, iWidth << 1, iHeight << 1, iStrideY );
  	//===== chroma cb =====
    xCopyToImageBuffer    ( pucBufferU, iWidth >> 1, iHeight >> 1, iStrideU );
    xCompUpsamplingDyadic (             iWidth >> 1, iHeight >> 1, true     );
    xCopyFromImageBuffer  ( pucBufferU, iWidth,      iHeight,      iStrideU );
    //===== chroma cb =====
    xCopyToImageBuffer    ( pucBufferV, iWidth >> 1, iHeight >> 1, iStrideV );
    xCompUpsamplingDyadic (             iWidth >> 1, iHeight >> 1, true     );
    xCopyFromImageBuffer  ( pucBufferV, iWidth,      iHeight,      iStrideV );
    iWidth  <<= 1;
    iHeight <<= 1;
  }
}


void
DownConvert::upsamplingLanczos( unsigned char*    pucBufferY,   int iStrideY,
                                unsigned char*    pucBufferU,   int iStrideU,
                                unsigned char*    pucBufferV,   int iStrideV,
                                ResizeParameters* pcParameters )
{
  int iInWidth    = pcParameters->m_iRefLayerFrmWidth;
  int iInHeight   = pcParameters->m_iRefLayerFrmHeight;
  int iGlobWidth  = pcParameters->m_iFrameWidth;
  int iGlobHeight = pcParameters->m_iFrameHeight;

  //===== luma =====
  xCopyToImageBuffer    ( pucBufferY,   iInWidth,   iInHeight,   iStrideY );
  xCompUpsamplingLanczos( pcParameters, false );
  xCopyFromImageBuffer  ( pucBufferY,   iGlobWidth, iGlobHeight, iStrideY );
  //===== parameters for chroma =====
  iInWidth    >>= 1;
  iInHeight   >>= 1;
  iGlobWidth  >>= 1;
  iGlobHeight >>= 1;
  //===== chroma cb =====
  xCopyToImageBuffer    ( pucBufferU,   iInWidth,   iInHeight,   iStrideU );
  xCompUpsamplingLanczos( pcParameters, true );
  xCopyFromImageBuffer  ( pucBufferU,   iGlobWidth, iGlobHeight, iStrideU );
  //===== chroma cr =====
  xCopyToImageBuffer    ( pucBufferV,   iInWidth,   iInHeight,   iStrideV );
  xCompUpsamplingLanczos( pcParameters, true );
  xCopyFromImageBuffer  ( pucBufferV,   iGlobWidth, iGlobHeight, iStrideV );
}


void
DownConvert::upsampling6tapBilin( unsigned char*    pucBufferY,   int iStrideY,
                                  unsigned char*    pucBufferU,   int iStrideU,
                                  unsigned char*    pucBufferV,   int iStrideV,
                                  ResizeParameters* pcParameters )
{
  int iInWidth    = pcParameters->m_iRefLayerFrmWidth;
  int iInHeight   = pcParameters->m_iRefLayerFrmHeight;
  int iGlobWidth  = pcParameters->m_iFrameWidth;
  int iGlobHeight = pcParameters->m_iFrameHeight;

  //===== luma =====
  xCopyToImageBuffer      ( pucBufferY,   iInWidth,   iInHeight,   iStrideY );
  xCompUpsampling6tapBilin( pcParameters, false );
  xCopyFromImageBuffer    ( pucBufferY,   iGlobWidth, iGlobHeight, iStrideY );
  //===== parameters for chroma =====
  iInWidth    >>= 1;
  iInHeight   >>= 1;
  iGlobWidth  >>= 1;
  iGlobHeight >>= 1;
  //===== chroma cb =====
  xCopyToImageBuffer      ( pucBufferU,   iInWidth,   iInHeight,   iStrideU );
  xCompUpsampling6tapBilin( pcParameters, true );
  xCopyFromImageBuffer    ( pucBufferU,   iGlobWidth, iGlobHeight, iStrideU );
  //===== chroma cr =====
  xCopyToImageBuffer      ( pucBufferV,   iInWidth,   iInHeight,   iStrideV );
  xCompUpsampling6tapBilin( pcParameters, true );
  xCopyFromImageBuffer    ( pucBufferV,   iGlobWidth, iGlobHeight, iStrideV );
}


void
DownConvert::upsamplingSVC( unsigned char*    pucBufferY,   int   iStrideY,
                            unsigned char*    pucBufferU,   int   iStrideU,
                            unsigned char*    pucBufferV,   int   iStrideV,
                            ResizeParameters* pcParameters, bool  bBotCoincided )
{
  int   iBaseW                  =   pcParameters->m_iRefLayerFrmWidth;
  int   iBaseH                  =   pcParameters->m_iRefLayerFrmHeight;
  int   iCurrW                  =   pcParameters->m_iFrameWidth;
  int   iCurrH                  =   pcParameters->m_iFrameHeight;
  bool  bTopAndBottomResampling = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == false  &&
                                    pcParameters->m_bRefLayerFieldPicFlag     == false  &&
                                    pcParameters->m_bFrameMbsOnlyFlag         == false  &&
                                    pcParameters->m_bFieldPicFlag             == false    );
  bool  bFrameBasedResampling   = ( pcParameters->m_bFrameMbsOnlyFlag         == true   &&
                                    pcParameters->m_bRefLayerFrameMbsOnlyFlag == true     );
  bool  bVerticalInterpolation  = ( bFrameBasedResampling                     == false  &&
                                    pcParameters->m_bFieldPicFlag             == false    );
  bool  bCurrBotField           = ( pcParameters->m_bFieldPicFlag             == true   &&
                                    pcParameters->m_bBotFieldFlag             == true     );
  bool  bBotFieldFlag           = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  false
                                  : pcParameters->m_bFieldPicFlag             ?  pcParameters->m_bBotFieldFlag
                                  : pcParameters->m_bRefLayerFieldPicFlag     ?  pcParameters->m_bRefLayerBotFieldFlag
                                  : false );
  int   iBaseField              = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  0 : 1 );
  int   iCurrField              = ( pcParameters->m_bFieldPicFlag             ?  1 : 0 );
  int   iBaseBot                = ( bBotFieldFlag ? 1 : 0 );
  int   iCurrBot                = ( bCurrBotField ? 1 : 0 );

  //==== check bot field coincided parameter for progressive to interlaced resampling =====
  if( pcParameters->m_bRefLayerFrameMbsOnlyFlag && ! pcParameters->m_bFrameMbsOnlyFlag )
  {
    bBotFieldFlag = bBotCoincided;
  }

  //=======================
  //=====   L U M A   =====
  //=======================
  if( bTopAndBottomResampling )
  {
    //===== top field =====
    unsigned char* pFld = pucBufferY;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );
    xCompIntraUpsampling( pcParameters, false,  false,       false         );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );

    //===== bottom field =====
    pFld += iStrideY;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );
    xCompIntraUpsampling( pcParameters, false,  true,        false         );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );
  }
  else
  {
    unsigned char* pSrc = pucBufferY + iStrideY * iBaseBot;
    unsigned char* pDes = pucBufferY + iStrideY * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iStrideY << iBaseField );
    xCompIntraUpsampling( pcParameters, false,  bBotFieldFlag,        bVerticalInterpolation );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iStrideY << iCurrField );
  }

  iBaseW >>= 1;
  iBaseH >>= 1;
  iCurrW >>= 1;
  iCurrH >>= 1;

  //===========================
  //=====   C H R O M A   =====
  //===========================
  if( bTopAndBottomResampling )
  {
    //===== top field (U) =====
    unsigned char* pFld = pucBufferU;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );
    xCompIntraUpsampling( pcParameters, true,   false,       false         );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );

    //===== bottom field (U) =====
    pFld += iStrideU;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );
    xCompIntraUpsampling( pcParameters, true,   true,        false         );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );

    //===== top field (V) =====
    pFld = pucBufferV;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );
    xCompIntraUpsampling( pcParameters, true,   false,       false         );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );

    //===== bottom field (V) =====
    pFld += iStrideV;
    xCopyToImageBuffer  ( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );
    xCompIntraUpsampling( pcParameters, true,   true,        false         );
    xCopyFromImageBuffer( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );
  }
  else
  {
    //===== U =====
    unsigned char* pSrc = pucBufferU + iStrideU * iBaseBot;
    unsigned char* pDes = pucBufferU + iStrideU * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iStrideU << iBaseField );
    xCompIntraUpsampling( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iStrideU << iCurrField );

    //===== V =====
    pSrc = pucBufferV + iStrideV * iBaseBot;
    pDes = pucBufferV + iStrideV * iCurrBot;
    xCopyToImageBuffer  ( pSrc,         iBaseW, iBaseH >> iBaseField, iStrideV << iBaseField );
    xCompIntraUpsampling( pcParameters, true,   bBotFieldFlag,        bVerticalInterpolation );
    xCopyFromImageBuffer( pDes,         iCurrW, iCurrH >> iCurrField, iStrideV << iCurrField );
  }
}


void
DownConvert::downsamplingDyadic( unsigned char*    pucBufferY,   int iStrideY,
                                 unsigned char*    pucBufferU,   int iStrideU,
                                 unsigned char*    pucBufferV,   int iStrideV,
                                 ResizeParameters* pcParameters )
{
  int iWidth  = pcParameters->m_iRefLayerFrmWidth;
  int iHeight = pcParameters->m_iRefLayerFrmHeight;
  int iRatio  = pcParameters->m_iRefLayerFrmWidth / pcParameters->m_iFrameWidth;
  int iStages = 0;
  while( iRatio > 1 )
  {
    iStages++;
    iRatio >>= 1;
  }

  for( int i = iStages; i > 0; i-- )
  {
    //===== luma =====
    xCopyToImageBuffer      ( pucBufferY, iWidth,      iHeight,      iStrideY );
    xCompDownsamplingDyadic (             iWidth,      iHeight                );
    xCopyFromImageBuffer    ( pucBufferY, iWidth >> 1, iHeight >> 1, iStrideY );
    //===== chroma cb =====
    xCopyToImageBuffer      ( pucBufferU, iWidth >> 1, iHeight >> 1, iStrideU );
    xCompDownsamplingDyadic (             iWidth >> 1, iHeight >> 1           );
    xCopyFromImageBuffer    ( pucBufferU, iWidth >> 2, iHeight >> 2, iStrideU );
    //===== chroma cr =====
    xCopyToImageBuffer      ( pucBufferV, iWidth >> 1, iHeight >> 1, iStrideV );
    xCompDownsamplingDyadic (             iWidth >> 1, iHeight >> 1           );
    xCopyFromImageBuffer    ( pucBufferV, iWidth >> 2, iHeight >> 2, iStrideV );
    iWidth  >>= 1;
    iHeight >>= 1;
  }
}


void
DownConvert::downsamplingSVC( unsigned char*    pucBufferY,   int   iStrideY,
                              unsigned char*    pucBufferU,   int   iStrideU,
                              unsigned char*    pucBufferV,   int   iStrideV,
                              ResizeParameters* pcParameters, bool  bBotCoincided )
{
  int   iBaseW                  =   pcParameters->m_iFrameWidth;
  int   iBaseH                  =   pcParameters->m_iFrameHeight;
  int   iCurrW                  =   pcParameters->m_iRefLayerFrmWidth;
  int   iCurrH                  =   pcParameters->m_iRefLayerFrmHeight;
  bool  bTopAndBottomResampling = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag == false  &&
                                    pcParameters->m_bRefLayerFieldPicFlag     == false  &&
                                    pcParameters->m_bFrameMbsOnlyFlag         == false  &&
                                    pcParameters->m_bFieldPicFlag             == false    );
  bool  bVerticalDownsampling   = ( pcParameters->m_bFrameMbsOnlyFlag         == true   &&
                                    pcParameters->m_bRefLayerFieldPicFlag     == true     );
  bool  bCurrBotField           = ( pcParameters->m_bFieldPicFlag             == true   &&
                                    pcParameters->m_bBotFieldFlag             == true     );
  bool  bBotFieldFlag           = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  false
                                  : pcParameters->m_bFieldPicFlag             ?  pcParameters->m_bBotFieldFlag
                                  : pcParameters->m_bRefLayerFieldPicFlag     ?  pcParameters->m_bRefLayerBotFieldFlag
                                  : false );
  int   iBaseField              = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ?  0 : 1 );
  int   iCurrField              = ( pcParameters->m_bFieldPicFlag             ?  1 : 0 );
  int   iBaseBot                = ( bBotFieldFlag ? 1 : 0 );
  int   iCurrBot                = ( bCurrBotField ? 1 : 0 );


  //==== check bot field coincided parameter for interlaced to progressive resampling =====
  if( pcParameters->m_bRefLayerFrameMbsOnlyFlag && ! pcParameters->m_bFrameMbsOnlyFlag )
  {
    bBotFieldFlag = bBotCoincided;
  }

  //=======================
  //=====   L U M A   =====
  //=======================
  if( bTopAndBottomResampling )
  {
    //===== top field =====
    unsigned char* pFld = pucBufferY;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );
    xCompDownsampling   ( pcParameters, false,  false,       false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );

    //===== bottom field =====
    pFld += iStrideY;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideY << 1 );
    xCompDownsampling   ( pcParameters, false,  true,        false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideY << 1 );
  }
  else
  {

    unsigned char* pSrc = pucBufferY + iStrideY * iCurrBot;
    unsigned char* pDes = pucBufferY + iStrideY * iBaseBot;
    xCopyToImageBuffer  ( pSrc,         iCurrW, iCurrH >> iCurrField, iStrideY << iCurrField );
    xCompDownsampling   ( pcParameters, false,  bBotFieldFlag,        bVerticalDownsampling  );
    xCopyFromImageBuffer( pDes,         iBaseW, iBaseH >> iBaseField, iStrideY << iBaseField );
  }

  iBaseW >>= 1;
  iBaseH >>= 1;
  iCurrW >>= 1;
  iCurrH >>= 1;

  //===========================
  //=====   C H R O M A   =====
  //===========================
  if( bTopAndBottomResampling )
  {
    //===== top field (U) =====
    unsigned char* pFld = pucBufferU;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );
    xCompDownsampling   ( pcParameters, true,   false,       false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );

    //===== bottom field (U) =====
    pFld += iStrideU;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideU << 1 );
    xCompDownsampling   ( pcParameters, true,   true,        false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideU << 1 );

    //===== top field (V) =====
    pFld = pucBufferV;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );
    xCompDownsampling   ( pcParameters, true,   false,       false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );

    //===== bottom field (V) =====
    pFld += iStrideV;
    xCopyToImageBuffer  ( pFld,         iCurrW, iCurrH >> 1, iStrideV << 1 );
    xCompDownsampling   ( pcParameters, true,   true,        false         );
    xCopyFromImageBuffer( pFld,         iBaseW, iBaseH >> 1, iStrideV << 1 );
  }
  else
  {
    //===== U =====
    unsigned char* pSrc = pucBufferU + iStrideU * iCurrBot;
    unsigned char* pDes = pucBufferU + iStrideU * iBaseBot;
    xCopyToImageBuffer  ( pSrc,         iCurrW, iCurrH >> iCurrField, iStrideU << iCurrField );
    xCompDownsampling   ( pcParameters, true,   bBotFieldFlag,        bVerticalDownsampling  );
    xCopyFromImageBuffer( pDes,         iBaseW, iBaseH >> iBaseField, iStrideU << iBaseField );

    //===== V =====
    pSrc = pucBufferV + iStrideV * iCurrBot;
    pDes = pucBufferV + iStrideV * iBaseBot;
    xCopyToImageBuffer  ( pSrc,         iCurrW, iCurrH >> iCurrField, iStrideV << iCurrField );
    xCompDownsampling   ( pcParameters, true,   bBotFieldFlag,        bVerticalDownsampling  );
    xCopyFromImageBuffer( pDes,         iBaseW, iBaseH >> iBaseField, iStrideV << iBaseField );
  }
}




//======================================================
//
//   G E N E R A L   H E L P E R    F U N C T I O N S
//
//======================================================

void
DownConvert::xDestroy()
{
  delete [] m_paiImageBuffer;
  delete [] m_paiTmp1dBuffer;
  delete [] m_padFilter;
  delete [] m_aiTmp1dBufferInHalfpel;
  delete [] m_aiTmp1dBufferInQ1pel;
  delete [] m_aiTmp1dBufferInQ3pel;
  delete [] m_paiTmp1dBufferOut;
  m_paiImageBuffer          = 0;
  m_paiTmp1dBuffer          = 0;
  m_padFilter               = 0;
  m_aiTmp1dBufferInHalfpel  = 0;
  m_aiTmp1dBufferInQ1pel    = 0;
  m_aiTmp1dBufferInQ3pel    = 0;
  m_paiTmp1dBufferOut       = 0;
}

int
DownConvert::xClip( int iValue, int imin, int imax )
{
  ROTRS( iValue < imin, imin );
  ROTRS( iValue > imax, imax );
  return iValue;
}


void
DownConvert::xCompIntraUpsampling( ResizeParameters* pcParameters, bool bChroma, bool bBotFlag, bool bVerticalInterpolation, int iMargin )
{
  //===== set general parameters =====
  int   iBotField   = ( bBotFlag ? 1 : 0 );
  int   iFactor     = ( !bChroma ? 1 : 2 );
  int   iRefPhaseX  = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseX );
  int   iRefPhaseY  = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseY );
  int   iPhaseX     = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseX );
  int   iPhaseY     = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseY );
  int   iRefW       = pcParameters->m_iRefLayerFrmWidth   / iFactor;  // reference layer frame width
  int   iRefH       = pcParameters->m_iRefLayerFrmHeight  / iFactor;  // reference layer frame height
  int   iOutW       = pcParameters->m_iScaledRefFrmWidth  / iFactor;  // scaled reference layer frame width
  int   iOutH       = pcParameters->m_iScaledRefFrmHeight / iFactor;  // scaled reference layer frame height
  int   iGlobalW    = pcParameters->m_iFrameWidth         / iFactor;  // current frame width
  int   iGlobalH    = pcParameters->m_iFrameHeight        / iFactor;  // current frame height
  int   iLeftOffset = pcParameters->m_iLeftFrmOffset      / iFactor;  // current left frame offset
  int   iTopOffset  = pcParameters->m_iTopFrmOffset       / iFactor;  // current top  frame offset

  //===== set input/output size =====
  int   iBaseField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ? 0 : 1 );
  int   iCurrField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag && pcParameters->m_bFrameMbsOnlyFlag ? 0 : 1 );
  int   iBaseW      = iRefW;
  int   iBaseH      = iRefH      >> iBaseField;
  int   iCurrW      = iGlobalW;
  int   iCurrH      = iGlobalH   >> iCurrField;
  int   iLOffset    = iLeftOffset;
  int   iTOffset    = iTopOffset >> iCurrField;
  int   iROffset    = iCurrW - iLOffset -   iOutW;
  int   iBOffset    = iCurrH - iTOffset - ( iOutH >> iCurrField );
  int   iYBorder    = ( bVerticalInterpolation ? ( bChroma ? 1 : 2 ) : 0 );

  //===== set position calculation parameters =====
  int   iScaledW    = iOutW;
  int   iScaledH    = ( ! pcParameters->m_bRefLayerFrameMbsOnlyFlag || pcParameters->m_bFrameMbsOnlyFlag ? iOutH : iOutH / 2 );
  int   iShiftX     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefW ) );
  int   iShiftY     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iRefH ) );
  int   iScaleX     = ( ( (unsigned int)iRefW << iShiftX ) + ( iScaledW >> 1 ) ) / iScaledW;
  int   iScaleY     = ( ( (unsigned int)iRefH << iShiftY ) + ( iScaledH >> 1 ) ) / iScaledH;
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    if( pcParameters->m_bRefLayerFrameMbsOnlyFlag )
    {
      iPhaseY       = iPhaseY + 4 * iBotField + ( 3 - iFactor );
      iRefPhaseY    = 2 * iRefPhaseY + 2;
    }
    else
    {
      iPhaseY       = iPhaseY    + 4 * iBotField;
      iRefPhaseY    = iRefPhaseY + 4 * iBotField;
    }
  }
  Int   iOffsetX    = iLeftOffset;
  Int   iOffsetY    = iTopOffset;
  Int   iAddX       = ( ( ( iRefW * ( 2 + iPhaseX ) ) << ( iShiftX - 2 ) ) + ( iScaledW >> 1 ) ) / iScaledW + ( 1 << ( iShiftX - 5 ) );
  Int   iAddY       = ( ( ( iRefH * ( 2 + iPhaseY ) ) << ( iShiftY - 2 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
  Int   iDeltaX     = 4 * ( 2 + iRefPhaseX );
  Int   iDeltaY     = 4 * ( 2 + iRefPhaseY );
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    iOffsetY        = iTopOffset / 2;
    iAddY           = ( ( ( iRefH * ( 2 + iPhaseY ) ) << ( iShiftY - 3 ) ) + ( iScaledH >> 1 ) ) / iScaledH + ( 1 << ( iShiftY - 5 ) );
    iDeltaY         = 2 * ( 2 + iRefPhaseY );
  }

  //===== basic interpolation of a frame or a field =====
  xBasicIntraUpsampling ( iBaseW,   iBaseH,   iCurrW,   iCurrH,
                          iLOffset, iTOffset, iROffset, iBOffset,
                          iShiftX,  iShiftY,  iScaleX,  iScaleY,
                          iOffsetX, iOffsetY, iAddX,    iAddY,
                          iDeltaX,  iDeltaY,  iYBorder, bChroma, iMargin );

  //===== vertical interpolation for second field =====
  if( bVerticalInterpolation )
  {
    xVertIntraUpsampling( iCurrW,   iCurrH,
                          iLOffset, iTOffset, iROffset, iBOffset,
                          iYBorder, bBotFlag, bChroma );
  }
}


void
DownConvert::xVertIntraUpsampling( int  iBaseW,   int  iBaseH,
                                   int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                   int  iYBorder, bool bBotFlag, bool bChromaFilter )
{
  AOT( !bChromaFilter && iYBorder < 2 );
  AOT(  bChromaFilter && iYBorder < 1 );

  int iBotField     = ( bBotFlag ? 1 : 0 );
  int iCurrW        = iBaseW;
  int iCurrH        = iBaseH   << 1;
  int iCurrLOffset  = iLOffset;
  int iCurrTOffset  = iTOffset << 1;
  int iCurrROffset  = iROffset;
  int iCurrBOffset  = iBOffset << 1;

  //========== vertical upsampling ===========
  for( int j = 0; j < iCurrW; j++ )
  {
    int* piSrc = &m_paiImageBuffer[j];

    //----- upsample column -----
    for( int i = 0; i < iCurrH; i++ )
    {
      if( j < iCurrLOffset || j >= iCurrW - iCurrROffset ||
          i < iCurrTOffset || i >= iCurrH - iCurrBOffset   )
      {
        m_paiTmp1dBuffer[i] = 128; // set to mid gray
        continue;
      }

      if( ( i % 2 ) == iBotField )
      {
        int iSrc = ( ( i >> 1 ) + iYBorder ) * m_iImageStride;
        m_paiTmp1dBuffer[i] = piSrc[ iSrc ];
      }
      else
      {
        int iSrc = ( ( i >> 1 ) + iYBorder - iBotField ) * m_iImageStride;
        if( bChromaFilter )
        {
          m_paiTmp1dBuffer[i]   = ( piSrc[ iSrc ] + piSrc[ iSrc + m_iImageStride ] + 1 ) >> 1;
        }
        else
        {
          m_paiTmp1dBuffer[i]   = 16;
          m_paiTmp1dBuffer[i]  += 19 * ( piSrc[ iSrc                  ] + piSrc[ iSrc +     m_iImageStride ] );
          m_paiTmp1dBuffer[i]  -=  3 * ( piSrc[ iSrc - m_iImageStride ] + piSrc[ iSrc + 2 * m_iImageStride ] );
          m_paiTmp1dBuffer[i] >>=  5;
          m_paiTmp1dBuffer[i]   = xClip( m_paiTmp1dBuffer[i], 0, 255 );
        }
      }
    }
    //----- copy back to image buffer -----
    for( int n = 0; n < iCurrH; n++ )
    {
      piSrc[n*m_iImageStride] = m_paiTmp1dBuffer[n];
    }
  }
}


void
DownConvert::xBasicIntraUpsampling( int  iBaseW,   int  iBaseH,   int  iCurrW,   int  iCurrH,
                                    int  iLOffset, int  iTOffset, int  iROffset, int  iBOffset,
                                    int  iShiftX,  int  iShiftY,  int  iScaleX,  int  iScaleY,
                                    int  iOffsetX, int  iOffsetY, int  iAddX,    int  iAddY,
                                    int  iDeltaX,  int  iDeltaY,  int  iYBorder, bool bChromaFilter, int iMargin )
{
  assert( iMargin >= 0 );

  int filter16_luma[16][4] =
  {
    {  0, 32,  0,  0 },
    { -1, 32,  2, -1 },
    { -2, 31,  4, -1 },
    { -3, 30,  6, -1 },
    { -3, 28,  8, -1 },
    { -4, 26, 11, -1 },
    { -4, 24, 14, -2 },
    { -3, 22, 16, -3 },
    { -3, 19, 19, -3 },
    { -3, 16, 22, -3 },
    { -2, 14, 24, -4 },
    { -1, 11, 26, -4 },
    { -1,  8, 28, -3 },
    { -1,  6, 30, -3 },
    { -1,  4, 31, -2 },
    { -1,  2, 32, -1 }
  };
  int filter16_chroma[16][2] =
  {
    { 32,  0 },
    { 30,  2 },
    { 28,  4 },
    { 26,  6 },
    { 24,  8 },
    { 22, 10 },
    { 20, 12 },
    { 18, 14 },
    { 16, 16 },
    { 14, 18 },
    { 12, 20 },
    { 10, 22 },
    {  8, 24 },
    {  6, 26 },
    {  4, 28 },
    {  2, 30 }
  };

  int iShiftXM4 = iShiftX - 4;
  int iShiftYM4 = iShiftY - 4;

  //========== horizontal upsampling ===========
  {
    for( int j = 0; j < iBaseH + 2 * iMargin; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      for( int i = 0; i < iCurrW; i++ )
      {
        if( i < iLOffset || i >= iCurrW - iROffset )
        {
          m_paiTmp1dBuffer[i] = 128; // set to mid gray
          continue;
        }

        m_paiTmp1dBuffer[i] = 0;

        int iRefPos16 = (int)( (unsigned int)( ( i - iOffsetX ) * iScaleX + iAddX ) >> iShiftXM4 ) - iDeltaX;
        int iPhase    = iRefPos16 & 15;
        int iRefPos   = iRefPos16 >> 4;

        if( bChromaFilter )
        {
          for( int k = 0; k < 2; k++ )
          {
            int m = xClip( iRefPos + k, -iMargin, iBaseW - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[i] += filter16_chroma[iPhase][k] * piSrc[m];
          }
        }
        else
        {
          for( int k = 0; k < 4; k++ )
          {
            int m = xClip( iRefPos + k - 1, -iMargin, iBaseW - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[i] += filter16_luma[iPhase][k] * piSrc[m];
          }
        }
      }
      //----- copy row back to image buffer -----
      memcpy( piSrc, m_paiTmp1dBuffer, iCurrW*sizeof(int) );
    }
  }

  //========== vertical upsampling ===========
  {
    for( int i = 0; i < iCurrW; i++ )
    {
      int* piSrc = &m_paiImageBuffer[i];
      for( int j = -iYBorder; j < iCurrH+iYBorder; j++ )
      {
        if( i < iLOffset            || i >= iCurrW - iROffset           ||
            j < iTOffset - iYBorder || j >= iCurrH - iBOffset + iYBorder  )
        {
          m_paiTmp1dBuffer[j+iYBorder] = 128; // set to mid gray
          continue;
        }

        m_paiTmp1dBuffer[j+iYBorder] = 0;

        int iPreShift   = ( j - iOffsetY ) * iScaleY + iAddY;
        int iPostShift  = ( j >= iOffsetY ? (int)( (unsigned int)iPreShift >> iShiftYM4 ) : ( iPreShift >> iShiftYM4 ) );
        int iRefPos16   = iPostShift - iDeltaY;
        int iPhase      = iRefPos16 & 15;
        int iRefPos     = iRefPos16 >> 4;

        if( bChromaFilter )
        {
          for( int k = 0; k < 2; k++ )
          {
            int m = xClip( iRefPos + k, -iMargin, iBaseH - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[j+iYBorder] += filter16_chroma[iPhase][k] * piSrc[m*m_iImageStride];
          }
        }
        else
        {
          for( int k = 0; k < 4; k++ )
          {
            int m = xClip( iRefPos + k - 1, -iMargin, iBaseH - 1 + iMargin ) + iMargin;
            m_paiTmp1dBuffer[j+iYBorder] += filter16_luma[iPhase][k] * piSrc[m*m_iImageStride];
          }
        }
        m_paiTmp1dBuffer[j+iYBorder] = ( m_paiTmp1dBuffer[j+iYBorder] + 512 ) >> 10;
      }
      //----- clip and copy back to image buffer -----
      for( int n = 0; n < iCurrH+2*iYBorder; n++ )
      {
        piSrc[n*m_iImageStride] = xClip( m_paiTmp1dBuffer[n], 0, 255 );
      }
    }
  }
}


//===============================================================================
//
//   H E L P E R   F U N C T I O N S   F O R   D O W N C O N V E R T   T O O L
//
//===============================================================================

void
DownConvert::xInitLanczosFilter()
{
  const double pi = 3.14159265359;
  m_padFilter[0]  = VALFACT;
  for( int i = 1; i < TMM_TABLE_SIZE; i++ )
  {
    double  x       = ( (double)i / TMM_TABLE_SIZE ) * TMM_FILTER_WINDOW_SIZE;
    double  pix     = pi * x;
    double  pixw    = pix / TMM_FILTER_WINDOW_SIZE;
    m_padFilter[i]  = (long)( sin( pix ) / pix * sin( pixw ) / pixw * VALFACT );
  }
}

void
DownConvert::xCopyToImageBuffer( unsigned char* pucSrc, int iWidth, int iHeight, int iStride )
{
  int* piDes = m_paiImageBuffer;
  for( int j = 0; j < iHeight; j++ )
  {
    for( int i = 0; i < iWidth;  i++ )
    {
      piDes[i] = (int)pucSrc[i];
    }
    piDes   += m_iImageStride;
    pucSrc  += iStride;
  }
}

void
DownConvert::xCopyFromImageBuffer( unsigned char* pucDes, int iWidth, int iHeight, int iStride )
{
  int* piSrc = m_paiImageBuffer;
  for( int j = 0; j < iHeight; j++ )
  {
    for( int i = 0; i < iWidth;  i++ )
    {
      pucDes[i] = (unsigned char)piSrc[i];
    }
    pucDes  += iStride;
    piSrc   += m_iImageStride;
  }
}

void
DownConvert::xInitializeWithValue( unsigned char* pucBuffer, int iWidth, int iHeight, int iStride, unsigned char cValue )
{
  for( int y = 0; y < iHeight; y++, pucBuffer += iStride )
  {
    ::memset( pucBuffer, cValue, iWidth );
  }
}

void
DownConvert::xCompUpsamplingDyadic( int iBaseW, int iBaseH, bool bChroma )
{
  int aiLumaFilter[6] = { 1, -5, 20, 20, -5, 1 };

  //========== vertical upsampling ===========
  {
    for( int j = 0; j < iBaseW; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j];
      //----- upsample column -----
      for( int i = 0; i < iBaseH; i++)
      {
        m_paiTmp1dBuffer[2*i  ] = piSrc[i*m_iImageStride] << 5;
        m_paiTmp1dBuffer[2*i+1] = 0;

        if( bChroma )
        {
          int m1 = i;
          int m2 = gMin( i + 1, iBaseH - 1 );
          m_paiTmp1dBuffer[2*i+1] += piSrc[m1*m_iImageStride] << 4;
          m_paiTmp1dBuffer[2*i+1] += piSrc[m2*m_iImageStride] << 4;
        }
        else
        {
          for( int k = 0; k < 6; k++ )
          {
            int m = xClip( i + k - 2, 0, iBaseH - 1 );
            m_paiTmp1dBuffer[2*i+1] += aiLumaFilter[k] * piSrc[m*m_iImageStride];
          }
        }
      }
      //----- copy back to image buffer -----
      for( int n = 0; n < 2*iBaseH; n++ )
      {
        piSrc[n*m_iImageStride] = m_paiTmp1dBuffer[n];
      }
    }
  }

  //========== horizontal upsampling ==========
  {
    for( int j = 0; j < 2*iBaseH; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      //----- upsample row -----
      for( int i = 0; i < iBaseW; i++)
      {
        m_paiTmp1dBuffer[2*i  ] = piSrc[i] << 5;
        m_paiTmp1dBuffer[2*i+1] = 0;

        if( bChroma )
        {
          int m1 = i;
          int m2 = gMin( i + 1, iBaseW - 1 );
          m_paiTmp1dBuffer[2*i+1] += piSrc[m1] << 4;
          m_paiTmp1dBuffer[2*i+1] += piSrc[m2] << 4;
        }
        else
        {
          for( int k = 0; k < 6; k++ )
          {
            int m = xClip( i + k - 2, 0, iBaseW - 1 );
            m_paiTmp1dBuffer[2*i+1] += aiLumaFilter[k] * piSrc[m];
          }
        }
      }
      //----- round, clip, and copy back to image buffer -----
      for( int n = 0; n < 2*iBaseW; n++ )
      {
        int iS    = ( m_paiTmp1dBuffer[n] + 512 ) >> 10;
        piSrc[n]  = xClip( iS, 0, 255 );
      }
    }
  }
}

void
DownConvert::xCompUpsamplingLanczos( ResizeParameters* pcParameters, bool bChroma )

{
  int   iShift        = ( bChroma ? 1 : 0 );
  int   iInWidth      = pcParameters->m_iRefLayerFrmWidth    >> iShift;
  int   iInHeight     = pcParameters->m_iRefLayerFrmHeight   >> iShift;
  int   iOutWidth     = pcParameters->m_iScaledRefFrmWidth   >> iShift;
  int   iOutHeight    = pcParameters->m_iScaledRefFrmHeight  >> iShift;
  int   iNumerator    = 1;
  int   iDenominator  = 1;
  long  spos          = 0;

  //===== vertical upsampling =====
  xGetNumDenomLanczos( iInHeight, iOutHeight, iNumerator, iDenominator );
  spos = ( 1 << NFACT ) * iDenominator / iNumerator;
  for( int xin = 0; xin < iInWidth; xin++ )
  {
    int* piSrc = &m_paiImageBuffer[xin];
    for( int yin = 0; yin < iInHeight; yin++ )
    {
      m_paiTmp1dBuffer[yin] = piSrc[yin * m_iImageStride];
    }
    xUpsamplingDataLanczos( iInHeight, iOutHeight, spos );
    for( int yout = 0; yout < iOutHeight; yout++ )
    {
      piSrc[yout*m_iImageStride] = m_paiTmp1dBufferOut[yout];
    }
  }

  //===== horizontal upsampling =====
  xGetNumDenomLanczos( iInWidth, iOutWidth, iNumerator, iDenominator );
  spos = ( 1 << NFACT ) * iDenominator / iNumerator;
  for( int yout = 0; yout < iOutHeight; yout++ )
  {
    int* piSrc = &m_paiImageBuffer[yout * m_iImageStride];
    for( int xin = 0; xin < iInWidth; xin++ )
    {
      m_paiTmp1dBuffer[xin] = piSrc[xin];
    }
    xUpsamplingDataLanczos( iInWidth, iOutWidth, spos );
    memcpy( piSrc, m_paiTmp1dBufferOut, iOutWidth*sizeof(int) );
  }
}

void
DownConvert::xUpsamplingDataLanczos( int iInLength, int iOutLength, long spos )
{
  long dpos0 = -spos;
  for( int iout = 0; iout < iOutLength; iout++ )
  {
    dpos0      += spos;
    long  rpos0 = dpos0  & MASKFACT;
    int   ipos0 = dpos0 >> NFACT;
    if(   rpos0 == 0 )
    {
      m_paiTmp1dBufferOut[iout] = m_paiTmp1dBuffer[ipos0];
      continue;
    }
    int   end   = ipos0 + TMM_FILTER_WINDOW_SIZE;
    int   begin = end   - TMM_FILTER_WINDOW_SIZE * 2;
    long  sval  = 0;
    long  posi  = ( ( begin - ipos0 ) << NFACT ) - rpos0;
    for( int i = begin; i <= end; i++, posi += VALFACT )
    {
      long  fact  = xGetFilterLanczos( posi );
      int   m     = xClip( i, 0, iInLength - 1 );
      int   val   = m_paiTmp1dBuffer[m];
      sval       += val * fact;
    }
    m_paiTmp1dBufferOut[iout] = xClip( sval >> NFACT, 0, 255 );
  }
}

void
DownConvert::xGetNumDenomLanczos( int iInWidth, int iOutWidth, int& riNumerator, int& riDenominator )
{
  int iA = 1;
  int iB = iOutWidth;
  int iC = iInWidth;
  while (iC != 0)
  {
    iA = iB;
    iB = iC;
    iC = iA % iB;
  }
  riNumerator   = iOutWidth / iB;
  riDenominator = iInWidth  / iB;
}

long
DownConvert::xGetFilterLanczos( long x )
{
  x      = ( x < 0 ? -x : x );
  int i  = (int)( ( x / TMM_FILTER_WINDOW_SIZE ) * TMM_TABLE_SIZE ) >> NFACT;
  if( i >= TMM_TABLE_SIZE )
  {
    return 0;
  }
  return m_padFilter[ i ];
}

void
DownConvert::xCompUpsampling6tapBilin( ResizeParameters* pcParameters, bool bChroma )
{
  int iShift      = ( bChroma ? 1 : 0 );
  int iInWidth    = pcParameters->m_iRefLayerFrmWidth    >> iShift;
  int iInHeight   = pcParameters->m_iRefLayerFrmHeight   >> iShift;
  int iOutWidth   = pcParameters->m_iScaledRefFrmWidth   >> iShift;
  int iOutHeight  = pcParameters->m_iScaledRefFrmHeight  >> iShift;

  //===== vertical upsampling =====
  for( int xin = 0; xin < iInWidth; xin++ )
  {
    int* piSrc = &m_paiImageBuffer[xin];
    for( int yin = 0; yin < iInHeight; yin++ )
    {
      m_paiTmp1dBuffer[yin] = piSrc[yin * m_iImageStride];
    }
    xUpsamplingData6tapBilin( iInHeight, iOutHeight );
    for( int yout = 0; yout < iOutHeight; yout++ )
    {
      piSrc[yout*m_iImageStride] = m_paiTmp1dBufferOut[yout];
    }
  }

  // ===== horizontal upsampling =====
  for( int yout = 0; yout < iOutHeight; yout++ )
  {
    int* piSrc = &m_paiImageBuffer[yout * m_iImageStride];
    for( int xin = 0; xin < iInWidth; xin++ )
    {
      m_paiTmp1dBuffer[xin] = piSrc[xin];
    }
    xUpsamplingData6tapBilin( iInWidth, iOutWidth );
    for( int i = 0; i < iOutWidth; i++ )
    {
      piSrc[i] = xClip( ( m_paiTmp1dBufferOut[i] + 512 ) >> 10, 0, 255 );
    }
  }
}

void
DownConvert::xUpsamplingData6tapBilin( int iInLength, int iOutLength )
{
  int*  Tmp1dBufferInHalfpel  = m_aiTmp1dBufferInHalfpel;
  int*  Tmp1dBufferInQ1pel    = m_aiTmp1dBufferInQ1pel;
  int*  Tmp1dBufferInQ3pel    = m_aiTmp1dBufferInQ3pel;
  int   x, y, iTemp;

  // half pel samples (6-tap: 1 -5 20 20 -5 1)
  for(  x = 0; x < iInLength; x++ )
  {
    y      = x;
    iTemp  = m_paiTmp1dBuffer[y];
    y      = ( x+1 < iInLength ? x+1 : iInLength-1 );
    iTemp += m_paiTmp1dBuffer[y];
    iTemp  = iTemp << 2;
    y      = ( x-1 >= 0 ? x-1 : 0 );
    iTemp -= m_paiTmp1dBuffer[y];
    y      = ( x+2 < iInLength ? x+2 : iInLength-1 );
    iTemp -= m_paiTmp1dBuffer[y];
    iTemp += iTemp << 2;
    y      = ( x-2 >= 0 ? x-2 : 0);
    iTemp += m_paiTmp1dBuffer[y];
    y      = ( x+3 < iInLength ? x+3 : iInLength-1 );
    iTemp += m_paiTmp1dBuffer[y];
    Tmp1dBufferInHalfpel[x] = iTemp;
  }

  // 1/4 pel samples
  for( x = 0; x < iInLength-1; x++ )
  {
    Tmp1dBufferInQ1pel[x] = ( ( m_paiTmp1dBuffer[x  ] << 5 ) + Tmp1dBufferInHalfpel[x] + 1 ) >> 1;
    Tmp1dBufferInQ3pel[x] = ( ( m_paiTmp1dBuffer[x+1] << 5 ) + Tmp1dBufferInHalfpel[x] + 1 ) >> 1;
  }
  Tmp1dBufferInQ1pel[iInLength-1] = ( ( m_paiTmp1dBuffer[iInLength-1] << 5 ) + Tmp1dBufferInHalfpel[iInLength-1] + 1 ) >> 1;
  Tmp1dBufferInQ3pel[iInLength-1] = Tmp1dBufferInHalfpel[iInLength-1];

  // generic interpolation to nearest 1/4 pel position
  for( int iout = 0; iout < iOutLength; iout++ )
  {
    double  dpos0   = ( (double)iout * iInLength / iOutLength );
    int     ipos0   = (int)dpos0;
    double  rpos0   = dpos0 - ipos0;
    int     iIndex  = (int)( 8 * rpos0 );
    switch( iIndex )
    {
    case 0:
      m_paiTmp1dBufferOut[iout] =  m_paiTmp1dBuffer     [ipos0] << 5; // original pel value
      break;
    case 1:
    case 2:
      m_paiTmp1dBufferOut[iout] =  Tmp1dBufferInQ1pel   [ipos0];      // 1/4 pel value
      break;
    case 3:
    case 4:
      m_paiTmp1dBufferOut[iout] =  Tmp1dBufferInHalfpel [ipos0];      // half pel value
      break;
    case 5:
    case 6:
      m_paiTmp1dBufferOut[iout] =  Tmp1dBufferInQ3pel   [ipos0];      // 1/4 pel value
      break;
    case 7:
      int ipos1                 = ( ipos0 + 1 < iInLength ? ipos0 + 1 : ipos0 );
      m_paiTmp1dBufferOut[iout] =  m_paiTmp1dBuffer     [ipos1] << 5; // original pel value
      break;
    }
  }
}

void
DownConvert::xCompDownsamplingDyadic( int iCurrW, int iCurrH )
{
  int iBaseW        = iCurrW >> 1;
  int iBaseH        = iCurrH >> 1;
  int aiFilter[13]  = { 2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2 };

  //========== horizontal downsampling ===========
  {
    for( int j = 0; j < iCurrH; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      //----- down sample row -----
      for( int i = 0; i < iBaseW; i++ )
      {
        m_paiTmp1dBuffer[i] = 0;
        for( int k = 0; k < 13; k++ )
        {
          int m = xClip( 2*i + k - 6, 0, iCurrW - 1 );
          m_paiTmp1dBuffer[i] += aiFilter[k] * piSrc[m];
        }
      }
      //----- copy row back to image buffer -----
      memcpy( piSrc, m_paiTmp1dBuffer, iBaseW*sizeof(int) );
    }
  }

  //=========== vertical downsampling ===========
  {
    for( int j = 0; j < iBaseW; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j];
      //----- down sample column -----
      for( int i = 0; i < iBaseH; i++ )
      {
        m_paiTmp1dBuffer[i] = 0;
        for( int k = 0; k < 13; k++ )
        {
          int m = xClip( 2*i + k - 6, 0, iCurrH - 1 );
          m_paiTmp1dBuffer[i] += aiFilter[k] * piSrc[m*m_iImageStride];
        }
      }
      //----- scale, clip, and copy back to image buffer -----
      for( int n = 0; n < iBaseH; n++ )
      {
        int iS                  = ( m_paiTmp1dBuffer[n] + 2048 ) >> 12;
        piSrc[n*m_iImageStride] = xClip( iS, 0, 255 );
      }
    }
  }
}

void
DownConvert::xCompDownsampling( ResizeParameters* pcParameters, bool bChroma, bool bBotFlag, bool bVerticalDownsampling )
{
  //===== set general parameters =====
  int   iBotField   = ( bBotFlag ? 1 : 0 );
  int   iFactor     = ( !bChroma ? 1 : 2 );
  int   iRefPhaseX  = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseX );
  int   iRefPhaseY  = ( !bChroma ? 0 : pcParameters->m_iChromaPhaseY );
  int   iPhaseX     = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseX );
  int   iPhaseY     = ( !bChroma ? 0 : pcParameters->m_iRefLayerChromaPhaseY );
  int   iRefW       = pcParameters->m_iFrameWidth         / iFactor;  // reference layer frame width
  int   iRefH       = pcParameters->m_iFrameHeight        / iFactor;  // reference layer frame height
  int   iOutW       = pcParameters->m_iScaledRefFrmWidth  / iFactor;  // scaled reference layer frame width
  int   iOutH       = pcParameters->m_iScaledRefFrmHeight / iFactor;  // scaled reference layer frame height
  int   iGlobalW    = pcParameters->m_iRefLayerFrmWidth   / iFactor;  // current frame width
  int   iGlobalH    = pcParameters->m_iRefLayerFrmHeight  / iFactor;  // current frame height
  int   iLeftOffset = pcParameters->m_iLeftFrmOffset      / iFactor;  // current left frame offset
  int   iTopOffset  = pcParameters->m_iTopFrmOffset       / iFactor;  // current  top frame offset

  //===== set input/output size =====
  int   iBaseField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag ? 0 : 1 );
  int   iCurrField  = ( pcParameters->m_bRefLayerFrameMbsOnlyFlag && pcParameters->m_bFrameMbsOnlyFlag ? 0 : 1 );
  int   iBaseW      = iRefW;
  int   iBaseH      = iRefH       >> iBaseField;
  int   iCurrW      = iGlobalW;
  int   iCurrH      = iGlobalH    >> iCurrField;
  int   iLOffset    = iLeftOffset;
  int   iTOffset    = iTopOffset  >> iCurrField;
  int   iROffset    = iCurrW - iLOffset -   iOutW;
  int   iBOffset    = iCurrH - iTOffset - ( iOutH >> iCurrField );

  //===== set position calculation parameters =====
  int   iScaledW    = iOutW;
  int   iScaledH    = ( ! pcParameters->m_bRefLayerFrameMbsOnlyFlag || pcParameters->m_bFrameMbsOnlyFlag ? iOutH : iOutH / 2 );
  int   iShiftX     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iScaledW ) );
  int   iShiftY     = ( pcParameters->m_iLevelIdc <= 30 ? 16 : 31 - CeilLog2( iScaledH ) );
  int   iScaleX     = ( ( (unsigned int)iScaledW << iShiftX ) + ( iRefW >> 1 ) ) / iRefW;
  int   iScaleY     = ( ( (unsigned int)iScaledH << iShiftY ) + ( iRefH >> 1 ) ) / iRefH;
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    if( pcParameters->m_bRefLayerFrameMbsOnlyFlag )
    {
      iPhaseY       = iPhaseY + 4 * iBotField + ( 3 - iFactor );
      iRefPhaseY    = 2 * iRefPhaseY + 2;
    }
    else
    {
      iPhaseY       = iPhaseY    + 4 * iBotField;
      iRefPhaseY    = iRefPhaseY + 4 * iBotField;
    }
  }
  int   iAddX       = ( ( ( iScaledW * ( 2 + iRefPhaseX ) ) << ( iShiftX - 2 ) ) + ( iRefW >> 1 ) ) / iRefW + ( 1 << ( iShiftX - 5 ) );
  int   iAddY       = ( ( ( iScaledH * ( 2 + iRefPhaseY ) ) << ( iShiftY - 2 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
  int   iDeltaX     = 4 * ( 2 + iPhaseX ) - ( iLeftOffset << 4 );
  int   iDeltaY     = 4 * ( 2 + iPhaseY ) - ( iTopOffset  << 4 );
  if( ! pcParameters->m_bFrameMbsOnlyFlag || ! pcParameters->m_bRefLayerFrameMbsOnlyFlag )
  {
    iAddY           = ( ( ( iScaledH * ( 2 + iRefPhaseY ) ) << ( iShiftY - 3 ) ) + ( iRefH >> 1 ) ) / iRefH + ( 1 << ( iShiftY - 5 ) );
    iDeltaY         = 2 * ( 2 + iPhaseY ) - ( iTopOffset  << 3 );
  }

  //===== vertical downsampling to generate a field signal from a progressive frame =====
  if( bVerticalDownsampling )
  {
    xVertDownsampling( iCurrW, iCurrH, bBotFlag );
  }

  //===== basic downsampling of a frame or field =====
  xBasicDownsampling( iBaseW,   iBaseH,   iCurrW,   iCurrH,
                      iLOffset, iTOffset, iROffset, iBOffset,
                      iShiftX,  iShiftY,  iScaleX,  iScaleY,
                      iAddX,    iAddY,    iDeltaX,  iDeltaY );
}

void
DownConvert::xVertDownsampling( int   iBaseW,
                                int   iBaseH,
                                bool  bBotFlag )
{
  int aiVertFilter[13]  = { 2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2 };
  int iBotField         = ( bBotFlag ? 1 : 0 );
  int iCurrW            = iBaseW;
  int iCurrH            = iBaseH << 1;

  //===== vertical downsampling =====
  for( int j = 0; j < iCurrW; j++ )
  {
    int* piSrc = &m_paiImageBuffer[j];
    for( int i = 0; i < iBaseH; i++ )
    {
      m_paiTmp1dBuffer[i] = 0;
      for( int k = 0; k < 13; k++ )
      {
        int m = xClip( 2 * i + iBotField + k - 6, 0, iCurrH - 1 );
        m_paiTmp1dBuffer[i] += aiVertFilter[k] * piSrc[m*m_iImageStride];
      }
      m_paiTmp1dBuffer[i] = ( m_paiTmp1dBuffer[i] + 32 ) >> 6;
    }
    //--- clip and copy back to image buffer ---
    for( int n = 0; n < iBaseH; n++ )
    {
      piSrc[n*m_iImageStride] = xClip( m_paiTmp1dBuffer[n], 0, 255 );
    }
  }
}

void
DownConvert::xBasicDownsampling( int iBaseW,   int iBaseH,   int iCurrW,   int iCurrH,
                                 int iLOffset, int iTOffset, int iROffset, int iBOffset,
                                 int iShiftX,  int iShiftY,  int iScaleX,  int iScaleY,
                                 int iAddX,    int iAddY,    int iDeltaX,  int iDeltaY )
{
  const int filter16[8][16][12] =
  {
    { // D = 1
      {   0,   0,   0,   0,   0, 128,   0,   0,   0,   0,   0,   0 },
      {   0,   0,   0,   2,  -6, 127,   7,  -2,   0,   0,   0,   0 },
      {   0,   0,   0,   3, -12, 125,  16,  -5,   1,   0,   0,   0 },
      {   0,   0,   0,   4, -16, 120,  26,  -7,   1,   0,   0,   0 },
      {   0,   0,   0,   5, -18, 114,  36, -10,   1,   0,   0,   0 },
      {   0,   0,   0,   5, -20, 107,  46, -12,   2,   0,   0,   0 },
      {   0,   0,   0,   5, -21,  99,  57, -15,   3,   0,   0,   0 },
      {   0,   0,   0,   5, -20,  89,  68, -18,   4,   0,   0,   0 },
      {   0,   0,   0,   4, -19,  79,  79, -19,   4,   0,   0,   0 },
      {   0,   0,   0,   4, -18,  68,  89, -20,   5,   0,   0,   0 },
      {   0,   0,   0,   3, -15,  57,  99, -21,   5,   0,   0,   0 },
      {   0,   0,   0,   2, -12,  46, 107, -20,   5,   0,   0,   0 },
      {   0,   0,   0,   1, -10,  36, 114, -18,   5,   0,   0,   0 },
      {   0,   0,   0,   1,  -7,  26, 120, -16,   4,   0,   0,   0 },
      {   0,   0,   0,   1,  -5,  16, 125, -12,   3,   0,   0,   0 },
      {   0,   0,   0,   0,  -2,   7, 127,  -6,   2,   0,   0,   0 }
    },
    { // D = 1.5
      {   0,   2,   0, -14,  33,  86,  33, -14,   0,   2,   0,   0 },
      {   0,   1,   1, -14,  29,  85,  38, -13,  -1,   2,   0,   0 },
      {   0,   1,   2, -14,  24,  84,  43, -12,  -2,   2,   0,   0 },
      {   0,   1,   2, -13,  19,  83,  48, -11,  -3,   2,   0,   0 },
      {   0,   0,   3, -13,  15,  81,  53, -10,  -4,   3,   0,   0 },
      {   0,   0,   3, -12,  11,  79,  57,  -8,  -5,   3,   0,   0 },
      {   0,   0,   3, -11,   7,  76,  62,  -5,  -7,   3,   0,   0 },
      {   0,   0,   3, -10,   3,  73,  65,  -2,  -7,   3,   0,   0 },
      {   0,   0,   3,  -9,   0,  70,  70,   0,  -9,   3,   0,   0 },
      {   0,   0,   3,  -7,  -2,  65,  73,   3, -10,   3,   0,   0 },
      {   0,   0,   3,  -7,  -5,  62,  76,   7, -11,   3,   0,   0 },
      {   0,   0,   3,  -5,  -8,  57,  79,  11, -12,   3,   0,   0 },
      {   0,   0,   3,  -4, -10,  53,  81,  15, -13,   3,   0,   0 },
      {   0,   0,   2,  -3, -11,  48,  83,  19, -13,   2,   1,   0 },
      {   0,   0,   2,  -2, -12,  43,  84,  24, -14,   2,   1,   0 },
      {   0,   0,   2,  -1, -13,  38,  85,  29, -14,   1,   1,   0 }
    },
    { // D = 2
      {   2,   0, -10,   0,  40,  64,  40,   0, -10,   0,   2,   0 },
      {   2,   1,  -9,  -2,  37,  64,  42,   2, -10,  -1,   2,   0 },
      {   2,   1,  -9,  -3,  34,  64,  44,   4, -10,  -1,   2,   0 },
      {   2,   1,  -8,  -5,  31,  63,  47,   6, -10,  -2,   3,   0 },
      {   1,   2,  -8,  -6,  29,  62,  49,   8, -10,  -2,   3,   0 },
      {   1,   2,  -7,  -7,  26,  61,  52,  10, -10,  -3,   3,   0 },
      {   1,   2,  -6,  -8,  23,  60,  54,  13, -10,  -4,   3,   0 },
      {   1,   2,  -6,  -9,  20,  59,  56,  15, -10,  -4,   3,   1 },
      {   1,   2,  -5,  -9,  18,  57,  57,  18,  -9,  -5,   2,   1 },
      {   1,   3,  -4, -10,  15,  56,  59,  20,  -9,  -6,   2,   1 },
      {   0,   3,  -4, -10,  13,  54,  60,  23,  -8,  -6,   2,   1 },
      {   0,   3,  -3, -10,  10,  52,  61,  26,  -7,  -7,   2,   1 },
      {   0,   3,  -2, -10,   8,  49,  62,  29,  -6,  -8,   2,   1 },
      {   0,   3,  -2, -10,   6,  47,  63,  31,  -5,  -8,   1,   2 },
      {   0,   2,  -1, -10,   4,  44,  64,  34,  -3,  -9,   1,   2 },
      {   0,   2,  -1, -10,   2,  42,  64,  37,  -2,  -9,   1,   2 }
    },
    { // D = 2.5
      {   0,  -4,  -7,  11,  38,  52,  38,  11,  -7,  -4,   0,   0 },
      {   0,  -4,  -7,   9,  37,  51,  40,  13,  -6,  -7,   2,   0 },
      {   0,  -3,  -7,   8,  35,  51,  41,  14,  -5,  -7,   1,   0 },
      {   0,  -2,  -8,   6,  33,  51,  42,  16,  -5,  -7,   2,   0 },
      {   0,  -2,  -8,   5,  32,  50,  43,  18,  -4,  -8,   2,   0 },
      {   0,  -2,  -8,   4,  30,  50,  45,  19,  -3,  -8,   1,   0 },
      {   0,  -1,  -8,   2,  28,  49,  46,  21,  -2,  -8,   1,   0 },
      {   0,  -1,  -8,   1,  26,  49,  47,  23,  -1,  -8,   0,   0 },
      {   0,   0,  -8,   0,  24,  48,  48,  24,   0,  -8,   0,   0 },
      {   0,   0,  -8,  -1,  23,  47,  49,  26,   1,  -8,  -1,   0 },
      {   0,   1,  -8,  -2,  21,  46,  49,  28,   2,  -8,  -1,   0 },
      {   0,   1,  -8,  -3,  19,  45,  50,  30,   4,  -8,  -2,   0 },
      {   0,   2,  -8,  -4,  18,  43,  50,  32,   5,  -8,  -2,   0 },
      {   0,   2,  -7,  -5,  16,  42,  51,  33,   6,  -8,  -2,   0 },
      {   0,   1,  -7,  -5,  14,  41,  51,  35,   8,  -7,  -3,   0 },
      {   0,   2,  -7,  -6,  13,  40,  51,  37,   9,  -7,  -4,   0 }
    },
    { // D = 3
      {  -2,  -7,   0,  17,  35,  43,  35,  17,   0,  -7,  -5,   2 },
      {  -2,  -7,  -1,  16,  34,  43,  36,  18,   1,  -7,  -5,   2 },
      {  -1,  -7,  -1,  14,  33,  43,  36,  19,   1,  -6,  -5,   2 },
      {  -1,  -7,  -2,  13,  32,  42,  37,  20,   3,  -6,  -5,   2 },
      {   0,  -7,  -3,  12,  31,  42,  38,  21,   3,  -6,  -5,   2 },
      {   0,  -7,  -3,  11,  30,  42,  39,  23,   4,  -6,  -6,   1 },
      {   0,  -7,  -4,  10,  29,  42,  40,  24,   5,  -6,  -6,   1 },
      {   1,  -7,  -4,   9,  27,  41,  40,  25,   6,  -5,  -6,   1 },
      {   1,  -6,  -5,   7,  26,  41,  41,  26,   7,  -5,  -6,   1 },
      {   1,  -6,  -5,   6,  25,  40,  41,  27,   9,  -4,  -7,   1 },
      {   1,  -6,  -6,   5,  24,  40,  42,  29,  10,  -4,  -7,   0 },
      {   1,  -6,  -6,   4,  23,  39,  42,  30,  11,  -3,  -7,   0 },
      {   2,  -5,  -6,   3,  21,  38,  42,  31,  12,  -3,  -7,   0 },
      {   2,  -5,  -6,   3,  20,  37,  42,  32,  13,  -2,  -7,  -1 },
      {   2,  -5,  -6,   1,  19,  36,  43,  33,  14,  -1,  -7,  -1 },
      {   2,  -5,  -7,   1,  18,  36,  43,  34,  16,  -1,  -7,  -2 }
    },
    { // D = 3.5
      {  -6,  -3,   5,  19,  31,  36,  31,  19,   5,  -3,  -6,   0 },
      {  -6,  -4,   4,  18,  31,  37,  32,  20,   6,  -3,  -6,  -1 },
      {  -6,  -4,   4,  17,  30,  36,  33,  21,   7,  -3,  -6,  -1 },
      {  -5,  -5,   3,  16,  30,  36,  33,  22,   8,  -2,  -6,  -2 },
      {  -5,  -5,   2,  15,  29,  36,  34,  23,   9,  -2,  -6,  -2 },
      {  -5,  -5,   2,  15,  28,  36,  34,  24,  10,  -2,  -6,  -3 },
      {  -4,  -5,   1,  14,  27,  36,  35,  24,  10,  -1,  -6,  -3 },
      {  -4,  -5,   0,  13,  26,  35,  35,  25,  11,   0,  -5,  -3 },
      {  -4,  -6,   0,  12,  26,  36,  36,  26,  12,   0,  -6,  -4 },
      {  -3,  -5,   0,  11,  25,  35,  35,  26,  13,   0,  -5,  -4 },
      {  -3,  -6,  -1,  10,  24,  35,  36,  27,  14,   1,  -5,  -4 },
      {  -3,  -6,  -2,  10,  24,  34,  36,  28,  15,   2,  -5,  -5 },
      {  -2,  -6,  -2,   9,  23,  34,  36,  29,  15,   2,  -5,  -5 },
      {  -2,  -6,  -2,   8,  22,  33,  36,  30,  16,   3,  -5,  -5 },
      {  -1,  -6,  -3,   7,  21,  33,  36,  30,  17,   4,  -4,  -6 },
      {  -1,  -6,  -3,   6,  20,  32,  37,  31,  18,   4,  -4,  -6 }
    },
    { // D = 4
      {  -9,   0,   9,  20,  28,  32,  28,  20,   9,   0,  -9,   0 },
      {  -9,   0,   8,  19,  28,  32,  29,  20,  10,   0,  -4,  -5 },
      {  -9,  -1,   8,  18,  28,  32,  29,  21,  10,   1,  -4,  -5 },
      {  -9,  -1,   7,  18,  27,  32,  30,  22,  11,   1,  -4,  -6 },
      {  -8,  -2,   6,  17,  27,  32,  30,  22,  12,   2,  -4,  -6 },
      {  -8,  -2,   6,  16,  26,  32,  31,  23,  12,   2,  -4,  -6 },
      {  -8,  -2,   5,  16,  26,  31,  31,  23,  13,   3,  -3,  -7 },
      {  -8,  -3,   5,  15,  25,  31,  31,  24,  14,   4,  -3,  -7 },
      {  -7,  -3,   4,  14,  25,  31,  31,  25,  14,   4,  -3,  -7 },
      {  -7,  -3,   4,  14,  24,  31,  31,  25,  15,   5,  -3,  -8 },
      {  -7,  -3,   3,  13,  23,  31,  31,  26,  16,   5,  -2,  -8 },
      {  -6,  -4,   2,  12,  23,  31,  32,  26,  16,   6,  -2,  -8 },
      {  -6,  -4,   2,  12,  22,  30,  32,  27,  17,   6,  -2,  -8 },
      {  -6,  -4,   1,  11,  22,  30,  32,  27,  18,   7,  -1,  -9 },
      {  -5,  -4,   1,  10,  21,  29,  32,  28,  18,   8,  -1,  -9 },
      {  -5,  -4,   0,  10,  20,  29,  32,  28,  19,   8,   0,  -9 }
    },
    { // D = 5.5
      {  -8,   7,  13,  18,  22,  24,  22,  18,  13,   7,   2, -10 },
      {  -8,   7,  13,  18,  22,  23,  22,  19,  13,   7,   2, -10 },
      {  -8,   6,  12,  18,  22,  23,  22,  19,  14,   8,   2, -10 },
      {  -9,   6,  12,  17,  22,  23,  23,  19,  14,   8,   3, -10 },
      {  -9,   6,  12,  17,  21,  23,  23,  19,  14,   9,   3, -10 },
      {  -9,   5,  11,  17,  21,  23,  23,  20,  15,   9,   3, -10 },
      {  -9,   5,  11,  16,  21,  23,  23,  20,  15,   9,   4, -10 },
      {  -9,   5,  10,  16,  21,  23,  23,  20,  15,  10,   4, -10 },
      { -10,   5,  10,  16,  20,  23,  23,  20,  16,  10,   5, -10 },
      { -10,   4,  10,  15,  20,  23,  23,  21,  16,  10,   5,  -9 },
      { -10,   4,   9,  15,  20,  23,  23,  21,  16,  11,   5,  -9 },
      { -10,   3,   9,  15,  20,  23,  23,  21,  17,  11,   5,  -9 },
      { -10,   3,   9,  14,  19,  23,  23,  21,  17,  12,   6,  -9 },
      { -10,   3,   8,  14,  19,  23,  23,  22,  17,  12,   6,  -9 },
      { -10,   2,   8,  14,  19,  22,  23,  22,  18,  12,   6,  -8 },
      { -10,   2,   7,  13,  19,  22,  23,  22,  18,  13,   7,  -8 }
    }
  };

  //===== determine filter sets =====
  int iCropW      = iCurrW - iLOffset - iROffset;
  int iCropH      = iCurrH - iTOffset - iBOffset;
  int iVerFilter  = 0;
  int iHorFilter  = 0;
  if      (  4 * iCropH > 15 * iBaseH )   iVerFilter  = 7;
  else if (  7 * iCropH > 20 * iBaseH )   iVerFilter  = 6;
  else if (  2 * iCropH >  5 * iBaseH )   iVerFilter  = 5;
  else if (  1 * iCropH >  2 * iBaseH )   iVerFilter  = 4;
  else if (  3 * iCropH >  5 * iBaseH )   iVerFilter  = 3;
  else if (  4 * iCropH >  5 * iBaseH )   iVerFilter  = 2;
  else if ( 19 * iCropH > 20 * iBaseH )   iVerFilter  = 1;
  if      (  4 * iCropW > 15 * iBaseW )   iHorFilter  = 7;
  else if (  7 * iCropW > 20 * iBaseW )   iHorFilter  = 6;
  else if (  2 * iCropW >  5 * iBaseW )   iHorFilter  = 5;
  else if (  1 * iCropW >  2 * iBaseW )   iHorFilter  = 4;
  else if (  3 * iCropW >  5 * iBaseW )   iHorFilter  = 3;
  else if (  4 * iCropW >  5 * iBaseW )   iHorFilter  = 2;
  else if ( 19 * iCropW > 20 * iBaseW )   iHorFilter  = 1;

  int iShiftXM4 = iShiftX - 4;
  int iShiftYM4 = iShiftY - 4;

  //===== horizontal downsampling =====
  {
    for( int j = 0; j < iCurrH; j++ )
    {
      int* piSrc = &m_paiImageBuffer[j*m_iImageStride];
      for( int i = 0; i < iBaseW; i++ )
      {
        int iRefPos16 = (int)( (unsigned int)( i * iScaleX + iAddX ) >> iShiftXM4 ) - iDeltaX;
        int iPhase    = iRefPos16  & 15;
        int iRefPos   = iRefPos16 >>  4;

        m_paiTmp1dBuffer[i] = 0;
        for( int k = 0; k < 12; k++ )
        {
          int m = xClip( iRefPos + k - 5, 0, iCurrW - 1 );
          m_paiTmp1dBuffer[i] += filter16[iHorFilter][iPhase][k] * piSrc[m];
        }
      }
      //--- copy row back to image buffer ---
      memcpy( piSrc, m_paiTmp1dBuffer, iBaseW*sizeof(int) );
    }
  }

  //===== vertical downsampling =====
  {
    for( int i = 0; i < iBaseW; i++ )
    {
      int* piSrc = &m_paiImageBuffer[i];
      for( int j = 0; j < iBaseH; j++ )
      {
        int iRefPos16 = (int)( (unsigned int)( j * iScaleY + iAddY ) >> iShiftYM4 ) - iDeltaY;
        int iPhase    = iRefPos16  & 15;
        int iRefPos   = iRefPos16 >>  4;

        m_paiTmp1dBuffer[j] = 0;
        for( int k = 0; k < 12; k++ )
        {
          int m = xClip( iRefPos + k - 5, 0, iCurrH - 1 );
          m_paiTmp1dBuffer[j] += filter16[iVerFilter][iPhase][k] * piSrc[m*m_iImageStride];
        }
        m_paiTmp1dBuffer[j] = ( m_paiTmp1dBuffer[j] + 8192 ) >> 14;
      }
      //--- clip and copy back to image buffer ---
      for( int n = 0; n < iBaseH; n++ )
      {
        piSrc[n*m_iImageStride] = xClip( m_paiTmp1dBuffer[n], 0, 255 );
      }
    }
  }
}



