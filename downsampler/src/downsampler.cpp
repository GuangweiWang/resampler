
#include "downsampler.h"
#include "measure_time.h"
#include <stdlib.h>
#include <iostream>

using namespace std;



Downsampler::Downsampler() {
  m_bInitDone		= false;
  m_bCreatDone      = false;
  m_iStartTime		= 0;
  m_iTotalTime		= 0;
  m_iInputFrameNum	= 0;
  m_iOutputFrameNum	= 0;
  m_iInputFrameNum  = 0;
  m_iOutputFrameNum = 0;

  m_eDownsampleMethod = BILINEAR_DOWN_SAMPLER;
  m_eDownsampleMode = SIMPLE;

  //m_pTempBuffer		= NULL;

  memset(&m_sSrcFrame, 0, sizeof(m_sSrcFrame));
  memset(&m_sDstFrame, 0, sizeof(m_sDstFrame));
}

Downsampler::~Downsampler() {
  DestroyDownsampler();
}

void Downsampler::GetOption(SDownsamplerCtx* pDownsamplerCtx) {
  if(pDownsamplerCtx->iDownsampleMethod)
	m_eDownsampleMethod = (EDownsampleMethod)pDownsamplerCtx->iDownsampleMethod;
  if(pDownsamplerCtx->iDownsampleMode)
	m_eDownsampleMode = (EDownsampleMode)pDownsamplerCtx->iDownsampleMode;

  m_iInputFrameNum = pDownsamplerCtx->iInputFrameNum;
  m_iOutputFrameNum = pDownsamplerCtx->iOutputFrameNum;

  //init data pointer
  m_sSrcFrame.pDataY = 
	m_sSrcFrame.pDataU = 
	m_sSrcFrame.pDataV = NULL;
  m_sDstFrame.pDataY = 
	m_sDstFrame.pDataU = 
	m_sDstFrame.pDataV = NULL;

  // set parameters for SrcFrame
  //y
  m_sSrcFrame.iWidthY = pDownsamplerCtx->iSrcWidth;
  m_sSrcFrame.iHeightY = pDownsamplerCtx->iSrcHeight;
  //u
  m_sSrcFrame.iWidthU = m_sSrcFrame.iWidthY >> 1;
  m_sSrcFrame.iHeightU = m_sSrcFrame.iHeightY >> 1;
  //v 
  m_sSrcFrame.iWidthV = m_sSrcFrame.iWidthY >> 1;
  m_sSrcFrame.iHeightV = m_sSrcFrame.iHeightY >> 1;

  // set parameters for DstFrame
  //y
  m_sDstFrame.iWidthY = pDownsamplerCtx->iDstWidth;
  m_sDstFrame.iHeightY = pDownsamplerCtx->iDstHeight;
  //u
  m_sDstFrame.iWidthU = m_sDstFrame.iWidthY >> 1;
  m_sDstFrame.iHeightU = m_sDstFrame.iHeightY >> 1;
  //v
  m_sDstFrame.iWidthV = m_sDstFrame.iWidthY >> 1;
  m_sDstFrame.iHeightV = m_sDstFrame.iHeightY >> 1;

  //for stride;
  m_sSrcFrame.iStrideY = 
	m_sDstFrame.iStrideY = WELS_MAX(m_sSrcFrame.iWidthY, m_sDstFrame.iWidthY);
  m_sSrcFrame.iStrideU = m_sSrcFrame.iStrideV =
	m_sDstFrame.iStrideU = m_sDstFrame.iStrideV = m_sSrcFrame.iStrideY >> 1;
}

