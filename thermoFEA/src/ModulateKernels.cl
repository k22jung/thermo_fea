// Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//
// The source code, information and material ("Material") contained herein is owned 
// by Intel Corporation or its suppliers or licensors, and title to such Material 
// remains with Intel Corporation or its suppliers or licensors. The Material 
// contains proprietary information of Intel or its suppliers and licensors. 
// The Material is protected by worldwide copyright laws and treaty provisions. 
// No part of the Material may be used, copied, reproduced, modified, published, 
// uploaded, posted, transmitted, distributed or disclosed in any way without 
// Intel's prior express written permission. No license under any patent, copyright
// or other intellectual property rights in the Material is granted to or conferred 
// upon you, either expressly, by implication, inducement, estoppel or otherwise. 
// Any license under such intellectual property rights must be express and approved 
// by Intel in writing.
//
// Unless otherwise agreed by Intel in writing, you may not remove or alter this notice 
// or any other notice embedded in Materials by Intel or Intel’s suppliers or licensors 
// in any way.

// Modulate Kernels:

#define MODULATE_SHIFT_FACTOR	 (uchar)1
#define MODULATE_FACTOR          0.5f

////////////////////////////////////////////////////////////////////////////////////////

__kernel void Modulate_v1_uchar (__global uchar* pSrcImage, __global uchar* pDstImage)
{
	uint  xNDR		= get_global_id(0); 
	uint  yNDR		= get_global_id(1); 
	uint  yStride   = get_global_size(0);
	uint  index     = yNDR * yStride + xNDR;
				
	uchar src = pSrcImage[index];					// Read image data in		
	src >>= MODULATE_SHIFT_FACTOR;					// Modulate the image 
	pDstImage[index] = src;							// Write image data out		
}

__kernel void Modulate_v2_uchar16 (__global uchar16* pSrcImage, __global uchar16* pDstImage)
{
	uint	xNDR		= get_global_id(0); 
	uint	yNDR		= get_global_id(1); 
	uint    yStride     = get_global_size(0);
	uint	index       = yNDR * yStride + xNDR;
				
	uchar16	src = pSrcImage[index];					// Read image data in		
	src >>= MODULATE_SHIFT_FACTOR;					// Modulate the image 
	pDstImage[index] = src;							// Write image data out		
}

__kernel void Modulate_v3_uchar16_vload (__global uchar16* pSrcImage, __global uchar16* pDstImage)
{
	uint	xNDR		= get_global_id(0); 
	uint	yNDR		= get_global_id(1); 
	uint    yStride     = get_global_size(0);
	uint	index       = yNDR * yStride + xNDR;
				
	uchar16	src = vload16(index, (__global uchar*)pSrcImage);					// Read image data in		
	src >>= MODULATE_SHIFT_FACTOR;								                // Modulate the image 
	vstore16(src, index, (__global uchar*)pDstImage);							// Write image data out		
}

__kernel void Modulate_v4_uchar16_vload_to_float16 (__global uchar16* pSrcImage, __global uchar16* pDstImage)
{
	uint	xNDR		= get_global_id(0); 
	uint	yNDR		= get_global_id(1); 
	uint    yStride     = get_global_size(0);
	uint	index       = yNDR * yStride + xNDR;
				
	float16	src = convert_float16(vload16(index, (__global uchar*)pSrcImage));	// Read image data in		
	src *= MODULATE_FACTOR;									                    // Modulate the image 
	vstore16(convert_uchar16(src), index, (__global uchar*)pDstImage);			// Write image data out	
}

__kernel void Modulate_v5_uchar16_vload_to_float16_16 (__global uchar16* pSrcImage, __global uchar16* pDstImage)
{
	uint	xNDR		= get_global_id(0); 
	uint    yNDR        = get_global_id(1);
    uint    yStride     = get_global_size(0);
	uint	index       = 16 * yNDR * yStride + xNDR;
	float16	src; 

	for (uint y = 0; y < 16; y++)
	{
		src = convert_float16(vload16(index, (__global uchar*)pSrcImage));		// Read image data in		
		src *= MODULATE_FACTOR;									                // Modulate the image 
		vstore16(convert_uchar16(src), index, (__global uchar*)pDstImage);		// Write image data out		
		index += yStride;
	}
}

__kernel void Modulate_v6_uchar16_vload_to_float16_16_unroll (__global uchar16* pSrcImage, __global uchar16* pDstImage)
{
	uint	xNDR		= get_global_id(0); 
	uint    yNDR        = get_global_id(1);
    uint    yStride     = get_global_size(0);
	uint	index       = 16 * yNDR * yStride + xNDR;
	float16	src; 

	for (uint y = 0; y < 16; y += 4)
	{
		src = convert_float16(vload16(index, (__global uchar*)pSrcImage));		// Read image data in		
		src *= MODULATE_FACTOR;									                // Modulate the image 
		vstore16(convert_uchar16(src), index, (__global uchar*)pDstImage);		// Write image data out		
		index += yStride;
		src = convert_float16(vload16(index, (__global uchar*)pSrcImage));		// Read image data in		
		src *= MODULATE_FACTOR;									                // Modulate the image 
		vstore16(convert_uchar16(src), index, (__global uchar*)pDstImage);		// Write image data out		
		index += yStride;
		src = convert_float16(vload16(index, (__global uchar*)pSrcImage));		// Read image data in		
		src *= MODULATE_FACTOR;									                // Modulate the image 
		vstore16(convert_uchar16(src), index, (__global uchar*)pDstImage);		// Write image data out		
		index += yStride;
		src = convert_float16(vload16(index, (__global uchar*)pSrcImage));		// Read image data in		
		src *= MODULATE_FACTOR;									                // Modulate the image 
		vstore16(convert_uchar16(src), index, (__global uchar*)pDstImage);		// Write image data out		
		index += yStride;
	}
}