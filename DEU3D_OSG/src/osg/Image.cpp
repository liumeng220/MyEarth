/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/
#include <osg/GL>
#include <osg/GLU>

#include <osg/Image>
#include <osg/Notify>
#include <osg/io_utils>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/Texture2DArray>
#include <osg/Light>

#include <string.h>
#include <stdlib.h>

#include "dxtctool.h"

using namespace osg;
using namespace std;


Image::Image()
    :BufferData(),
    _fileName(""),
    _writeHint(NO_PREFERENCE),
    _origin(BOTTOM_LEFT),
    _s(0), _t(0), _r(0),
    _internalTextureFormat(0),
    _pixelFormat(0),
    _dataType(0),
    _packing(4),
    _pixelAspectRatio(1.0),
    _allocationMode(USE_NEW_DELETE),
    _data(0L)
{
    setDataVariance(STATIC); 
}

Image::Image(const Image& image,const CopyOp& copyop):
    BufferData(image,copyop),
    _fileName(image._fileName),
    _writeHint(image._writeHint),
    _origin(image._origin),
    _s(image._s), _t(image._t), _r(image._r),
    _internalTextureFormat(image._internalTextureFormat),
    _pixelFormat(image._pixelFormat),
    _dataType(image._dataType),
    _packing(image._packing),
    _pixelAspectRatio(image._pixelAspectRatio),
    _allocationMode(USE_NEW_DELETE),
    _data(0L),
    _mipmapData(image._mipmapData)
{
    if (image._data)
    {
        int size = image.getTotalSizeInBytesIncludingMipmaps();
        setData(new unsigned char [size],USE_NEW_DELETE);
        memcpy(_data,image._data,size);
    }
}

Image::~Image()
{
    deallocateData();
}

void Image::deallocateData()
{
    if (_data) {
        if (_allocationMode==USE_NEW_DELETE) delete [] _data;
        else if (_allocationMode==USE_MALLOC_FREE) ::free(_data);
        _data = 0;
    }
}

int Image::compare(const Image& rhs) const
{
    // if at least one filename is empty, then need to test buffer
    // pointers because images could have been created on the fly
    // and therefore we can't rely on file names to get an accurate
    // comparison
    if (getFileName().empty() || rhs.getFileName().empty())
    {
        if (_data<rhs._data) return -1;
        if (_data>rhs._data) return 1;
    }

    // need to test against image contents here...
    COMPARE_StateAttribute_Parameter(_s)
    COMPARE_StateAttribute_Parameter(_t)
    COMPARE_StateAttribute_Parameter(_internalTextureFormat)
    COMPARE_StateAttribute_Parameter(_pixelFormat)
    COMPARE_StateAttribute_Parameter(_dataType)
    COMPARE_StateAttribute_Parameter(_packing)
    COMPARE_StateAttribute_Parameter(_mipmapData)
    COMPARE_StateAttribute_Parameter(_modifiedCount)

    // same buffer + same parameters = same image
    if ((_data || rhs._data) && (_data == rhs._data)) return 0;

    // slowest comparison at the bottom!
    COMPARE_StateAttribute_Parameter(getFileName())    

    return 0;
}

void Image::setFileName(const std::string& fileName)
{
    _fileName = fileName;
}

void Image::setData(unsigned char* data, AllocationMode mode)
{
    deallocateData();
    _data = data;
    _allocationMode = mode;
}


bool Image::isPackedType(GLenum type)
{
    switch(type)
    {
        case(GL_UNSIGNED_BYTE_3_3_2):
        case(GL_UNSIGNED_BYTE_2_3_3_REV):
        case(GL_UNSIGNED_SHORT_5_6_5):
        case(GL_UNSIGNED_SHORT_5_6_5_REV):
        case(GL_UNSIGNED_SHORT_4_4_4_4):
        case(GL_UNSIGNED_SHORT_4_4_4_4_REV):
        case(GL_UNSIGNED_SHORT_5_5_5_1):
        case(GL_UNSIGNED_SHORT_1_5_5_5_REV):
        case(GL_UNSIGNED_INT_8_8_8_8):
        case(GL_UNSIGNED_INT_8_8_8_8_REV):
        case(GL_UNSIGNED_INT_10_10_10_2):
        case(GL_UNSIGNED_INT_2_10_10_10_REV): return true;
        default: return false;
    }    
}


GLenum Image::computePixelFormat(GLenum format)
{
    switch(format)
    {
        case(GL_ALPHA16F_ARB):
        case(GL_ALPHA32F_ARB):
            return GL_ALPHA;
        case(GL_LUMINANCE16F_ARB):
        case(GL_LUMINANCE32F_ARB):
            return GL_LUMINANCE;
        case(GL_INTENSITY16F_ARB):
        case(GL_INTENSITY32F_ARB):
            return GL_INTENSITY;
        case(GL_LUMINANCE_ALPHA16F_ARB):
        case(GL_LUMINANCE_ALPHA32F_ARB):
            return GL_LUMINANCE_ALPHA;
        case(GL_RGB32F_ARB):
        case(GL_RGB16F_ARB):
            return GL_RGB;
        case(GL_RGBA32F_ARB):
        case(GL_RGBA16F_ARB):
            return GL_RGBA;

        case(GL_ALPHA8I_EXT):
        case(GL_ALPHA16I_EXT):
        case(GL_ALPHA32I_EXT):
        case(GL_ALPHA8UI_EXT):
        case(GL_ALPHA16UI_EXT):
        case(GL_ALPHA32UI_EXT):
            return GL_ALPHA_INTEGER_EXT;
        case(GL_LUMINANCE8I_EXT):
        case(GL_LUMINANCE16I_EXT):
        case(GL_LUMINANCE32I_EXT):
        case(GL_LUMINANCE8UI_EXT):
        case(GL_LUMINANCE16UI_EXT):
        case(GL_LUMINANCE32UI_EXT):
            return GL_LUMINANCE_INTEGER_EXT;
        case(GL_INTENSITY8I_EXT):
        case(GL_INTENSITY16I_EXT):
        case(GL_INTENSITY32I_EXT):
        case(GL_INTENSITY8UI_EXT):
        case(GL_INTENSITY16UI_EXT):
        case(GL_INTENSITY32UI_EXT):
            OSG_WARN<<"Image::computePixelFormat("<<std::hex<<format<<std::dec<<") intensity pixel format is not correctly specified, so assume GL_LUMINANCE_INTEGER."<<std::endl;            
            return GL_LUMINANCE_INTEGER_EXT;
        case(GL_LUMINANCE_ALPHA8I_EXT):
        case(GL_LUMINANCE_ALPHA16I_EXT):
        case(GL_LUMINANCE_ALPHA32I_EXT):
        case(GL_LUMINANCE_ALPHA8UI_EXT):
        case(GL_LUMINANCE_ALPHA16UI_EXT):
        case(GL_LUMINANCE_ALPHA32UI_EXT):
            return GL_LUMINANCE_ALPHA_INTEGER_EXT;
        case(GL_RGB32I_EXT):
        case(GL_RGB16I_EXT):
        case(GL_RGB8I_EXT):
        case(GL_RGB32UI_EXT):
        case(GL_RGB16UI_EXT):
        case(GL_RGB8UI_EXT):
            return GL_RGB_INTEGER_EXT;
        case(GL_RGBA32I_EXT):
        case(GL_RGBA16I_EXT):
        case(GL_RGBA8I_EXT):
        case(GL_RGBA32UI_EXT):
        case(GL_RGBA16UI_EXT):
        case(GL_RGBA8UI_EXT):
            return GL_RGBA_INTEGER_EXT;;

        default:
            return format;
    }
}

GLenum Image::computeFormatDataType(GLenum pixelFormat)
{
    switch (pixelFormat)
    {
        case GL_LUMINANCE32F_ARB:
        case GL_LUMINANCE16F_ARB: 
        case GL_LUMINANCE_ALPHA32F_ARB:
        case GL_LUMINANCE_ALPHA16F_ARB: 
        case GL_RGB32F_ARB:
        case GL_RGB16F_ARB: 
        case GL_RGBA32F_ARB:
        case GL_RGBA16F_ARB: return GL_FLOAT;

        case GL_RGBA32UI_EXT:
        case GL_RGB32UI_EXT:
        case GL_LUMINANCE32UI_EXT:
        case GL_LUMINANCE_ALPHA32UI_EXT: return GL_UNSIGNED_INT;

        case GL_RGB16UI_EXT:
        case GL_RGBA16UI_EXT:
        case GL_LUMINANCE16UI_EXT: 
        case GL_LUMINANCE_ALPHA16UI_EXT: return GL_UNSIGNED_SHORT;

        case GL_RGBA8UI_EXT:
        case GL_RGB8UI_EXT:
        case GL_LUMINANCE8UI_EXT:
        case GL_LUMINANCE_ALPHA8UI_EXT:  return GL_UNSIGNED_BYTE;

        case GL_RGBA32I_EXT:  
        case GL_RGB32I_EXT:
        case GL_LUMINANCE32I_EXT:
        case GL_LUMINANCE_ALPHA32I_EXT: return GL_INT;

        case GL_RGBA16I_EXT:
        case GL_RGB16I_EXT:
        case GL_LUMINANCE16I_EXT:
        case GL_LUMINANCE_ALPHA16I_EXT: return GL_SHORT;

        case GL_RGB8I_EXT: 
        case GL_RGBA8I_EXT: 
        case GL_LUMINANCE8I_EXT: 
        case GL_LUMINANCE_ALPHA8I_EXT: return GL_BYTE;

        case GL_RGBA:
        case GL_RGB:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA: return GL_UNSIGNED_BYTE;

        default: 
        {
            OSG_WARN<<"error computeFormatType = "<<std::hex<<pixelFormat<<std::dec<<std::endl;
            return 0;
        }
    }
}