void  Downsampler::CreatDownsampler(SDownsamplerCtx* pDownsamplerCtx) {
  if(!m_bCreatDone) {
    
    int32_t iSrcFrameSizeY = (m_sSrcFrame.iStrideY) * (m_sSrcFrame.iHeightY);
    int32_t iDstFrameSizeY = (m_sDstFrame.iStrideY) * (m_sDstFrame.iHeightY);
    
    m_sSrcFrame.pDataY = new uint8_t[iSrcFrameSizeY];
    m_sSrcFrame.pDataU = new uint8_t[iSrcFrameSizeY >> 2];
    m_sSrcFrame.pDataV = new uint8_t[iSrcFrameSizeY >> 2];
    if(m_sSrcFrame.pDataY == NULL ||
       m_sSrcFrame.pDataU == NULL ||
       m_sSrcFrame.pDataV == NULL)
    {
      cout << "Error: malloc srcFarme memory error!" << endl;
      exit(1);
    }
    
    m_sDstFrame.pDataY = new uint8_t[iDstFrameSizeY];
    m_sDstFrame.pDataU = new uint8_t[iDstFrameSizeY >> 2];
    m_sDstFrame.pDataV = new uint8_t[iDstFrameSizeY >> 2];
    if(m_sDstFrame.pDataY == NULL ||
       m_sDstFrame.pDataU == NULL ||
       m_sDstFrame.pDataV == NULL)
    {
      cout << "Error: malloc srcFarme memory error!" << endl;
      exit(1);
    }
    
    //memset
    memset (m_sSrcFrame.pDataY, 0, iSrcFrameSizeY * sizeof(uint8_t));
    memset (m_sSrcFrame.pDataU, 0, (iSrcFrameSizeY >> 2) * sizeof(uint8_t));
    memset (m_sSrcFrame.pDataV, 0, (iSrcFrameSizeY >> 2) * sizeof(uint8_t));
    
    memset (m_sDstFrame.pDataY, 0, iDstFrameSizeY * sizeof(uint8_t));
    memset (m_sDstFrame.pDataU, 0, (iDstFrameSizeY >> 2) * sizeof(uint8_t));
    memset (m_sDstFrame.pDataV, 0, (iDstFrameSizeY >> 2) * sizeof(uint8_t));
    
    m_bCreatDone = true;
    
  }else {
    cout << "Error: can not creat an existed downsampler!" << endl;
    exit(1);
  }

}

void Downsampler::InitDownsampler(SDownsamplerCtx* pDownsamplerCtx) {
  GetOption(pDownsamplerCtx);
  CreatDownsampler(pDownsamplerCtx);
}

void Downsampler::DestroyDownsampler() {
  if(m_bCreatDone) {
	//output Info of Downsampler
	cout << endl << "---------------------------------------" << endl;
    cout << "Total frame:     " << m_iOutputFrameNum << " frame downsampled" << endl;
	cout << "Total time:      " << m_iTotalTime/1e6 << " sec";
	cout << " => " << (m_iTotalTime/m_iOutputFrameNum)/1000.0 << " ms/f"  << endl;
	//reset member variable
	m_bInitDone = false;
	m_iStartTime = 0;
	m_iTotalTime = 0;
	m_iInputFrameNum = 0;
	m_iOutputFrameNum = 0;
    m_iInputFrameNum  = 0;
    m_iOutputFrameNum = 0;
	m_eDownsampleMethod = UNDEFINED_DOWN_SAMPLER;
	m_eDownsampleMode = UNDEFINED;

	//free srcFrame data pointer
	//y
	if(m_sSrcFrame.pDataY != NULL) {
	  delete m_sSrcFrame.pDataY;
	  m_sSrcFrame.pDataY = NULL;
	}
	//u
	if(m_sSrcFrame.pDataU != NULL) {
	  delete m_sSrcFrame.pDataU;
	  m_sSrcFrame.pDataU = NULL;
	}
	//v
	if(m_sSrcFrame.pDataV != NULL) {
	  delete m_sSrcFrame.pDataV;
	  m_sSrcFrame.pDataV = NULL;
	}

	//free DstFrame data pointer
	//y
	if(m_sDstFrame.pDataY != NULL) {
	  delete m_sDstFrame.pDataY;
	  m_sDstFrame.pDataY = NULL;
	}
	//u
	if(m_sDstFrame.pDataU != NULL) {
	  delete m_sDstFrame.pDataU;
	  m_sDstFrame.pDataU = NULL;
	}
	//v
	if(m_sDstFrame.pDataV != NULL) {
	  delete m_sDstFrame.pDataV;
	  m_sDstFrame.pDataV = NULL;
	}

	memset(&m_sSrcFrame, 0, sizeof(m_sSrcFrame));
	memset(&m_sDstFrame, 0, sizeof(m_sDstFrame));

	m_bCreatDone = false;
  }
}

