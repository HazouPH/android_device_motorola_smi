/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2007-2012 Intel Corporation. All Rights Reserved.
//
//                  Intel(R) Performance Primitives
//                  Data Integrity Primitives (ippDI)
//
*/

#if !defined( __IPPDI_H__ ) || defined( _OWN_BLDPCS )
#define __IPPDI_H__


#if defined (_WIN32_WCE) && defined (_M_IX86) && defined (__stdcall)
  #define _IPP_STDCALL_CDECL
  #undef __stdcall
#endif


#ifndef __IPPDEFS_H__
  #include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined( _IPP_NO_DEFAULT_LIB )
  #if defined( _IPP_PARALLEL_DYNAMIC )
    #pragma comment( lib, "ippdi" )
    #pragma comment( lib, "ippcore" )
  #elif defined( _IPP_PARALLEL_STATIC )
    #pragma comment( lib, "ippdi_t" )
    #pragma comment( lib, "ippcore_t" )
  #elif defined( _IPP_SEQUENTIAL_STATIC )
    #pragma comment( lib, "ippdi_l" )
    #pragma comment( lib, "ippcore_l" )
  #endif
#endif


/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippdiGetLibVersion
//  Purpose:    getting of the library version
//  Returns:    the structure of information about version of ippDI library
//  Parameters:
//
//  Notes:      not necessary to release the returned structure
*/
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI( const IppLibraryVersion*, ippdiGetLibVersion, (void) )


/* /////////////////////////////////////////////////////////////////////////////
//
// GF(2^m) extension of elementary GF(2)
//
*/
#if !defined( _OWN_BLDPCS )
   typedef struct _GF8  IppsGFSpec_8u;
#endif /* _OWN_BLDPCS */

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFGetSize_8u,(int gfDegree, int* pSize))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFInit_8u,(int gfDegree, const Ipp8u* pPolynomial, IppsGFSpec_8u* pGF))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFAdd_8u,(Ipp8u srcA, Ipp8u srcB, Ipp8u* pDstR, const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFSub_8u,(Ipp8u srcA, Ipp8u srcB, Ipp8u* pDstR, const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFMul_8u,(Ipp8u srcA, Ipp8u srcB, Ipp8u* pDstR, const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFDiv_8u,(Ipp8u srcA, Ipp8u srcB, Ipp8u* pDstR, const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFPow_8u,(Ipp8u srcA, int srcPwr, Ipp8u* pDstR, const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFInv_8u,(Ipp8u srcA,        Ipp8u* pDstR,  const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFNeg_8u,(Ipp8u srcA,        Ipp8u* pDstR,  const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFLogAlpha_8u,(Ipp8u srcA,   Ipp8u* pDstPwr,const IppsGFSpec_8u* pGF))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsGFExpAlpha_8u,(Ipp8u srcPwr, Ipp8u* pDdstR, const IppsGFSpec_8u* pGF))


/* /////////////////////////////////////////////////////////////////////////////
//
// polynomials over GF(2^m)
//
*/
#if !defined( _OWN_BLDPCS )
   typedef struct _PolyGF8 IppsPoly_GF8u;
#endif /* _OWN_BLDPCS */

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFGetSize_8u,(int maxDegree, int* pSize))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFInit_8u,(const IppsGFSpec_8u* pGF, int maxDegree,
                                    IppsPoly_GF8u* pPoly))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFSetCoeffs_8u,(const Ipp8u* pCoeff,int degree,
                                    IppsPoly_GF8u* pPoly))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFSetDegree_8u,(int degree,
                                    IppsPoly_GF8u* pPoly))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFCopy_8u,(const IppsPoly_GF8u* pPolyA, IppsPoly_GF8u* pPolyB))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFGetRef_8u,(Ipp8u** const pDstCoeff, int* pDstDegree,
                                    IppsGFSpec_8u** const pDstGF,
                                    const IppsPoly_GF8u* pPoly))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFAdd_8u,(const IppsPoly_GF8u* pSrcA, const IppsPoly_GF8u* pSrcB,
                                          IppsPoly_GF8u* pDstR))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFSub_8u,(const IppsPoly_GF8u* pSrcA, const IppsPoly_GF8u* pSrcB,
                                          IppsPoly_GF8u* pSrcR))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFMul_8u,(const IppsPoly_GF8u* pSrcA, const IppsPoly_GF8u* pSrcB,
                                          IppsPoly_GF8u* pDstR))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFMod_8u,(const IppsPoly_GF8u* pSrcA, const IppsPoly_GF8u* pSrcB,
                                          IppsPoly_GF8u* pDstR))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFDiv_8u,(const IppsPoly_GF8u* pSrcDividend, const IppsPoly_GF8u* pSrcDivisor,
                                          IppsPoly_GF8u* pDstQuotient,
                                          IppsPoly_GF8u* pDstReminder))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFShlC_8u,(const IppsPoly_GF8u* pSrc, int nShift,
                                           IppsPoly_GF8u* pDst))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFShrC_8u,(const IppsPoly_GF8u* pSrc, int nShift,
                                           IppsPoly_GF8u* pDst))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFIrreducible_8u,(const IppsPoly_GF8u* pSrc, IppBool* pIsIrreducible))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFPrimitive_8u,(const IppsPoly_GF8u* pSrc, IppBool isIrreducible, IppBool* pIsPrimitive))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFValue_8u,(const IppsPoly_GF8u* pSrc, Ipp8u srcE, Ipp8u* pDstValue))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFDerive_8u,(const IppsPoly_GF8u* pSrc, IppsPoly_GF8u* pDst))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFRoots_8u,(const IppsPoly_GF8u* pSrc,
                                      Ipp8u* pRoot, int* nRoots))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsPolyGFGCD_8u,(const IppsPoly_GF8u* pSrcA, const IppsPoly_GF8u* pSrcB,
                                          IppsPoly_GF8u* pDstGCD))