unsigned int Image::computeNumComponents(GLenum pixelFormat)
{
    switch(pixelFormat)
    {
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT): return 3;
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT): return 4;
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT): return 4;
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT): return 4;
        case(GL_COMPRESSED_SIGNED_RED_RGTC1_EXT): return 1;
        case(GL_COMPRESSED_RED_RGTC1_EXT):   return 1;
        case(GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT): return 2;
        case(GL_COMPRESSED_RED_GREEN_RGTC2_EXT): return 2;    
        case(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG): return 3;
        case(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG): return 3;
        case(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG): return 4;
        case(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG): return 4;
        case(GL_ETC1_RGB8_OES): return 3;
        case(GL_COLOR_INDEX): return 1;
        case(GL_STENCIL_INDEX): return 1;
        case(GL_DEPTH_COMPONENT): return 1;
        case(GL_RED): return 1;
        case(GL_GREEN): return 1;
        case(GL_BLUE): return 1;
        case(GL_ALPHA): return 1;
        case(GL_ALPHA8I_EXT): return 1;
        case(GL_ALPHA8UI_EXT): return 1;
        case(GL_ALPHA16I_EXT): return 1;
        case(GL_ALPHA16UI_EXT): return 1;
        case(GL_ALPHA32I_EXT): return 1;
        case(GL_ALPHA32UI_EXT): return 1;
        case(GL_ALPHA16F_ARB): return 1;
        case(GL_ALPHA32F_ARB): return 1;
        case(GL_RGB): return 3;
        case(GL_BGR): return 3;
        case(GL_RGB8I_EXT): return 3;
        case(GL_RGB8UI_EXT): return 3;
        case(GL_RGB16I_EXT): return 3;
        case(GL_RGB16UI_EXT): return 3;
        case(GL_RGB32I_EXT): return 3;
        case(GL_RGB32UI_EXT): return 3;
        case(GL_RGB16F_ARB): return 3;
        case(GL_RGB32F_ARB): return 3;
        case(GL_RGBA16F_ARB): return 4;
        case(GL_RGBA32F_ARB): return 4;
        case(GL_RGBA): return 4;
        case(GL_BGRA): return 4;
        case(GL_RGBA8): return 4;
        case(GL_LUMINANCE): return 1;
        case(GL_LUMINANCE4): return 1;
        case(GL_LUMINANCE8): return 1;
        case(GL_LUMINANCE12): return 1;
        case(GL_LUMINANCE16): return 1;
        case(GL_LUMINANCE8I_EXT): return 1;
        case(GL_LUMINANCE8UI_EXT): return 1;
        case(GL_LUMINANCE16I_EXT): return 1;
        case(GL_LUMINANCE16UI_EXT): return 1;
        case(GL_LUMINANCE32I_EXT): return 1;
        case(GL_LUMINANCE32UI_EXT): return 1;
        case(GL_LUMINANCE16F_ARB): return 1;
        case(GL_LUMINANCE32F_ARB): return 1;
        case(GL_LUMINANCE4_ALPHA4): return 2;
        case(GL_LUMINANCE6_ALPHA2): return 2;
        case(GL_LUMINANCE8_ALPHA8): return 2;
        case(GL_LUMINANCE12_ALPHA4): return 2;
        case(GL_LUMINANCE12_ALPHA12): return 2;
        case(GL_LUMINANCE16_ALPHA16): return 2;
        case(GL_INTENSITY): return 1;
        case(GL_INTENSITY4): return 1;
        case(GL_INTENSITY8): return 1;
        case(GL_INTENSITY12): return 1;
        case(GL_INTENSITY16): return 1;
        case(GL_INTENSITY8UI_EXT): return 1;
        case(GL_INTENSITY8I_EXT): return 1;
        case(GL_INTENSITY16I_EXT): return 1;
        case(GL_INTENSITY16UI_EXT): return 1;
        case(GL_INTENSITY32I_EXT): return 1;
        case(GL_INTENSITY32UI_EXT): return 1;
        case(GL_INTENSITY16F_ARB): return 1;
        case(GL_INTENSITY32F_ARB): return 1;
        case(GL_LUMINANCE_ALPHA): return 2;
        case(GL_LUMINANCE_ALPHA8I_EXT): return 2;
        case(GL_LUMINANCE_ALPHA8UI_EXT): return 2;
        case(GL_LUMINANCE_ALPHA16I_EXT): return 2;
        case(GL_LUMINANCE_ALPHA16UI_EXT): return 2;
        case(GL_LUMINANCE_ALPHA32I_EXT): return 2;
        case(GL_LUMINANCE_ALPHA32UI_EXT): return 2;
        case(GL_LUMINANCE_ALPHA16F_ARB): return 2;
        case(GL_LUMINANCE_ALPHA32F_ARB): return 2;
        case(GL_HILO_NV): return 2;
        case(GL_DSDT_NV): return 2;
        case(GL_DSDT_MAG_NV): return 3;
        case(GL_DSDT_MAG_VIB_NV): return 4;
        case(GL_RED_INTEGER_EXT): return 1;
        case(GL_GREEN_INTEGER_EXT): return 1;
        case(GL_BLUE_INTEGER_EXT): return 1;
        case(GL_ALPHA_INTEGER_EXT): return 1;
        case(GL_RGB_INTEGER_EXT): return 3;
        case(GL_RGBA_INTEGER_EXT): return 4;
        case(GL_BGR_INTEGER_EXT): return 3;
        case(GL_BGRA_INTEGER_EXT): return 4;
        case(GL_LUMINANCE_INTEGER_EXT): return 1;
        case(GL_LUMINANCE_ALPHA_INTEGER_EXT): return 2;

        default:
        {
            OSG_WARN<<"error pixelFormat = "<<std::hex<<pixelFormat<<std::dec<<std::endl;
            return 0;
        }
    }        
}


unsigned int Image::computePixelSizeInBits(GLenum format,GLenum type)
{

    switch(format)
    {
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT): return 4;
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT): return 4;
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT): return 8;
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT): return 8;
        case(GL_COMPRESSED_SIGNED_RED_RGTC1_EXT): return 4;
        case(GL_COMPRESSED_RED_RGTC1_EXT):   return 4;
        case(GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT): return 8;
        case(GL_COMPRESSED_RED_GREEN_RGTC2_EXT): return 8;
        case(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG): return 4;
        case(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG): return 2;
        case(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG): return 4;
        case(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG): return 2;
        case(GL_ETC1_RGB8_OES): return 4;
        default: break;
    }

    // note, haven't yet added proper handling of the ARB GL_COMPRESSRED_* pathways
    // yet, no clear size for these since its probably implementation dependent
    // which raises the question of how to actually querry for these sizes...
    // will need to revisit this issue, for now just report an error.
    // this is possible a bit of mute point though as since the ARB compressed formats
    // arn't yet used for storing images to disk, so its likely that users wont have
    // osg::Image's for pixel formats set the ARB compressed formats, just using these
    // compressed formats as internal texture modes.  This is very much speculation though
    // if get the below error then its time to revist this issue :-)
    // Robert Osfield, Jan 2005.
    switch(format)
    {
        case(GL_COMPRESSED_ALPHA):
        case(GL_COMPRESSED_LUMINANCE):
        case(GL_COMPRESSED_LUMINANCE_ALPHA):
        case(GL_COMPRESSED_INTENSITY):
        case(GL_COMPRESSED_RGB):
        case(GL_COMPRESSED_RGBA):
            OSG_WARN<<"Image::computePixelSizeInBits(format,type) : cannot compute correct size of compressed format ("<<format<<") returning 0."<<std::endl;
            return 0;
        default: break;
    }

    switch(format)
    {
        case(GL_LUMINANCE4): return 4;
        case(GL_LUMINANCE8): return 8;
        case(GL_LUMINANCE12): return 12;
        case(GL_LUMINANCE16): return 16;
        case(GL_LUMINANCE4_ALPHA4): return 8;
        case(GL_LUMINANCE6_ALPHA2): return 8;
        case(GL_LUMINANCE8_ALPHA8): return 16;
        case(GL_LUMINANCE12_ALPHA4): return 16;
        case(GL_LUMINANCE12_ALPHA12): return 24;
        case(GL_LUMINANCE16_ALPHA16): return 32;
        case(GL_INTENSITY4): return 4;
        case(GL_INTENSITY8): return 8;
        case(GL_INTENSITY12): return 12;
        case(GL_INTENSITY16): return 16;
        default: break;
    }

    switch(type)
    {
   
        case(GL_BITMAP): return computeNumComponents(format);
        
        case(GL_BYTE):
        case(GL_UNSIGNED_BYTE): return 8*computeNumComponents(format);
        
        case(GL_HALF_FLOAT_NV):
        case(GL_SHORT):
        case(GL_UNSIGNED_SHORT): return 16*computeNumComponents(format);
        
        case(GL_INT):
        case(GL_UNSIGNED_INT):
        case(GL_FLOAT): return 32*computeNumComponents(format);
    
    
        case(GL_UNSIGNED_BYTE_3_3_2): 
        case(GL_UNSIGNED_BYTE_2_3_3_REV): return 8;
        
        case(GL_UNSIGNED_SHORT_5_6_5):
        case(GL_UNSIGNED_SHORT_5_6_5_REV):
        case(GL_UNSIGNED_SHORT_4_4_4_4):
        case(GL_UNSIGNED_SHORT_4_4_4_4_REV):
        case(GL_UNSIGNED_SHORT_5_5_5_1):
        case(GL_UNSIGNED_SHORT_1_5_5_5_REV): return 16;
        
        case(GL_UNSIGNED_INT_8_8_8_8):
        case(GL_UNSIGNED_INT_8_8_8_8_REV):
        case(GL_UNSIGNED_INT_10_10_10_2):
        case(GL_UNSIGNED_INT_2_10_10_10_REV): return 32;
        default: 
        {
            OSG_WARN<<"error type = "<<type<<std::endl;
            return 0;
        }
    }    

}