int32_t Downsampler::ReadFrameFromFile(FILE* fin) {
  int32_t sizeY = 0, sizeU = 0, sizeV = 0;
  int i = 0;

  uint8_t* pDataY = m_sSrcFrame.pDataY;
  uint8_t* pDataU = m_sSrcFrame.pDataU;
  uint8_t* pDataV = m_sSrcFrame.pDataV;

  int32_t  iStrideY = m_sSrcFrame.iStrideY;
  int32_t  iStrideU = m_sSrcFrame.iStrideU;
  int32_t  iStrideV = m_sSrcFrame.iStrideV;

  int32_t  iWidthY = m_sSrcFrame.iWidthY;
  int32_t  iWidthU = m_sSrcFrame.iWidthU;
  int32_t  iWidthV = m_sSrcFrame.iWidthV;

  int32_t  iHeightY = m_sSrcFrame.iHeightY;
  int32_t  iHeightU = m_sSrcFrame.iHeightU;
  int32_t  iHeightV = m_sSrcFrame.iHeightV;

  for(i = 0; i < iHeightY; i++) {
	sizeY += fread(pDataY, 1, iWidthY, fin);
	pDataY += iStrideY;
  }
  if(sizeY <= 0) {
	cout << "file is empty!" << endl;
	exit (1);
  }

  for(i = 0; i < iHeightU; i++) {
	sizeU += fread(pDataU, 1, iWidthU, fin);
	pDataU += iStrideU;
  }
  if(sizeU <= 0) {
	cout << "Error: read file error!" << endl;
	exit (1);
  }

  for(i = 0; i < iHeightV; i++) {
	sizeV += fread(pDataV, 1, iWidthV, fin);
	pDataV += iStrideV;
  }
  if(sizeV <= 0) {
	cout << "Error: read file error!" << endl;
	exit (1);
  }

  m_iInputFrameNum++;
  return (sizeY + sizeU + sizeV);
}

int32_t Downsampler::WriteFrameToFile(FILE* fout) {
  cout << m_iOutputFrameNum + 1 << " frames downsampled" << endl;
  int32_t sizeY = 0, sizeU = 0, sizeV = 0;
  int i = 0;

  uint8_t* pDataY = m_sDstFrame.pDataY;
  uint8_t* pDataU = m_sDstFrame.pDataU;
  uint8_t* pDataV = m_sDstFrame.pDataV;

  int32_t  iStrideY = m_sDstFrame.iStrideY;
  int32_t  iStrideU = m_sDstFrame.iStrideU;
  int32_t  iStrideV = m_sDstFrame.iStrideV;

  int32_t  iWidthY = m_sDstFrame.iWidthY;
  int32_t  iWidthU = m_sDstFrame.iWidthU;
  int32_t  iWidthV = m_sDstFrame.iWidthV;

  int32_t  iHeightY = m_sDstFrame.iHeightY;
  int32_t  iHeightU = m_sDstFrame.iHeightU;
  int32_t  iHeightV = m_sDstFrame.iHeightV;

  for(i = 0; i < iHeightY; i++) {
	sizeY += fwrite(pDataY, 1, iWidthY, fout);
	pDataY += iStrideY;
  }
  if(sizeY <= 0) {
	cout << "write frame error(Y)!" << endl;
	exit (1);
  }

  for(i = 0; i < iHeightU; i++) {
	sizeU += fwrite(pDataU, 1, iWidthU, fout);
	pDataU += iStrideU;
  }
  if(sizeU <= 0) {
	cout << "write frame error!(U)" << endl;
	exit (1);
  }

  for(i = 0; i < iHeightV; i++) {
	sizeV += fwrite(pDataV, 1, iWidthV, fout);
	pDataV += iStrideV;
  }
  if(sizeV <= 0) {
	cout << "write frame error!(V)" << endl;
	exit (1);
  }
  
  m_iOutputFrameNum++;
  return (sizeY + sizeU + sizeV);
}