/* /////////////////////////////////////////////////////////////////////////////
//
// RS encoder
//
*/
#if !defined( _OWN_BLDPCS )
   typedef struct _RSencodeGF8   IppsRSEncodeSpec_8u;
#endif /* _OWN_BLDPCS */

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSEncodeGetSize_8u,(int codeLen, int dataLen, int* pSize))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSEncodeInit_8u,(int codeLen, int dataLen, const IppsGFSpec_8u* pGF, Ipp8u root,
                                       IppsRSEncodeSpec_8u* pRS))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSEncodeGetBufferSize_8u,(const IppsRSEncodeSpec_8u* pRS, int* pSize))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSEncode_8u,(const Ipp8u* pSrc,
                                         Ipp8u* pDst,
                                   const IppsRSEncodeSpec_8u* pRS,
                                         Ipp8u* pBuffer))


/* /////////////////////////////////////////////////////////////////////////////
//
// RS decoder
//
*/
#if !defined( _OWN_BLDPCS )
   typedef struct _RSdecodeGF8  IppsRSDecodeSpec_8u;
#endif /* _OWN_BLDPCS */

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSDecodeGetSize_8u,(int codeLen, int dataLen, int* pSize))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSDecodeInit_8u,(int codeLen, int dataLen, const IppsGFSpec_8u* pGF, Ipp8u root,
                                       IppsRSDecodeSpec_8u* pRS))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSDecodeBMGetBufferSize_8u,(const IppsRSDecodeSpec_8u* pRS, int* pSize))
IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSDecodeEEGetBufferSize_8u,(const IppsRSDecodeSpec_8u* pRS, int* pSize))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSDecodeBM_8u,(const int *pErasureList, int erasureListLen,
                                     Ipp8u* pSrcDstCodeWord,
                                     const IppsRSDecodeSpec_8u* pRS,
                                     Ipp8u* pBuffer))

IPP_DEPRECATED("is deprecated. This function is obsolete and will be removed in one of the future IPP releases. Use the following link for details: http://software.intel.com/en-us/articles/intel-ipp-71-deprecated-features/")\
IPPAPI(IppStatus, ippsRSDecodeEE_8u,(const int *pErasureList, int erasureListLen,
                                     Ipp8u* pSrcDstCodeWord,
                                     const IppsRSDecodeSpec_8u* pRS,
                                     Ipp8u* pBuffer))


#ifdef __cplusplus
}
#endif


#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif


#endif /* __IPPDI_H__ */