unsigned int Image::computeRowWidthInBytes(int width,GLenum pixelFormat,GLenum type,int packing)
{
    unsigned int pixelSize = computePixelSizeInBits(pixelFormat,type);
    int widthInBits = width*pixelSize;
    int packingInBits = packing*8;
    //OSG_INFO << "width="<<width<<" pixelSize="<<pixelSize<<"  width in bit="<<widthInBits<<" packingInBits="<<packingInBits<<" widthInBits%packingInBits="<<widthInBits%packingInBits<<std::endl;
    return (widthInBits/packingInBits + ((widthInBits%packingInBits)?1:0))*packing;
}

int Image::computeNearestPowerOfTwo(int s,float bias)
{
    if ((s & (s-1))!=0)
    {
        // it isn't so lets find the closest power of two.
        // yes, logf and powf are slow, but this code should
        // only be called during scene graph initilization,
        // if at all, so not critical in the greater scheme.
        float p2 = logf((float)s)/logf(2.0f);
        float rounded_p2 = floorf(p2+bias);
        s = (int)(powf(2.0f,rounded_p2));
    }
    return s;
}

int Image::computeNumberOfMipmapLevels(int s,int t, int r)
{
    int w = maximum(s, t);
    w = maximum(w, r);
    return 1 + static_cast<int>(floor(logf(w)/logf(2.0f)));
}


GLenum Image::findBestBitsOf16(void) const
{
    if(getPixelSizeInBits() != 32u) return 0;
    if(r() < 1u || s() < 1u || t() < 1u)
    {
        return 0;
    }

    const GLenum ePixelFormat = getPixelFormat();
    if((ePixelFormat != GL_RGBA) && (ePixelFormat != GL_BGRA))  return 0;

    unsigned nBlendCount = 0u;
    for(int r = 0; r < _r; r++)
    {
        for(int t = 0; t < _t; t++)
        {
            const unsigned char *pRowData = data(0, t, r);
            for(int s = 0; s < _s; s++)
            {
                const unsigned alpha0 = pRowData[3];
                if((alpha0 > 64u) && (alpha0 < 192u))
                {
                    nBlendCount++;
                }
                pRowData += 4u;
            }
        }
    }

    const double dblTotalCount = _r * _t * _s;
    const double dblRatio = nBlendCount / dblTotalCount;
    if(dblRatio < 0.1)
    {
        return GL_UNSIGNED_SHORT_5_5_5_1;
    }
    return GL_UNSIGNED_SHORT_4_4_4_4;
}


bool Image::isCompressed() const
{
    switch(_pixelFormat)
    {
        case(GL_COMPRESSED_ALPHA_ARB):
        case(GL_COMPRESSED_INTENSITY_ARB):
        case(GL_COMPRESSED_LUMINANCE_ALPHA_ARB):
        case(GL_COMPRESSED_LUMINANCE_ARB):
        case(GL_COMPRESSED_RGBA_ARB):
        case(GL_COMPRESSED_RGB_ARB):
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT):
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT):
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT):
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT):
        case(GL_COMPRESSED_SIGNED_RED_RGTC1_EXT):
        case(GL_COMPRESSED_RED_RGTC1_EXT):
        case(GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT):
        case(GL_COMPRESSED_RED_GREEN_RGTC2_EXT):
        case(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG): 
        case(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG):
        case(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG):
        case(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG):
        case(GL_ETC1_RGB8_OES):
            return true;
        default:
            return false;
    }
}

unsigned int Image::getTotalSizeInBytesIncludingMipmaps() const
{
    if (_mipmapData.empty()) 
    {
        // no mips so just return size of main image
        return getTotalSizeInBytes();
    }
    
    int s = _s;
    int t = _t;
    int r = _r;
    
    unsigned int maxValue = 0;
    for(unsigned int i=0;i<_mipmapData.size() && _mipmapData[i];++i)
    {
        s >>= 1;
        t >>= 1;
        r >>= 1;
        maxValue = maximum(maxValue,_mipmapData[i]);
   }
   
   if (s==0) s=1;
   if (t==0) t=1;
   if (r==0) r=1;
   
   unsigned int sizeOfLastMipMap = computeRowWidthInBytes(s,_pixelFormat,_dataType,_packing)* r*t;
   switch(_pixelFormat)
   {
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT):
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT):
           sizeOfLastMipMap = maximum(sizeOfLastMipMap, 8u); // block size of 8
           break;
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT):
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT):
        case(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG):
        case(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG):
        case(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG):
        case(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG):
        case(GL_ETC1_RGB8_OES):
           sizeOfLastMipMap = maximum(sizeOfLastMipMap, 16u); // block size of 16
           break;
        case(GL_COMPRESSED_SIGNED_RED_RGTC1_EXT):
        case(GL_COMPRESSED_RED_RGTC1_EXT):
            sizeOfLastMipMap = maximum(sizeOfLastMipMap, 8u); // block size of 8
            break;
        case(GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT):
        case(GL_COMPRESSED_RED_GREEN_RGTC2_EXT):
            sizeOfLastMipMap = maximum(sizeOfLastMipMap, 16u); // block size of 8
            break;
        default: break;
   }

   // OSG_INFO<<"sizeOfLastMipMap="<<sizeOfLastMipMap<<"\ts="<<s<<"\tt="<<t<<"\tr"<<r<<std::endl;                  

   return maxValue+sizeOfLastMipMap;

}


void Image::setInternalTextureFormat(GLint internalFormat)
{
    // won't do any sanity checking right now, leave it to 
    // OpenGL to make the call.
    _internalTextureFormat = internalFormat;
}

void Image::setPixelFormat(GLenum pixelFormat)
{
    _pixelFormat = pixelFormat;
}

void Image::setDataType(GLenum dataType)
{
    if (_dataType==dataType) return; // do nothing if the same.

    if (_dataType==0)
    {
        // setting the datatype for the first time
        _dataType = dataType;
    }
    else
    {
        OSG_WARN<<"Image::setDataType(..) - warning, attempt to reset the data type not permitted."<<std::endl;
    }
}


void Image::allocateImage(int s,int t,int r,
                        GLenum format,GLenum type,
                        int packing)
{
    _mipmapData.clear();

    unsigned int previousTotalSize = 0;
    
    if (_data) previousTotalSize = computeRowWidthInBytes(_s,_pixelFormat,_dataType,_packing)*_t*_r;
    
    unsigned int newTotalSize = computeRowWidthInBytes(s,format,type,packing)*t*r;

    if (newTotalSize!=previousTotalSize)
    {
        if (newTotalSize)
            setData(new unsigned char [newTotalSize],USE_NEW_DELETE);
        else
            deallocateData(); // and sets it to NULL.
    }

    if (_data)
    {
        _s = s;
        _t = t;
        _r = r;
        _pixelFormat = format;
        _dataType = type;
        _packing = packing;
        
        // preserve internalTextureFormat if already set, otherwise
        // use the pixelFormat as the source for the format.
        if (_internalTextureFormat==0) _internalTextureFormat = format;
    }
    else
    {
    
        // failed to allocate memory, for now, will simply set values to 0.
        _s = 0;
        _t = 0;
        _r = 0;
        _pixelFormat = 0;
        _dataType = 0;
        _packing = 0;
        
        // commenting out reset of _internalTextureFormat as we are changing
        // policy so that allocateImage honours previous settings of _internalTextureFormat.
        //_internalTextureFormat = 0;
    }
    
    dirty();
}

void Image::setImage(int s,int t,int r,
                     GLint internalTextureFormat,
                     GLenum format,GLenum type,
                     unsigned char *data,
                     AllocationMode mode,
                     int packing)
{
    _mipmapData.clear();

    _s = s;
    _t = t;
    _r = r;

    _internalTextureFormat = internalTextureFormat;
    _pixelFormat    = format;
    _dataType       = type;

    setData(data,mode);

    _packing = packing;
        
    dirty();

}

void Image::readPixels(int x,int y,int width,int height,
                       GLenum format,GLenum type)
{
    allocateImage(width,height,1,format,type);

    glPixelStorei(GL_PACK_ALIGNMENT,_packing);

    glReadPixels(x,y,width,height,format,type,_data);
}