void Downsampler::Processing() {
  cout << "this is father" << endl;
#if 0
  switch(m_eDownsampleMethod) {
	//use bilinear downsampler
	case BILINEAR_DOWN_SAMPLER:
	  cout << "use bilinear downsampler" << endl;
	  if(m_eDownsampleMode == SIMPLE)
		;//bilinearDownsamplerSpecific();
	  else if(m_eDownsampleMode == COMPLEX)
		;//bilinearDownsamplerGeneral();
	  else {
		cout << "Error: undefined downsample mode!" << endl;
		exit(1);
	  }
	  break;

	//use jsvm downsampler
	case JSVM_DOWN_SAMPLER:
	  cout << "use jsvm downsampler" << endl;
	  if(m_eDownsampleMode == SIMPLE)
		;//jsvmDownsamplerDyadic();
	  else if(m_eDownsampleMode == COMPLEX)
		;//jsvmDownsamplerSvc();
	  else {
		cout << "Error: undefined downsample mode!" << endl;
		exit(1);
	  }
	  break;

	  //use my test downsampler
	case TAP_DOWN_SAMPLER:
	  ;//tapDownsampler();
	  break;
	default:
	  cout << "choose the defaulte downsampler(bilinearDownsampler)" << endl;
	  ;//bilinearDownsamplerGeneral();
	  break;
  }
#endif
}

//father class end


//-------------------------------------------
#if 0
void Downsampler::jsvmDownsamplerDyadic() {
  int32_t iterationStage = 0;
  int32_t ratio = m_sSrcFrame.iWidthY / m_sDstFrame.iWidthY;
  int32_t iWidthY = m_sSrcFrame.iWidthY;
  int32_t iHeightY = m_sSrcFrame.iHeightY;

  if(ratio == 2) 
	iterationStage = 1;
  else if(ratio == 4)
	iterationStage = 2;
  else
	iterationStage = 0;

  for(int32_t i = 0; i < iterationStage; i++) {
	m_iStartTime = WelsTime();
	tap13Filter(m_sDstFrame.pDataY, m_sDstFrame.iStrideY, 
		m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY, 
		iWidthY , iHeightY );
	tap13Filter(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
		m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
		iWidthY >> 1, iHeightY >> 1);
	tap13Filter(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
		m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
		iWidthY >> 1, iHeightY >> 1);
	iWidthY >>= 1;
	iHeightY >>= 1;
	m_iTotalTime += WelsTime() - m_iStartTime;
  }
}

void Downsampler::jsvmDownsamplerSvc()
{
  //TODO:support this mode
  cout << "not support yet!" << endl;
  exit(1);
}

void Downsampler::tapDownsampler() {
  int32_t iterationStage = 0;
  int32_t ratio = m_sSrcFrame.iWidthY / m_sDstFrame.iWidthY;
  int32_t iWidthY = m_sSrcFrame.iWidthY;
  int32_t iHeightY = m_sSrcFrame.iHeightY;

  if(ratio == 2) 
	iterationStage = 1;
  else if(ratio == 4)
	iterationStage = 2;
  else
	iterationStage = 0;

  for(int32_t i = 0; i < iterationStage; i++) {
	m_iStartTime = WelsTime();
	tap7Filter(m_sDstFrame.pDataY, m_sDstFrame.iStrideY, 
		m_sSrcFrame.pDataY, m_sSrcFrame.iStrideY, 
		iWidthY , iHeightY );
	tap7Filter(m_sDstFrame.pDataU, m_sDstFrame.iStrideU,
		m_sSrcFrame.pDataU, m_sSrcFrame.iStrideU,
		iWidthY >> 1, iHeightY >> 1);
	tap7Filter(m_sDstFrame.pDataV, m_sDstFrame.iStrideV,
		m_sSrcFrame.pDataV, m_sSrcFrame.iStrideV,
		iWidthY >> 1, iHeightY >> 1);
	iWidthY >>= 1;
	iHeightY >>= 1;
	m_iTotalTime += WelsTime() - m_iStartTime;
  }
}

void Downsampler::tap13Filter(uint8_t* pDstData, int32_t iDstStride,
	uint8_t* pSrcData, int32_t iSrcStride,
	int32_t iSrcWidth, int32_t iSrcHeight) 
{
  int32_t iDstWidth = iSrcWidth >> 1;
  int32_t iDstHeight = iSrcHeight >> 1;
  int32_t iFilterCoeff[13] = {2,0,-4,-3,5,19,26,19,5,-3,-4,0,2};
  int32_t iIndex;
  int32_t i, j, k;
  uint8_t* pSrc = pSrcData;
  uint8_t* pDst = pDstData;
  int32_t* pTempBuffer = m_pTempBuffer;
  int32_t iTempPixel = 0;

  //horizontal downsampling
  for(i = 0; i < iSrcHeight; i++){
	memset(pTempBuffer, 0, iDstWidth * sizeof(int32_t));
	for(j = 0; j < iDstWidth; j++) {
	  
	  for(k=0; k < 13; k++) {
		iIndex = WELS_CLAMP((2*j-6+k), 0, iSrcWidth-1);
		pTempBuffer[j] += iFilterCoeff[k] * pSrc[iIndex];
	  }
	}
	pSrc += iSrcStride;
	pTempBuffer += iDstStride;
  }

  //vertical downsampling
  pTempBuffer = m_pTempBuffer;
  for(i = 0; i < iDstWidth; i++) {
	for(j = 0; j < iDstHeight; j++) {

	  iTempPixel = 0;
	  for(k = 0; k < 13; k++) {
		iIndex = WELS_CLAMP((2*j-6+k), 0, iSrcHeight-1);
		iTempPixel += iFilterCoeff[k] * pTempBuffer[iIndex * iDstStride]; 
		pDst[j * iDstStride] = (uint8_t)WELS_CLAMP((iTempPixel + 2048) >> 12, 0, 255);
	  }
	}
	pTempBuffer += 1;
	pDst += 1;
  }

  //copy result to src
#if 0
  pSrc = pSrcData;
  pDst = pDstData;
  for(i = 0; i < iDstHeight; i++) 
  {
	memcpy(pSrc, pDst, iDstWidth);
	pSrc += iSrcStride;
	pDst += iDstStride;
  }
#endif
}