void Image::readImageFromCurrentTexture(unsigned int contextID, bool copyMipMapsIfAvailable, GLenum type)
{
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
    // OSG_NOTICE<<"Image::readImageFromCurrentTexture()"<<std::endl;

    const osg::Texture::Extensions* extensions = osg::Texture::getExtensions(contextID,true);
    const osg::Texture3D::Extensions* extensions3D = osg::Texture3D::getExtensions(contextID,true);
    const osg::Texture2DArray::Extensions* extensions2DArray = osg::Texture2DArray::getExtensions(contextID,true);

    
    GLboolean binding1D = GL_FALSE, binding2D = GL_FALSE, binding3D = GL_FALSE, binding2DArray = GL_FALSE;

    glGetBooleanv(GL_TEXTURE_BINDING_1D, &binding1D);
    glGetBooleanv(GL_TEXTURE_BINDING_2D, &binding2D);
    glGetBooleanv(GL_TEXTURE_BINDING_3D, &binding3D);

    if (extensions2DArray->isTexture2DArraySupported())
    {
        glGetBooleanv(GL_TEXTURE_BINDING_2D_ARRAY_EXT, &binding2DArray);
    }

    GLenum textureMode = binding1D ? GL_TEXTURE_1D : binding2D ? GL_TEXTURE_2D : binding3D ? GL_TEXTURE_3D : binding2DArray ? GL_TEXTURE_2D_ARRAY_EXT : 0;
    
    if (textureMode==0) return;

    GLint internalformat;
    GLint width;
    GLint height;
    GLint depth;
    GLint packing;

    GLint numMipMaps = 0;
    if (copyMipMapsIfAvailable)
    {
        for(;numMipMaps<20;++numMipMaps)
        {
            glGetTexLevelParameteriv(textureMode, numMipMaps, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(textureMode, numMipMaps, GL_TEXTURE_HEIGHT, &height);
            glGetTexLevelParameteriv(textureMode, numMipMaps, GL_TEXTURE_DEPTH, &depth);
            // OSG_NOTICE<<"   numMipMaps="<<numMipMaps<<" width="<<width<<" height="<<height<<" depth="<<depth<<std::endl;
            if (width==0 || height==0 || depth==0) break;
        }
    }
    else
    {
        numMipMaps = 1;
    }
    
    // OSG_NOTICE<<"Image::readImageFromCurrentTexture() : numMipMaps = "<<numMipMaps<<std::endl;

        
    GLint compressed = 0;

    if (textureMode==GL_TEXTURE_2D)
    {
        if (extensions->isCompressedTexImage2DSupported())
        {
            glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_COMPRESSED_ARB,&compressed);
        }
    }
    else if (textureMode==GL_TEXTURE_3D)
    {
        if (extensions3D->isCompressedTexImage3DSupported())
        {
            glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_COMPRESSED_ARB,&compressed);
        }
    }
    else if (textureMode==GL_TEXTURE_2D_ARRAY_EXT)
    {
        if (extensions2DArray->isCompressedTexImage3DSupported())
        {
            glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_COMPRESSED_ARB,&compressed);
        }
    }
    
    
        
    /* if the compression has been successful */
    if (compressed == GL_TRUE)
    {

        MipmapDataType mipMapData;
        
        unsigned int total_size = 0;
        GLint i;
        for(i=0;i<numMipMaps;++i)
        {
            if (i>0) mipMapData.push_back(total_size);
            
            GLint compressed_size;
            glGetTexLevelParameteriv(textureMode, i, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &compressed_size);
            
            total_size += compressed_size;
        }
        
        
        unsigned char* data = new unsigned char[total_size];
        if (!data)
        {
            OSG_WARN<<"Warning: Image::readImageFromCurrentTexture(..) out of memory, now image read."<<std::endl;
            return; 
        }

        deallocateData(); // and sets it to NULL.

        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);
        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_DEPTH, &depth);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &packing);
        glPixelStorei(GL_PACK_ALIGNMENT, packing);

        _data = data;
        _s = width;
        _t = height;
        _r = depth;
        
        _pixelFormat = internalformat;
        _dataType = type;
        _internalTextureFormat = internalformat;
        _mipmapData = mipMapData;
        _allocationMode=USE_NEW_DELETE;
        _packing = packing;
        
        for(i=0;i<numMipMaps;++i)
        {
            extensions->glGetCompressedTexImage(textureMode, i, getMipmapData(i));
        }

        dirty();
    
    }
    else
    {
        MipmapDataType mipMapData;

        // Get the internal texture format and packing value from OpenGL,
        // instead of using possibly outdated values from the class.
        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &packing);
        glPixelStorei(GL_PACK_ALIGNMENT, packing);

        unsigned int total_size = 0;
        GLint i;
        for(i=0;i<numMipMaps;++i)
        {
            if (i>0) mipMapData.push_back(total_size);
            
            glGetTexLevelParameteriv(textureMode, i, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(textureMode, i, GL_TEXTURE_HEIGHT, &height);
            glGetTexLevelParameteriv(textureMode, i, GL_TEXTURE_DEPTH, &depth);
            
            unsigned int level_size = computeRowWidthInBytes(width,internalformat,type,packing)*height*depth;

            total_size += level_size;
        }
        
        
        unsigned char* data = new unsigned char[total_size];
        if (!data)
        {
            OSG_WARN<<"Warning: Image::readImageFromCurrentTexture(..) out of memory, now image read."<<std::endl;
            return; 
        }

        deallocateData(); // and sets it to NULL.

        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTexLevelParameteriv(textureMode, 0, GL_TEXTURE_DEPTH, &depth);

        _data = data;
        _s = width;
        _t = height;
        _r = depth;
        
        _pixelFormat = computePixelFormat(internalformat);
        _dataType = type;
        _internalTextureFormat = internalformat;
        _mipmapData = mipMapData;
        _allocationMode=USE_NEW_DELETE;
        _packing = packing;
        
        for(i=0;i<numMipMaps;++i)
        {
            glGetTexImage(textureMode,i,_pixelFormat,_dataType,getMipmapData(i));
        }

        dirty();
    }    
#else
    OSG_NOTICE<<"Warning: Image::readImageFromCurrentTexture() not supported."<<std::endl;
#endif
}

void Image::scaleImage(int s,int t,int r, GLenum newDataType)
{
    if (_s==s && _t==t && _r==r) return;

    if (_data==NULL)
    {
        OSG_WARN << "Error Image::scaleImage() do not succeed : cannot scale NULL image."<<std::endl;
        return;
    }

    if (_r!=1 || r!=1)
    {
        OSG_WARN << "Error Image::scaleImage() do not succeed : scaling of volumes not implemented."<<std::endl;
        return;
    }

    unsigned int newTotalSize = computeRowWidthInBytes(s,_pixelFormat,newDataType,_packing)*t;

    // need to sort out what size to really use...
    unsigned char* newData = new unsigned char [newTotalSize];
    if (!newData)
    {
        // should we throw an exception???  Just return for time being.
        OSG_FATAL << "Error Image::scaleImage() do not succeed : out of memory."<<newTotalSize<<std::endl;
        return;
    }

    PixelStorageModes psm;
    psm.pack_alignment = _packing;
    psm.unpack_alignment = _packing;

    GLint status = gluScaleImage(&psm, _pixelFormat,
        _s,
        _t,
        _dataType,
        _data,
        s,
        t,
        newDataType,
        newData);

    if (status==0)
    {

        // free old image.
        _s = s;
        _t = t;
        _dataType = newDataType;
        setData(newData,USE_NEW_DELETE);
    }
    else
    {
        delete [] newData;

        OSG_WARN << "Error Image::scaleImage() did not succeed : errorString = "<< gluErrorString((GLenum)status) << ". The rendering context may be invalid." << std::endl;
    }

    dirty();
}

void Image::copySubImage(int s_offset, int t_offset, int r_offset, const osg::Image* source)
{
    if (!source) return;
    if (s_offset<0 || t_offset<0 || r_offset<0)
    {
        OSG_WARN<<"Warning: negative offsets passed to Image::copySubImage(..) not supported, operation ignored."<<std::endl;
        return;
    }

    if (!_data)
    {
        OSG_INFO<<"allocating image"<<endl;
        allocateImage(s_offset+source->s(),t_offset+source->t(),r_offset+source->r(),
                    source->getPixelFormat(),source->getDataType(),
                    source->getPacking());
    }

    if (s_offset>=_s || t_offset>=_t  || r_offset>=_r)
    {
        OSG_WARN<<"Warning: offsets passed to Image::copySubImage(..) outside destination image, operation ignored."<<std::endl;
        return;
    }


    if (_pixelFormat != source->getPixelFormat())
    {
        OSG_WARN<<"Warning: image with an incompatible pixel formats passed to Image::copySubImage(..), operation ignored."<<std::endl;
        return;
    }

    void* data_destination = data(s_offset,t_offset,r_offset);

    PixelStorageModes psm;
    psm.pack_alignment = _packing;
    psm.pack_row_length = _s;
    psm.unpack_alignment = _packing;

    GLint status = gluScaleImage(&psm, _pixelFormat,
        source->s(),
        source->t(),
        source->getDataType(),
        source->data(),
        source->s(),
        source->t(),
        _dataType,
        data_destination);

    glPixelStorei(GL_PACK_ROW_LENGTH,0);

    if (status!=0)
    {
        OSG_WARN << "Error Image::scaleImage() did not succeed : errorString = "<< gluErrorString((GLenum)status) << ". The rendering context may be invalid." << std::endl;
    }
}

void Image::flipHorizontal()
{
    if (_data==NULL)
    {
        OSG_WARN << "Error Image::flipHorizontal() did not succeed : cannot flip NULL image."<<std::endl;
        return;
    }

    unsigned int elemSize = getPixelSizeInBits()/8;

    if (_mipmapData.empty())
    {

        for(int r=0;r<_r;++r)
        {
            for (int t=0; t<_t; ++t)
            {
                unsigned char* rowData = _data+t*getRowSizeInBytes()+r*getImageSizeInBytes();
                unsigned char* left  = rowData ;
                unsigned char* right = rowData + ((_s-1)*getPixelSizeInBits())/8;

                while (left < right)
                {
                    char tmp[32];  // max elem size is four floats
                    memcpy(tmp, left, elemSize);
                    memcpy(left, right, elemSize);
                    memcpy(right, tmp, elemSize);
                    left  += elemSize;
                    right -= elemSize;
                }
            }
        }
    }
    else
    {
        OSG_WARN << "Error Image::flipHorizontal() did not succeed : cannot flip mipmapped image."<<std::endl;
        return;
    }
        
    dirty();
}