void Downsampler::tap7Filter(uint8_t* pDstData, int32_t iDstStride,
	uint8_t* pSrcData, int32_t iSrcStride,
	int32_t iSrcWidth, int32_t iSrcHeight) 
{
  int32_t iDstWidth = iSrcWidth >> 1;
  int32_t iDstHeight = iSrcHeight >> 1;
  //int32_t iFilterCoeff[13] = {2,0,-4,-3,5,19,26,19,5,-3,-4,0,2};
  //int32_t iFilterCoeff[13] = {-3, 5,19,26,19,5,-3};
  int32_t iFilterCoeff[13] = {2, -6, 127, 7, -2};
  int32_t iIndex;
  int32_t i, j, k;
  uint8_t* pSrc = pSrcData;
  uint8_t* pDst = pDstData;
  int32_t* pTempBuffer = m_pTempBuffer;
  int32_t iTempPixel = 0;

  //horizontal downsampling
  for(i = 0; i < iSrcHeight; i++){
	memset(pTempBuffer, 0, iDstWidth * sizeof(int32_t));
	for(j = 0; j < iDstWidth; j++) {
	  
	  for(k=0; k < 5; k++) {
		iIndex = WELS_CLAMP((2*j-2+k), 0, iSrcWidth-1);
	  //for(k=0; k < 13; k++) {
		//iIndex = WELS_CLAMP((2*j-6+k), 0, iSrcWidth-1);
		pTempBuffer[j] += iFilterCoeff[k] * pSrc[iIndex];
	  }
	}
	pSrc += iSrcStride;
	pTempBuffer += iDstStride;
  }

  //vertical downsampling
  pTempBuffer = m_pTempBuffer;
  for(i = 0; i < iDstWidth; i++) {
	for(j = 0; j < iDstHeight; j++) {

	  iTempPixel = 0;
	  for(k = 0; k < 7; k++) {
		iIndex = WELS_CLAMP((2*j-2+k), 0, iSrcHeight-1);
	  //for(k = 0; k < 13; k++) {
		//iIndex = WELS_CLAMP((2*j-6+k), 0, iSrcHeight-1);
		iTempPixel += iFilterCoeff[k] * pTempBuffer[iIndex * iDstStride]; 
		pDst[j * iDstStride] = (uint8_t)WELS_CLAMP((iTempPixel + 8192) >> 14, 0, 255);
	  }
	}
	pTempBuffer += 1;
	pDst += 1;
  }
}

void Downsampler::tap6Filter(uint8_t* pDstData, int32_t iDstStride,
	uint8_t* pSrcData, int32_t iSrcStride,
	int32_t iSrcWidth, int32_t iSrcHeight) 
{
  int32_t iDstWidth = iSrcWidth >> 1;
  int32_t iDstHeight = iSrcHeight >> 1;
  int32_t iFilterCoeff[7] = {-3,5,19,26,19,5,-3};
  //int32_t iFilterCoeff[13] = {2,0,-4,-3,5,19,26,19,5,-3,-4,0,2};
  int32_t iIndex;
  int32_t i, j, k;
  uint8_t* pSrc = pSrcData;
  uint8_t* pDst = pDstData;
  int32_t* pTempBuffer = m_pTempBuffer;
  int32_t iTempPixel = 0;

  //horizontal downsampling
  for(i = 0; i < iSrcHeight; i++){
	memset(pTempBuffer, 0, iDstWidth * sizeof(int32_t));
	for(j = 0; j < iDstWidth; j++) {
	  
	  for(k=0; k < 7; k++) {
		iIndex = WELS_CLAMP((2*j-3+k), 0, iSrcWidth-1);
	  //for(k=0; k < 13; k++) {
		//iIndex = WELS_CLAMP((2*j-6+k), 0, iSrcWidth-1);
		pTempBuffer[j] += iFilterCoeff[k] * pSrc[iIndex];
	  }
	}
	pSrc += iSrcStride;
	pTempBuffer += iDstStride;
  }

  //vertical downsampling
  pTempBuffer = m_pTempBuffer;
  for(i = 0; i < iDstWidth; i++) {
	for(j = 0; j < iDstHeight; j++) {

	  iTempPixel = 0;
	  for(k = 0; k < 7; k++) {
		iIndex = WELS_CLAMP((2*j-3+k), 0, iSrcHeight-1);
	  //for(k = 0; k < 13; k++) {
		//iIndex = WELS_CLAMP((2*j-6+k), 0, iSrcHeight-1);
		iTempPixel += iFilterCoeff[k] * pTempBuffer[iIndex * iDstStride]; 
		pDst[j * iDstStride] = (uint8_t)WELS_CLAMP((iTempPixel + 2048) >> 12, 0, 255);
	  }
	}
	pTempBuffer += 1;
	pDst += 1;
  }
}
#endif

#if 0
void Downsampler::testAlgorithm(uint8_t* pDstData, int32_t iDstStride,
	uint8_t* pSrcData, int32_t iSrcStride,
	int32_t iSrcWidth, int32_t iSrcHeight) 
{
}
#endif