void flipImageVertical(unsigned char* top, unsigned char* bottom, unsigned int rowSize)
{
    while(top<bottom)
    {
        for(unsigned int i=0;i<rowSize;++i, ++top,++bottom)
        {
            unsigned char temp=*top;
            *top = *bottom;
            *bottom = temp;
        }
        bottom -= 2*rowSize;
    }
}


void Image::flipVertical()
{
    if (_data==NULL)
    {
        OSG_WARN << "Error Image::flipVertical() do not succeed : cannot flip NULL image."<<std::endl;
        return;
    }

    if (!_mipmapData.empty() && _r>1)
    {
        OSG_WARN << "Error Image::flipVertical() do not succeed : flipping of mipmap 3d textures not yet supported."<<std::endl;
        return;
    }

    if (_mipmapData.empty())
    {
        // no mipmaps,
        // so we can safely handle 3d textures
        for(int r=0;r<_r;++r)
        {
            if (!dxtc_tool::VerticalFlip(_s,_t,_pixelFormat,data(0,0,r)))
            {
                // its not a compressed image, so implement flip oursleves.
                
                unsigned int rowSize = computeRowWidthInBytes(_s,_pixelFormat,_dataType,_packing);
                unsigned char* top = data(0,0,r);
                unsigned char* bottom = top + (_t-1)*rowSize;
                    
                flipImageVertical(top, bottom, rowSize);
            }
        }
    }
    else if (_r==1)
    {
        if (!dxtc_tool::VerticalFlip(_s,_t,_pixelFormat,_data))
        {
            // its not a compressed image, so implement flip oursleves.
            unsigned int rowSize = computeRowWidthInBytes(_s,_pixelFormat,_dataType,_packing);
            unsigned char* top = data(0,0,0);
            unsigned char* bottom = top + (_t-1)*rowSize;

            flipImageVertical(top, bottom, rowSize);
        }

        int s = _s;
        int t = _t;
        //int r = _r;

        for(unsigned int i=0;i<_mipmapData.size() && _mipmapData[i];++i)
        {
            s >>= 1;
            t >>= 1;
            if (s==0) s=1;
            if (t==0) t=1;
            if (!dxtc_tool::VerticalFlip(s,t,_pixelFormat,_data+_mipmapData[i]))
            {
                // its not a compressed image, so implement flip oursleves.
                unsigned int rowSize = computeRowWidthInBytes(s,_pixelFormat,_dataType,_packing);
                unsigned char* top = _data+_mipmapData[i];
                unsigned char* bottom = top + (t-1)*rowSize;

                flipImageVertical(top, bottom, rowSize);
            }
       }
    }   

    dirty();
}



void Image::ensureValidSizeForTexturing(GLint maxTextureSize)
{
    int new_s = computeNearestPowerOfTwo(_s);
    int new_t = computeNearestPowerOfTwo(_t);
    
    if (new_s>maxTextureSize) new_s = maxTextureSize;
    if (new_t>maxTextureSize) new_t = maxTextureSize;
    
    if (new_s!=_s || new_t!=_t)
    {
        //if (!_fileName.empty()) { OSG_NOTICE << "Scaling image '"<<_fileName<<"' from ("<<_s<<","<<_t<<") to ("<<new_s<<","<<new_t<<")"<<std::endl; }
        //else { OSG_NOTICE << "Scaling image from ("<<_s<<","<<_t<<") to ("<<new_s<<","<<new_t<<")"<<std::endl; }

        scaleImage(new_s,new_t,_r);
    }
}


template <typename T>    
bool _findLowerAlphaValueInRow(unsigned int num, T* data,T value, unsigned int delta)
{
    for(unsigned int i=0;i<num;++i)
    {
        if (*data<value) return true;
        data += delta;
    }
    return false;
}

template <typename T>    
bool _maskedFindLowerAlphaValueInRow(unsigned int num, T* data,T value, T mask, unsigned int delta)
{
    for(unsigned int i=0;i<num;++i)
    {
        if ((*data & mask)<value) return true;
        data += delta;
    }
    return false;
}

bool Image::isImageTranslucent() const
{
    unsigned int offset = 0;
    unsigned int delta = 1;
    switch(_pixelFormat)
    {
        case(GL_ALPHA):
            offset = 0;
            delta = 1;
            break;
        case(GL_LUMINANCE_ALPHA):
            offset = 1;
            delta = 2;
            break;
        case(GL_RGBA):
            offset = 3;
            delta = 4;
            break;
        case(GL_BGRA):
            offset = 3;
            delta = 4;
            break;
        case(GL_RGB):
            return false;
        case(GL_BGR):
            return false;
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT):
            return false;
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT):
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT):
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT):
            return dxtc_tool::CompressedImageTranslucent(_s, _t, _pixelFormat, _data);
        default:
            return false;
    }

    for(int ir=0;ir<r();++ir)
    {
        for(int it=0;it<t();++it)
        {
            const unsigned char* d = data(0,it,ir);
            switch(_dataType)
            {
                case(GL_BYTE):
                    if (_findLowerAlphaValueInRow(s(), (char*)d +offset, (char)127, delta))
                        return true;
                    break;
                case(GL_UNSIGNED_BYTE):
                    if (_findLowerAlphaValueInRow(s(), (unsigned char*)d + offset, (unsigned char)255, delta))
                        return true;
                    break;
                case(GL_SHORT):
                    if (_findLowerAlphaValueInRow(s(), (short*)d + offset, (short)32767, delta))
                        return true;
                    break;
                case(GL_UNSIGNED_SHORT):
                    if (_findLowerAlphaValueInRow(s(), (unsigned short*)d + offset, (unsigned short)65535, delta))
                        return true;
                    break;
                case(GL_INT):
                    if (_findLowerAlphaValueInRow(s(), (int*)d + offset, (int)2147483647, delta))
                        return true;
                    break;
                case(GL_UNSIGNED_INT):
                    if (_findLowerAlphaValueInRow(s(), (unsigned int*)d + offset, 4294967295u, delta))
                        return true;
                    break;
                case(GL_FLOAT):
                    if (_findLowerAlphaValueInRow(s(), (float*)d + offset, 1.0f, delta))
                        return true;
                    break;
                case(GL_UNSIGNED_SHORT_5_5_5_1):
                    if (_maskedFindLowerAlphaValueInRow(s(), (unsigned short*)d,
                                                        (unsigned short)0x0001,
                                                        (unsigned short)0x0001, 1))
                        return true;
                    break;
                case(GL_UNSIGNED_SHORT_1_5_5_5_REV):
                    if (_maskedFindLowerAlphaValueInRow(s(), (unsigned short*)d,
                                                        (unsigned short)0x8000,
                                                        (unsigned short)0x8000, 1))
                        return true;
                    break;
                case(GL_UNSIGNED_SHORT_4_4_4_4):
                    if (_maskedFindLowerAlphaValueInRow(s(), (unsigned short*)d,
                                                        (unsigned short)0x000f,
                                                        (unsigned short)0x000f, 1))
                        return true;
                    break;
                case(GL_UNSIGNED_SHORT_4_4_4_4_REV):
                    if (_maskedFindLowerAlphaValueInRow(s(), (unsigned short*)d,
                                                        (unsigned short)0xf000,
                                                        (unsigned short)0xf000, 1))
                        return true;
                    break;
                case(GL_UNSIGNED_INT_10_10_10_2):
                    if (_maskedFindLowerAlphaValueInRow(s(), (unsigned int*)d,
                                                        0x00000003u,
                                                        0x00000003u, 1))
                        return true;
                    break;                    
                case(GL_UNSIGNED_INT_2_10_10_10_REV):
                    if (_maskedFindLowerAlphaValueInRow(s(), (unsigned int*)d,
                                                        0xc0000000u,
                                                        0xc0000000u, 1))
                        return true;
                    break;
                case(GL_HALF_FLOAT_NV):
                    if (_findLowerAlphaValueInRow(s(), (unsigned short*)d + offset,
                                                  (unsigned short)0x3c00, delta))
                        return true;
                    break;
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////


Geode* osg::createGeodeForImage(osg::Image* image)
{
    return createGeodeForImage(image,image->s(),image->t());
}


#include <osg/TextureRectangle> 


Geode* osg::createGeodeForImage(osg::Image* image,float s,float t)
{
    if (image)
    {
        if (s>0 && t>0)
        {

            float y = 1.0;
            float x = y*(s/t);

            float texcoord_y_b = (image->getOrigin() == osg::Image::BOTTOM_LEFT) ? 0.0f : 1.0f;
            float texcoord_y_t = (image->getOrigin() == osg::Image::BOTTOM_LEFT) ? 1.0f : 0.0f;

            // set up the texture.

#if 0
            osg::TextureRectangle* texture = new osg::TextureRectangle;
            texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
            texture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
            //texture->setResizeNonPowerOfTwoHint(false);
            float texcoord_x = image->s();
            texcoord_y_b *= image->t();
            texcoord_y_t *= image->t();
#else
            osg::Texture2D* texture = new osg::Texture2D;
            texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
            texture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
            texture->setResizeNonPowerOfTwoHint(false);
            float texcoord_x = 1.0f;
#endif
            texture->setImage(image);

            // set up the drawstate.
            osg::StateSet* dstate = new osg::StateSet;
            dstate->setMode(GL_CULL_FACE,osg::StateAttribute::OFF);
            dstate->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
            dstate->setTextureAttributeAndModes(0, texture,osg::StateAttribute::ON);

            // set up the geoset.
            Geometry* geom = new Geometry;
            geom->setStateSet(dstate);

            Vec3Array* coords = new Vec3Array(4);
            (*coords)[0].set(-x,0.0f,y);
            (*coords)[1].set(-x,0.0f,-y);
            (*coords)[2].set(x,0.0f,-y);
            (*coords)[3].set(x,0.0f,y);
            geom->setVertexArray(coords);

            Vec2Array* tcoords = new Vec2Array(4);
            (*tcoords)[0].set(0.0f*texcoord_x,texcoord_y_t);
            (*tcoords)[1].set(0.0f*texcoord_x,texcoord_y_b);
            (*tcoords)[2].set(1.0f*texcoord_x,texcoord_y_b);
            (*tcoords)[3].set(1.0f*texcoord_x,texcoord_y_t);
            geom->setTexCoordArray(0,tcoords);

            osg::Vec4Array* colours = new osg::Vec4Array(1);
            (*colours)[0].set(1.0f,1.0f,1.0,1.0f);
            geom->setColorArray(colours);
            geom->setColorBinding(Geometry::BIND_OVERALL);

            geom->addPrimitiveSet(new DrawArrays(PrimitiveSet::QUADS,0,4));

            // set up the geode.
            osg::Geode* geode = new osg::Geode;
            geode->addDrawable(geom);

            return geode;

        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}


Image *osg::RGB888_2_RGB332(const Image *pImage)
{
    if(!pImage) return NULL;

    if(pImage->getPixelSizeInBits() != 24u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGB) && (ePixelFormat != GL_BGR))  return NULL;

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), ePixelFormat, GL_UNSIGNED_BYTE_3_3_2);

    std::vector<osg::Vec4f>  vecBias_LastRow, vecBias_NextRow;
    vecBias_LastRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    vecBias_NextRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            osg::Vec4f bias_LastCol(0.0f, 0.0f, 0.0f, 0.0f);
            const bool bNotMaxT = (t + 1 < pImage->t());

            const unsigned char *pRowData0 = pImage->data(0, t, r);
            unsigned char *pRowData1 = pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                float red0   = pRowData0[0];
                float green0 = pRowData0[1];
                float blue0  = pRowData0[2];
                if(ePixelFormat == GL_BGR)
                {
                    blue0  = pRowData0[0];
                    green0 = pRowData0[1];
                    red0   = pRowData0[2];
                }

                const osg::Vec4f &biasLastLine = vecBias_LastRow[s];
                red0   -= biasLastLine.r() + bias_LastCol.r();
                green0 -= biasLastLine.g() + bias_LastCol.g();
                blue0  -= biasLastLine.b() + bias_LastCol.b();

                const unsigned red1   = osg::clampBetween((unsigned)((red0   * 7.0f) / 255.0f + 0.5f), 0u, 7u);
                const unsigned green1 = osg::clampBetween((unsigned)((green0 * 7.0f) / 255.0f + 0.5f), 0u, 7u);
                const unsigned blue1  = osg::clampBetween((unsigned)((blue0  * 3.0f) / 255.0f + 0.5f), 0u, 3u);

                const unsigned color = ((red1 << 5u) | (green1 << 2u) | (blue1));
                *pRowData1 = (unsigned char)color;

                pRowData0 += 3u;
                pRowData1++;

                const osg::Vec4f bias
                (
                    (red1   * 255.0f / 7.0f) - red0,
                    (green1 * 255.0f / 7.0f) - green0,
                    (blue1  * 255.0f / 3.0f) - blue0,
                    1.0f
                );

                const bool bNotMaxS = (s + 1 < pImage->s());
                if(bNotMaxS)
                {
                    bias_LastCol.r() = bias.r() * 0.375f;
                    bias_LastCol.g() = bias.g() * 0.375f;
                    bias_LastCol.b() = bias.b() * 0.375f;
                }

                if(bNotMaxT)
                {
                    osg::Vec4f &biasBottom = vecBias_NextRow[s];
                    biasBottom.r() += bias.r() * 0.375f;
                    biasBottom.g() += bias.g() * 0.375f;
                    biasBottom.b() += bias.b() * 0.375f;
                }

                if(bNotMaxT && bNotMaxS)
                {
                    osg::Vec4f &biasRightBottom = vecBias_NextRow[s + 1];
                    biasRightBottom.r() += bias.r() * 0.25f;
                    biasRightBottom.g() += bias.g() * 0.25f;
                    biasRightBottom.b() += bias.b() * 0.25f;
                }
            }

            vecBias_LastRow.swap(vecBias_NextRow);
            vecBias_NextRow.assign(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }
    return pResultImage;
}


Image *osg::RGB888_2_RGB565(const Image *pImage)
{
    if(!pImage) return NULL;

    if(pImage->getPixelSizeInBits() != 24u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGB) && (ePixelFormat != GL_BGR))  return NULL;

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), ePixelFormat, GL_UNSIGNED_SHORT_5_6_5);

    std::vector<osg::Vec4f>  vecBias_LastRow, vecBias_NextRow;
    vecBias_LastRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    vecBias_NextRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            osg::Vec4f bias_LastCol(0.0f, 0.0f, 0.0f, 0.0f);
            const bool bNotMaxT = (t + 1 < pImage->t());

            const unsigned char *pRowData0 = pImage->data(0, t, r);
            unsigned short *pRowData1 = (unsigned short *)pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                float red0   = pRowData0[0];
                float green0 = pRowData0[1];
                float blue0  = pRowData0[2];
                if(ePixelFormat == GL_BGR)
                {
                    blue0  = pRowData0[0];
                    green0 = pRowData0[1];
                    red0   = pRowData0[2];
                }

                const osg::Vec4f &biasLastLine = vecBias_LastRow[s];
                red0   -= biasLastLine.r() + bias_LastCol.r();
                green0 -= biasLastLine.g() + bias_LastCol.g();
                blue0  -= biasLastLine.b() + bias_LastCol.b();

                const unsigned red1   = osg::clampBetween((unsigned)((red0   * 31.0f) / 255.0f + 0.5f), 0u, 31u);
                const unsigned green1 = osg::clampBetween((unsigned)((green0 * 63.0f) / 255.0f + 0.5f), 0u, 63u);
                const unsigned blue1  = osg::clampBetween((unsigned)((blue0  * 31.0f) / 255.0f + 0.5f), 0u, 31u);

                const unsigned color = ((red1 << 11u) | (green1 << 5u) | (blue1));
                *pRowData1 = (unsigned short)color;

                pRowData0 += 3u;
                pRowData1++;

                const osg::Vec4f bias
                (
                    (red1   * 255.0f / 31.0f) - red0,
                    (green1 * 255.0f / 63.0f) - green0,
                    (blue1  * 255.0f / 31.0f) - blue0,
                    1.0f
                );

                const bool bNotMaxS = (s + 1 < pImage->s());
                if(bNotMaxS)
                {
                    bias_LastCol.r() = bias.r() * 0.375f;
                    bias_LastCol.g() = bias.g() * 0.375f;
                    bias_LastCol.b() = bias.b() * 0.375f;
                }

                if(bNotMaxT)
                {
                    osg::Vec4f &biasBottom = vecBias_NextRow[s];
                    biasBottom.r() += bias.r() * 0.375f;
                    biasBottom.g() += bias.g() * 0.375f;
                    biasBottom.b() += bias.b() * 0.375f;
                }

                if(bNotMaxT && bNotMaxS)
                {
                    osg::Vec4f &biasRightBottom = vecBias_NextRow[s + 1];
                    biasRightBottom.r() += bias.r() * 0.25f;
                    biasRightBottom.g() += bias.g() * 0.25f;
                    biasRightBottom.b() += bias.b() * 0.25f;
                }
            }

            vecBias_LastRow.swap(vecBias_NextRow);
            vecBias_NextRow.assign(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }
    return pResultImage;
}


Image *osg::RGBA8888_2_RGBA5551(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 32u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGBA) && (ePixelFormat != GL_BGRA))  return NULL;

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), ePixelFormat, GL_UNSIGNED_SHORT_5_5_5_1);

    std::vector<osg::Vec4f>  vecBias_LastRow, vecBias_NextRow;
    vecBias_LastRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    vecBias_NextRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            osg::Vec4f bias_LastCol(0.0f, 0.0f, 0.0f, 0.0f);
            const bool bNotMaxT = (t + 1 < pImage->t());

            const unsigned char *pRowData0 = pImage->data(0, t, r);
            unsigned short *pRowData1 = (unsigned short *)pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                float red0   = pRowData0[0];
                float green0 = pRowData0[1];
                float blue0  = pRowData0[2];
                float alpha0 = pRowData0[3];
                if(ePixelFormat == GL_BGRA)
                {
                    blue0  = pRowData0[0];
                    green0 = pRowData0[1];
                    red0   = pRowData0[2];
                    alpha0 = pRowData0[3];
                }

                const osg::Vec4f &biasLastLine = vecBias_LastRow[s];
                red0   -= biasLastLine.r() + bias_LastCol.r();
                green0 -= biasLastLine.g() + bias_LastCol.g();
                blue0  -= biasLastLine.b() + bias_LastCol.b();
                alpha0 -= biasLastLine.a() + bias_LastCol.a();

                const unsigned red1   = osg::clampBetween((unsigned)((red0   * 31.0f) / 255.0f + 0.5f), 0u, 31u);
                const unsigned green1 = osg::clampBetween((unsigned)((green0 * 31.0f) / 255.0f + 0.5f), 0u, 31u);
                const unsigned blue1  = osg::clampBetween((unsigned)((blue0  * 31.0f) / 255.0f + 0.5f), 0u, 31u);
                const unsigned alpha1 = (unsigned)(alpha0 > 127.5f);

                const unsigned color = ((red1 << 11u) | (green1 << 6u) | (blue1 << 1u) | (alpha1));
                *pRowData1 = (unsigned short)color;

                pRowData0 += 4u;
                pRowData1++;

                const osg::Vec4f bias
                (
                    (red1   * 255.0f / 31.0f) - red0,
                    (green1 * 255.0f / 31.0f) - green0,
                    (blue1  * 255.0f / 31.0f) - blue0,
                    (alpha1 * 255.0f /  1.0f) - alpha0
                );

                const bool bNotMaxS = (s + 1 < pImage->s());
                if(bNotMaxS)
                {
                    bias_LastCol.r() = bias.r() * 0.375f;
                    bias_LastCol.g() = bias.g() * 0.375f;
                    bias_LastCol.b() = bias.b() * 0.375f;
                    bias_LastCol.a() = bias.a() * 0.375f;
                }

                if(bNotMaxT)
                {
                    osg::Vec4f &biasBottom = vecBias_NextRow[s];
                    biasBottom.r() += bias.r() * 0.375f;
                    biasBottom.g() += bias.g() * 0.375f;
                    biasBottom.b() += bias.b() * 0.375f;
                    biasBottom.a() += bias.a() * 0.375f;
                }

                if(bNotMaxT && bNotMaxS)
                {
                    osg::Vec4f &biasRightBottom = vecBias_NextRow[s + 1];
                    biasRightBottom.r() += bias.r() * 0.25f;
                    biasRightBottom.g() += bias.g() * 0.25f;
                    biasRightBottom.b() += bias.b() * 0.25f;
                    biasRightBottom.a() += bias.a() * 0.25f;
                }
            }

            vecBias_LastRow.swap(vecBias_NextRow);
            vecBias_NextRow.assign(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }

    return pResultImage;
}


Image *osg::RGBA8888_2_RGBA4444(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 32u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGBA) && (ePixelFormat != GL_BGRA))  return NULL;

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), ePixelFormat, GL_UNSIGNED_SHORT_4_4_4_4);

    std::vector<osg::Vec4f>  vecBias_LastRow, vecBias_NextRow;
    vecBias_LastRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    vecBias_NextRow.resize(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            osg::Vec4f bias_LastCol(0.0f, 0.0f, 0.0f, 0.0f);
            const bool bNotMaxT = (t + 1 < pImage->t());

            const unsigned char *pRowData0 = pImage->data(0, t, r);
            unsigned short *pRowData1 = (unsigned short *)pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                float red0   = pRowData0[0];
                float green0 = pRowData0[1];
                float blue0  = pRowData0[2];
                float alpha0 = pRowData0[3];
                if(ePixelFormat == GL_BGRA)
                {
                    blue0  = pRowData0[0];
                    green0 = pRowData0[1];
                    red0   = pRowData0[2];
                    alpha0 = pRowData0[3];
                }

                const osg::Vec4f &biasLastLine = vecBias_LastRow[s];
                red0   -= biasLastLine.r() + bias_LastCol.r();
                green0 -= biasLastLine.g() + bias_LastCol.g();
                blue0  -= biasLastLine.b() + bias_LastCol.b();
                alpha0 -= biasLastLine.a() + bias_LastCol.a();

                const unsigned red1   = osg::clampBetween((unsigned)((red0   * 15.0f) / 255.0f + 0.5f), 0u, 15u);
                const unsigned green1 = osg::clampBetween((unsigned)((green0 * 15.0f) / 255.0f + 0.5f), 0u, 15u);
                const unsigned blue1  = osg::clampBetween((unsigned)((blue0  * 15.0f) / 255.0f + 0.5f), 0u, 15u);
                const unsigned alpha1 = osg::clampBetween((unsigned)((alpha0 * 15.0f) / 255.0f + 0.5f), 0u, 15u);

                const unsigned color = ((red1 << 12u) | (green1 << 8u) | (blue1 << 4u) | (alpha1));
                *pRowData1 = (unsigned short)color;

                pRowData0 += 4u;
                pRowData1++;

                const osg::Vec4f bias
                (
                    (red1   * 255.0f / 15.0f) - red0,
                    (green1 * 255.0f / 15.0f) - green0,
                    (blue1  * 255.0f / 15.0f) - blue0,
                    (alpha1 * 255.0f / 15.0f) - alpha0
                );

                const bool bNotMaxS = (s + 1 < pImage->s());
                if(bNotMaxS)
                {
                    bias_LastCol.r() = bias.r() * 0.375f;
                    bias_LastCol.g() = bias.g() * 0.375f;
                    bias_LastCol.b() = bias.b() * 0.375f;
                    bias_LastCol.a() = bias.a() * 0.375f;
                }

                if(bNotMaxT)
                {
                    osg::Vec4f &biasBottom = vecBias_NextRow[s];
                    biasBottom.r() += bias.r() * 0.375f;
                    biasBottom.g() += bias.g() * 0.375f;
                    biasBottom.b() += bias.b() * 0.375f;
                    biasBottom.a() += bias.a() * 0.375f;
                }

                if(bNotMaxT && bNotMaxS)
                {
                    osg::Vec4f &biasRightBottom = vecBias_NextRow[s + 1];
                    biasRightBottom.r() += bias.r() * 0.25f;
                    biasRightBottom.g() += bias.g() * 0.25f;
                    biasRightBottom.b() += bias.b() * 0.25f;
                    biasRightBottom.a() += bias.a() * 0.25f;
                }
            }

            vecBias_LastRow.swap(vecBias_NextRow);
            vecBias_NextRow.assign(pImage->s(), osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }

    return pResultImage;
}


Image *osg::RGBA8888_2_RGB888(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 32u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGBA) && (ePixelFormat != GL_BGRA))  return NULL;

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), GL_RGB, GL_UNSIGNED_BYTE);

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            const unsigned char *pRowData0 = pImage->data(0, t, r);
            unsigned char *pRowData1 = pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                if(ePixelFormat == GL_RGBA)
                {
                    pRowData1[0] = pRowData0[0];
                    pRowData1[1] = pRowData0[1];
                    pRowData1[2] = pRowData0[2];
                }
                else
                {
                    pRowData1[2] = pRowData0[0];
                    pRowData1[1] = pRowData0[1];
                    pRowData1[0] = pRowData0[2];
                }

                pRowData0 += 4u;
                pRowData1 += 3u;
            }
        }
    }

    return pResultImage;
}


Image *osg::RGB888_2_RGBA8888(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 24u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGB) && (ePixelFormat != GL_BGR))    return NULL;

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), GL_RGBA, GL_UNSIGNED_BYTE);

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            const unsigned char *pRowData0 = pImage->data(0, t, r);
            unsigned char *pRowData1 = pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                if(ePixelFormat == GL_RGB)
                {
                    pRowData1[0] = pRowData0[0];
                    pRowData1[1] = pRowData0[1];
                    pRowData1[2] = pRowData0[2];
                    pRowData1[3] = 255u;
                }
                else
                {
                    pRowData1[3] = 255u;
                    pRowData1[2] = pRowData0[0];
                    pRowData1[1] = pRowData0[1];
                    pRowData1[0] = pRowData0[2];
                }

                pRowData0 += 3u;
                pRowData1 += 4u;
            }
        }
    }

    return pResultImage;
}


Image *osg::RGBA5551_2_RGBA8888(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 16u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGBA) && (ePixelFormat != GL_BGRA))  return NULL;

    const GLenum eDataType = pImage->getDataType();
    if((eDataType != GL_UNSIGNED_SHORT_5_5_5_1) && (eDataType != GL_UNSIGNED_SHORT_1_5_5_5_REV))
    {
        return NULL;
    }

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), GL_RGBA, GL_UNSIGNED_BYTE);

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            const unsigned short *pRowData0 = (unsigned short *)pImage->data(0, t, r);
            unsigned char *pRowData1 = pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                const unsigned nColor = *pRowData0;
                unsigned red0   = 0u;
                unsigned green0 = 0u;
                unsigned blue0  = 0u;
                unsigned alpha0 = 0u;
                if(ePixelFormat == GL_RGBA)
                {
                    red0   = (nColor >> 11u) & 0x1F;
                    green0 = (nColor >>  6u) & 0x1F;
                    blue0  = (nColor >>  1u) & 0x1F;
                    alpha0 = (nColor       ) & 0x01;
                }
                else
                {
                    alpha0 = (nColor >> 15u) & 0x01;
                    blue0  = (nColor >> 10u) & 0x1F;
                    green0 = (nColor >>  5u) & 0x1F;
                    red0   = (nColor       ) & 0x1F;
                }

                const unsigned red1   = osg::clampBetween((unsigned)(red0   * 255.0f / 31.0f), 0u, 255u);
                const unsigned green1 = osg::clampBetween((unsigned)(green0 * 255.0f / 31.0f), 0u, 255u);
                const unsigned blue1  = osg::clampBetween((unsigned)(blue0  * 255.0f / 31.0f), 0u, 255u);
                const unsigned alpha1 = (alpha0 == 0u ? 0u : 255u);

                if(eDataType == GL_UNSIGNED_SHORT_5_5_5_1)
                {
                    pRowData1[0] = red1;
                    pRowData1[1] = green1;
                    pRowData1[2] = blue1;
                    pRowData1[3] = alpha1;
                }
                else
                {
                    pRowData1[0] = alpha1;
                    pRowData1[1] = blue1;
                    pRowData1[2] = green1;
                    pRowData1[3] = red1;
                }

                pRowData1 += 4u;
                pRowData0++;
            }
        }
    }

    return pResultImage;
}


Image *osg::RGBA4444_2_RGBA8888(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 16u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGBA) && (ePixelFormat != GL_BGRA))  return NULL;

    const GLenum eDataType = pImage->getDataType();
    if((eDataType != GL_UNSIGNED_SHORT_4_4_4_4) && (eDataType != GL_UNSIGNED_SHORT_4_4_4_4_REV))
    {
        return NULL;
    }

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), GL_RGBA, GL_UNSIGNED_BYTE);

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            const unsigned short *pRowData0 = (unsigned short *)pImage->data(0, t, r);
            unsigned char *pRowData1 = pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                const unsigned nColor = *pRowData0;
                unsigned red0   = 0u;
                unsigned green0 = 0u;
                unsigned blue0  = 0u;
                unsigned alpha0 = 0u;
                if(ePixelFormat == GL_RGBA)
                {
                    red0   = (nColor >> 12u) & 0x0F;
                    green0 = (nColor >>  8u) & 0x0F;
                    blue0  = (nColor >>  4u) & 0x0F;
                    alpha0 = (nColor       ) & 0x0F;
                }
                else
                {
                    alpha0 = (nColor >> 12u) & 0x0F;
                    blue0  = (nColor >>  8u) & 0x0F;
                    green0 = (nColor >>  4u) & 0x0F;
                    red0   = (nColor       ) & 0x0F;
                }

                const unsigned red1   = osg::clampBetween((unsigned)(red0   * 255.0f / 15.0f), 0u, 255u);
                const unsigned green1 = osg::clampBetween((unsigned)(green0 * 255.0f / 15.0f), 0u, 255u);
                const unsigned blue1  = osg::clampBetween((unsigned)(blue0  * 255.0f / 15.0f), 0u, 255u);
                const unsigned alpha1 = osg::clampBetween((unsigned)(alpha0 * 255.0f / 15.0f), 0u, 255u);

                if(eDataType == GL_UNSIGNED_SHORT_4_4_4_4)
                {
                    pRowData1[0] = red1;
                    pRowData1[1] = green1;
                    pRowData1[2] = blue1;
                    pRowData1[3] = alpha1;
                }
                else
                {
                    pRowData1[0] = alpha1;
                    pRowData1[1] = blue1;
                    pRowData1[2] = green1;
                    pRowData1[3] = red1;
                }

                pRowData1 += 4u;
                pRowData0++;
            }
        }
    }

    return pResultImage;
}


Image *osg::RGB332_2_RGB888(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 8u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGB) && (ePixelFormat != GL_BGR))    return NULL;

    const GLenum eDataType = pImage->getDataType();
    if((eDataType != GL_UNSIGNED_BYTE_3_3_2) && (eDataType != GL_UNSIGNED_BYTE_2_3_3_REV))
    {
        return NULL;
    }

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), GL_RGB, GL_UNSIGNED_BYTE);

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            const unsigned char *pRowData0 = pImage->data(0, t, r);
            unsigned char *pRowData1 = pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                const unsigned nColor = *pRowData0;

                unsigned red0   = 0u;
                unsigned green0 = 0u;
                unsigned blue0  = 0u;
                if(ePixelFormat == GL_RGB)
                {
                    red0   = (nColor >> 5u) & 0x07;
                    green0 = (nColor >> 2u) & 0x07;
                    blue0  = (nColor      ) & 0x03;
                }
                else
                {
                    blue0  = (nColor >> 5u) & 0x07;
                    green0 = (nColor >> 2u) & 0x07;
                    red0   = (nColor      ) & 0x03;
                }

                const unsigned red1   = osg::clampBetween((unsigned)(red0   * 255.0f / 7.0f), 0u, 255u);
                const unsigned green1 = osg::clampBetween((unsigned)(green0 * 255.0f / 7.0f), 0u, 255u);
                const unsigned blue1  = osg::clampBetween((unsigned)(blue0  * 255.0f / 3.0f), 0u, 255u);

                if(eDataType == GL_UNSIGNED_BYTE_3_3_2)
                {
                    pRowData1[0] = red1;
                    pRowData1[1] = green1;
                    pRowData1[2] = blue1;
                }
                else
                {
                    pRowData1[0] = blue1;
                    pRowData1[1] = green1;
                    pRowData1[2] = red1;
                }

                pRowData1[0] = red1;
                pRowData1[1] = green1;
                pRowData1[2] = blue1;

                pRowData1 += 3u;
                pRowData0++;
            }
        }
    }

    return pResultImage;
}


Image *osg::RGB565_2_RGB888(const Image *pImage)
{
    if(!pImage) return NULL;
    if(pImage->getPixelSizeInBits() != 16u) return NULL;

    const GLenum ePixelFormat = pImage->getPixelFormat();
    if((ePixelFormat != GL_RGB) && (ePixelFormat != GL_BGR))    return NULL;

    const GLenum eDataType = pImage->getDataType();
    if((eDataType != GL_UNSIGNED_SHORT_5_6_5) && (eDataType != GL_UNSIGNED_SHORT_5_6_5_REV))
    {
        return NULL;
    }

    Image *pResultImage = dynamic_cast<Image*>(pImage->clone(CopyOp::DEEP_COPY_ALL));
    pResultImage->allocateImage(pImage->s(), pImage->t(), pImage->r(), GL_RGB, GL_UNSIGNED_BYTE);

    for(int r = 0; r < pImage->r(); r++)
    {
        for(int t = 0; t < pImage->t(); t++)
        {
            const unsigned short *pRowData0 = (unsigned short *)pImage->data(0, t, r);
            unsigned char *pRowData1 = pResultImage->data(0, t, r);
            for(int s = 0; s < pImage->s(); s++)
            {
                const unsigned nColor = *pRowData0;

                unsigned red0   = 0u;
                unsigned green0 = 0u;
                unsigned blue0  = 0u;
                if(ePixelFormat == GL_RGB)
                {
                    red0   = (nColor >> 11u) & 0x1F;
                    green0 = (nColor >>  5u) & 0x3F;
                    blue0  = (nColor       ) & 0x1F;
                }
                else
                {
                    blue0  = (nColor >> 11u) & 0x1F;
                    green0 = (nColor >>  5u) & 0x3F;
                    red0   = (nColor       ) & 0x1F;
                }

                const unsigned red1   = osg::clampBetween((unsigned)(red0   * 255.0f / 31.0f), 0u, 255u);
                const unsigned green1 = osg::clampBetween((unsigned)(green0 * 255.0f / 63.0f), 0u, 255u);
                const unsigned blue1  = osg::clampBetween((unsigned)(blue0  * 255.0f / 31.0f), 0u, 255u);

                if(eDataType == GL_UNSIGNED_SHORT_5_6_5)
                {
                    pRowData1[0] = red1;
                    pRowData1[1] = green1;
                    pRowData1[2] = blue1;
                }
                else
                {
                    pRowData1[0] = blue1;
                    pRowData1[1] = green1;
                    pRowData1[2] = red1;
                }

                pRowData1 += 3u;
                pRowData0++;
            }
        }
    }

    return pResultImage;
}


template <typename T>    
Vec4 _readColor(GLenum pixelFormat, T* data,float scale)
{
    switch(pixelFormat)
    {
        case(GL_DEPTH_COMPONENT):   //intentionally fall through and execute the code for GL_LUMINANCE
        case(GL_LUMINANCE):         { float l = float(*data++)*scale; return Vec4(l, l, l, 1.0f); }
        case(GL_ALPHA):             { float a = float(*data++)*scale; return Vec4(1.0f, 1.0f, 1.0f, a); }
        case(GL_LUMINANCE_ALPHA):   { float l = float(*data++)*scale; float a = float(*data++)*scale; return Vec4(l,l,l,a); }
        case(GL_RGB):               { float r = float(*data++)*scale; float g = float(*data++)*scale; float b = float(*data++)*scale; return Vec4(r,g,b,1.0f); }
        case(GL_RGBA):              { float r = float(*data++)*scale; float g = float(*data++)*scale; float b = float(*data++)*scale; float a = float(*data++)*scale; return Vec4(r,g,b,a); }
        case(GL_BGR):               { float b = float(*data++)*scale; float g = float(*data++)*scale; float r = float(*data++)*scale; return Vec4(r,g,b,1.0f); }
        case(GL_BGRA):              { float b = float(*data++)*scale; float g = float(*data++)*scale; float r = float(*data++)*scale; float a = float(*data++)*scale; return Vec4(r,g,b,a); }
    }
    return Vec4(1.0f,1.0f,1.0f,1.0f);
}

Vec4 Image::getColor(unsigned int s,unsigned t,unsigned r) const
{
    const unsigned char* ptr = data(s,t,r);

    switch(_dataType)
    {
        case(GL_BYTE):              return _readColor(_pixelFormat, (char*)ptr,             1.0f/128.0f);
        case(GL_UNSIGNED_BYTE):     return _readColor(_pixelFormat, (unsigned char*)ptr,    1.0f/255.0f);
        case(GL_SHORT):             return _readColor(_pixelFormat, (short*)ptr,            1.0f/32768.0f);
        case(GL_UNSIGNED_SHORT):    return _readColor(_pixelFormat, (unsigned short*)ptr,   1.0f/65535.0f);
        case(GL_INT):               return _readColor(_pixelFormat, (int*)ptr,              1.0f/2147483648.0f);
        case(GL_UNSIGNED_INT):      return _readColor(_pixelFormat, (unsigned int*)ptr,     1.0f/4294967295.0f);
        case(GL_FLOAT):             return _readColor(_pixelFormat, (float*)ptr,            1.0f);
    }
    return Vec4(1.0f,1.0f,1.0f,1.0f);
}

Vec4 Image::getColor(const Vec3& texcoord) const
{
    int s = int(texcoord.x()*float(_s-1)) % _s;
    int t = int(texcoord.y()*float(_t-1)) % _t;
    int r = int(texcoord.z()*float(_r-1)) % _r;
    //OSG_NOTICE<<"getColor("<<texcoord<<")="<<getColor(s,t,r)<<std::endl;
    return getColor(s,t,r);
}