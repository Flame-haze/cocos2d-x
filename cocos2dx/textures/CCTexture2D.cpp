/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/



/*
* Support for RGBA_4_4_4_4 and RGBA_5_5_5_1 was copied from:
* https://devforums.apple.com/message/37855#37855 by a1studmuffin
*/

#include "CCTexture2D.h"

#include "ccConfig.h"
#include "ccMacros.h"
#include "CCTexture2D.h"
#include "CCConfiguration.h"
#include "platform/platform.h"
#include "ccxImage.h"
#include "CCGL.h"
#include "support/ccUtils.h"
#include "platform/CCPlatformMacros.h"

#ifdef _POWERVR_SUPPORT_
    #include "CCPVRTexture.h"
#endif

#if CC_ENABLE_CACHE_TEXTTURE_DATA
    #include <list>
#endif

namespace   cocos2d {

#if CC_FONT_LABEL_SUPPORT
// FontLabel support
#endif// CC_FONT_LABEL_SUPPORT

//CLASS IMPLEMENTATIONS:

#if CC_ENABLE_CACHE_TEXTTURE_DATA
    class VolatileTexture
    {
    protected:
        CCTexture2D *texture;
        unsigned char *data;
        CCTexture2DPixelFormat pixelFormat;
        unsigned int pixelsWide;
        unsigned int pixelsHigh;
        CGSize contentSize;

    public:

        static std::list<VolatileTexture*> textures;
        static bool isReloading;

        VolatileTexture(CCTexture2D *t) : texture(t), data(0)
        {
            textures.push_back(this);
        }

        ~VolatileTexture()
        {
            if (data)
                delete [] data;
            textures.remove(this);
        }

        static void addTextureWithData(CCTexture2D *tt, 
            const void *d, 
            CCTexture2DPixelFormat f, 
            unsigned int w, 
            unsigned int h, 
            CGSize s)
        {
            if (isReloading)
                return;

            VolatileTexture *vt = 0;
            std::list<VolatileTexture *>::iterator i = textures.begin();
            while( i != textures.end() )
            {
                VolatileTexture *v = *i++;
                if (v->texture == tt) {
                    vt = v;
                    break;
                }
            }

            if (!vt)
                vt = new VolatileTexture(tt);

            vt->pixelFormat = f;
            vt->pixelsWide = w;
            vt->pixelsHigh = h;
            vt->contentSize = s;

            //CCLOGINFO("added volatile %d", textures.size());

            if (vt->data) {
                delete [] vt->data;
                vt->data = 0;   
            }

            switch(f) {          
    case kCCTexture2DPixelFormat_RGBA8888:
    case kCCTexture2DPixelFormat_RGBA4444:
    case kCCTexture2DPixelFormat_RGB5A1:
    case kCCTexture2DPixelFormat_RGB565:
    case kCCTexture2DPixelFormat_A8:
        vt->data = new ccxByte[w * h * 4];
        memcpy(vt->data, d, w * h * 4);
        break;    
    case kCCTexture2DPixelFormat_RGB888:
        vt->data = new ccxByte[w * h * 3];
        memcpy(vt->data, d, w * h * 3);
        break;
            }
        }

        static void removeTexture(CCTexture2D *t) {

            std::list<VolatileTexture *>::iterator i = textures.begin();
            while( i != textures.end() )
            {
                VolatileTexture *vt = *i++;
                if (vt->texture == t) {
                    delete vt;
                    break;
                }
            }
        }

        static void reloadAllTextures()
        {
            isReloading = true;

            CCLOG("reload all texture");
            std::list<VolatileTexture *>::iterator i = textures.begin();

            while( i != textures.end() )
            {
                VolatileTexture *vt = *i++;
                if (vt->data) {
                    vt->texture->initWithData((const void *)vt->data, vt->pixelFormat, vt->pixelsWide, vt->pixelsHigh, vt->contentSize);
                }
            }

            isReloading = false;
        }
    };

    std::list<VolatileTexture*> VolatileTexture::textures;
    bool VolatileTexture::isReloading = false;
#endif // CC_ENABLE_CACHE_TEXTTURE_DATA

// If the image has alpha, you can create RGBA8 (32-bit) or RGBA4 (16-bit) or RGB5A1 (16-bit)
// Default is: RGBA8888 (32-bit textures)
static CCTexture2DPixelFormat g_defaultAlphaPixelFormat = kCCTexture2DPixelFormat_Default;

CCTexture2D::CCTexture2D()
{
    m_uName = 0;
}

CCTexture2D::~CCTexture2D()
{
#if CC_ENABLE_CACHE_TEXTTURE_DATA
    VolatileTexture::removeTexture(this);
#endif

	CCLOGINFO("cocos2d: deallocing CCTexture2D %u.", m_uName);
	if(m_uName)
	{
		glDeleteTextures(1, &m_uName);
	}
}

CCTexture2DPixelFormat CCTexture2D::getPixelFormat()
{
	return m_ePixelFormat;
}

unsigned int CCTexture2D::getPixelsWide()
{
	return m_uPixelsWide;
}

unsigned int CCTexture2D::getPixelsHigh()
{
	return m_uPixelsHigh;
}

GLuint CCTexture2D::getName()
{
	return m_uName;
}

CGSize CCTexture2D::getContentSizeInPixels()
{
	return m_tContentSize;
}

CGSize CCTexture2D::getContentSize()
{
	CGSize ret;
	ret.width = m_tContentSize.width / CC_CONTENT_SCALE_FACTOR();
	ret.height = m_tContentSize.height / CC_CONTENT_SCALE_FACTOR();

	return ret;
}

GLfloat CCTexture2D::getMaxS()
{
	return m_fMaxS;
}

void CCTexture2D::setMaxS(GLfloat maxS)
{
	m_fMaxS = maxS;
}

GLfloat CCTexture2D::getMaxT()
{
	return m_fMaxT;
}

void CCTexture2D::setMaxT(GLfloat maxT)
{
	m_fMaxT = maxT;
}

void CCTexture2D::releaseData(void *data)
{
    free(data);
}

void* CCTexture2D::keepData(void *data, unsigned int length)
{
	//The texture data mustn't be saved becuase it isn't a mutable texture.
	return data;
}

bool CCTexture2D::getHasPremultipliedAlpha()
{
	return m_bHasPremultipliedAlpha;
}

bool CCTexture2D::initWithData(const void *data, CCTexture2DPixelFormat pixelFormat, unsigned int pixelsWide, unsigned int pixelsHigh, CGSize contentSize)
{

#if CC_ENABLE_CACHE_TEXTTURE_DATA
    // cache the texture data
    VolatileTexture::addTextureWithData(this, data, pixelFormat, pixelsWide, pixelsHigh, contentSize);
#endif

	glGenTextures(1, &m_uName);
	glBindTexture(GL_TEXTURE_2D, m_uName);

	this->setAntiAliasTexParameters();

	// Specify OpenGL texture image

	switch(pixelFormat)
	{
	case kCCTexture2DPixelFormat_RGBA8888:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelsWide, pixelsHigh, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_RGB888:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pixelsWide, pixelsHigh, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		break;
	case kCCTexture2DPixelFormat_RGBA4444:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelsWide, pixelsHigh, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
		break;
	case kCCTexture2DPixelFormat_RGB5A1:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelsWide, pixelsHigh, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
		break;
	case kCCTexture2DPixelFormat_RGB565:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pixelsWide, pixelsHigh, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
		break;
	case kCCTexture2DPixelFormat_A8:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, pixelsWide, pixelsHigh, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		break;
	default:;
		NSAssert(0, "NSInternalInconsistencyException");

	}

	m_tContentSize = contentSize;
	m_uPixelsWide = pixelsWide;
	m_uPixelsHigh = pixelsHigh;
	m_ePixelFormat = pixelFormat;
	m_fMaxS = contentSize.width / (float)(pixelsWide);
	m_fMaxT = contentSize.height / (float)(pixelsHigh);

	m_bHasPremultipliedAlpha = false;

	return true;
}


char * CCTexture2D::description(void)
{
	char *ret = new char[100];
	sprintf(ret, "<CCTexture2D | Name = %u | Dimensions = %u x %u | Coordinates = (%.2f, %.2f)>", m_uName, m_uPixelsWide, m_uPixelsHigh, m_fMaxS, m_fMaxT);
	return ret;
}

// implementation CCTexture2D (Image)

bool CCTexture2D::initWithImage(ccxImage * uiImage)
{
	unsigned int POTWide, POTHigh;

	if(uiImage == NULL)
	{
		CCLOG("cocos2d: CCTexture2D. Can't create Texture. UIImage is nil");
		this->release();
		return false;
	}

	CCConfiguration *conf = CCConfiguration::sharedConfiguration();

#if CC_TEXTURE_NPOT_SUPPORT
	if( conf->isSupportsNPOT() ) 
	{
		POTWide = uiimage->getWidth();
		POTHigh = uiimage->getHeight();
	}
	else 
#endif
	{
		POTWide = ccNextPOT(uiImage->getWidth());
		POTHigh = ccNextPOT(uiImage->getHeight());
	}

	unsigned maxTextureSize = conf->getMaxTextureSize();
	if( POTHigh > maxTextureSize || POTWide > maxTextureSize ) 
	{
		CCLOG("cocos2d: WARNING: Image (%u x %u) is bigger than the supported %u x %u", POTWide, POTHigh, maxTextureSize, maxTextureSize);
		this->release();
		return NULL;
	}

	// always load premultiplied images
	return initPremultipliedATextureWithImage(uiImage, POTWide, POTHigh);
}
bool CCTexture2D::initPremultipliedATextureWithImage(ccxImage *image, unsigned int POTWide, unsigned int POTHigh)
{
	unsigned char*			data = NULL;
	unsigned char*			tempData =NULL;
	unsigned int*			inPixel32 = NULL;
	unsigned short*			outPixel16 = NULL;
	bool					hasAlpha;
	CGSize					imageSize;
	CCTexture2DPixelFormat	pixelFormat;

	hasAlpha = image->hasAlpha();

	size_t bpp = image->getBitsPerComponent();
	int colorSpace = image->getColorSpace();

	if(colorSpace)
	{
		if(hasAlpha)
		{
			pixelFormat = defaultAlphaPixelFormat();
		}
		else
		{
			if (bpp >= 8)
			{
				pixelFormat = kCCTexture2DPixelFormat_RGB888;
			}
			else
			{
				CCLOG("cocos2d: CCTexture2D: Using RGB565 texture since image has no alpha");
				pixelFormat = kCCTexture2DPixelFormat_RGB565;
			}
		}
	}
	else
	{
		// NOTE: No colorspace means a mask image
		CCLOG("cocos2d: CCTexture2D: Using A8 texture since image is a mask");
		pixelFormat = kCCTexture2DPixelFormat_A8;
	}

	imageSize = CGSizeMake((float)(image->getWidth()), (float)(image->getHeight()));

	// Create the bitmap graphics context

	switch(pixelFormat) {          
		case kCCTexture2DPixelFormat_RGBA8888:
		case kCCTexture2DPixelFormat_RGBA4444:
		case kCCTexture2DPixelFormat_RGB5A1:
//			colorSpace = CGColorSpaceCreateDeviceRGB();
//			data = malloc(POTHigh * POTWide * 4);
// 			info = hasAlpha ? kCGImageAlphaPremultipliedLast : kCGImageAlphaNoneSkipLast; 
// 			context = CGBitmapContextCreate(data, POTWide, POTHigh, 8, 4 * POTWide, colorSpace, info | kCGBitmapByteOrder32Big);				
// 			CGColorSpaceRelease(colorSpace);
//			break;
		case kCCTexture2DPixelFormat_RGB565:
//			colorSpace = CGColorSpaceCreateDeviceRGB();
//			data = malloc(POTHigh * POTWide * 4);
// 			info = kCGImageAlphaNoneSkipLast;
// 			context = CGBitmapContextCreate(data, POTWide, POTHigh, 8, 4 * POTWide, colorSpace, info | kCGBitmapByteOrder32Big);
// 			CGColorSpaceRelease(colorSpace);
//			break;
		case kCCTexture2DPixelFormat_A8:
//			data = malloc(POTHigh * POTWide);
// 			info = kCGImageAlphaOnly; 
// 			context = CGBitmapContextCreate(data, POTWide, POTHigh, 8, POTWide, NULL, info);

			tempData = (ccxByte*)(image->getData());
			NSAssert(tempData != NULL, "NULL image data.");

			if(image->getWidth() == POTWide && image->getHeight() == POTHigh)
			{
				data = new ccxByte[POTHigh * POTWide * 4];
				memcpy(data, tempData, POTHigh * POTWide * 4);
			}
			else
			{
				data = new ccxByte[POTHigh * POTWide * 4];
				memset(data, 0, POTHigh * POTWide * 4);

				ccxByte* pPixelData = (ccxByte*) tempData;
				ccxByte* pTargetData = (ccxByte*) data;

				for(int y=0; y<image->getHeight(); ++y)
				{
					memcpy(pTargetData+POTWide*4*y, pPixelData+(image->getWidth())*4*y, (image->getWidth())*4);
				}
			}

			break;    
		case kCCTexture2DPixelFormat_RGB888:
			tempData = (ccxByte*)(image->getData());
			NSAssert(tempData != NULL, "NULL image data.");
			if(image->getWidth() == POTWide && image->getHeight() == POTHigh)
			{
				data = new ccxByte[POTHigh * POTWide * 3];
				memcpy(data, tempData, POTHigh * POTWide * 3);
			}
			else
			{
				data = new ccxByte[POTHigh * POTWide * 3];
				memset(data, 0, POTHigh * POTWide * 3);

				ccxByte* pPixelData = (ccxByte*) tempData;
				ccxByte* pTargetData = (ccxByte*) data;

				for(int y=0; y<image->getHeight(); ++y)
				{
					memcpy(pTargetData+POTWide*3*y, pPixelData+(image->getWidth())*3*y, (image->getWidth())*3);
				}
			}
			break;   
		default:
			NSAssert(0, "Invalid pixel format");
			//[NSException raise:NSInternalInconsistencyException format:@"Invalid pixel format"];
	}


// 	CGContextClearRect(context, CGRectMake(0, 0, POTWide, POTHigh));
// 	CGContextTranslateCTM(context, 0, POTHigh - imageSize.height);
// 	CGContextDrawImage(context, CGRectMake(0, 0, CGImageGetWidth(image), CGImageGetHeight(image)), image);

	// Repack the pixel data into the right format

	if(pixelFormat == kCCTexture2DPixelFormat_RGB565) {
		//Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRRGGGGGGBBBBB"
		tempData = new unsigned char[POTHigh * POTWide * 2];
		inPixel32 = (unsigned int*)data;
		outPixel16 = (unsigned short*)tempData;

		for(unsigned int i = 0; i < POTWide * POTHigh; ++i, ++inPixel32)
			*outPixel16++ = ((((*inPixel32 >> 0) & 0xFF) >> 3) << 11) | ((((*inPixel32 >> 8) & 0xFF) >> 2) << 5) | ((((*inPixel32 >> 16) & 0xFF) >> 3) << 0);

		delete [] data;
		data = tempData;
	}
	else if (pixelFormat == kCCTexture2DPixelFormat_RGBA4444) {
		//Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRGGGGBBBBAAAA"
		tempData = new unsigned char[POTHigh * POTWide * 2];
		inPixel32 = (unsigned int*)data;
		outPixel16 = (unsigned short*)tempData;

		for(unsigned int i = 0; i < POTWide * POTHigh; ++i, ++inPixel32)
			*outPixel16++ = 
			((((*inPixel32 >> 0) & 0xFF) >> 4) << 12) | // R
			((((*inPixel32 >> 8) & 0xFF) >> 4) << 8) | // G
			((((*inPixel32 >> 16) & 0xFF) >> 4) << 4) | // B
			((((*inPixel32 >> 24) & 0xFF) >> 4) << 0); // A

		delete [] data;
		data = tempData;
	}
	else if (pixelFormat == kCCTexture2DPixelFormat_RGB5A1) {
		//Convert "RRRRRRRRRGGGGGGGGBBBBBBBBAAAAAAAA" to "RRRRRGGGGGBBBBBA"
		tempData = new unsigned char[POTHigh * POTWide * 2];
		inPixel32 = (unsigned int*)data;
		outPixel16 = (unsigned short*)tempData;

		for(unsigned int i = 0; i < POTWide * POTHigh; ++i, ++inPixel32)
			*outPixel16++ = 
			((((*inPixel32 >> 0) & 0xFF) >> 3) << 11) | // R
			((((*inPixel32 >> 8) & 0xFF) >> 3) << 6) | // G
			((((*inPixel32 >> 16) & 0xFF) >> 3) << 1) | // B
			((((*inPixel32 >> 24) & 0xFF) >> 7) << 0); // A

		delete []data;
		data = tempData;
	}

	if (data)
	{
		this->initWithData(data, pixelFormat, POTWide, POTHigh, imageSize);

		// should be after calling super init
		m_bHasPremultipliedAlpha = image->isPremultipliedAlpha();

		//CGContextRelease(context);
		delete [] data;
	}
	return true;
}

// implementation CCTexture2D (Text)
bool CCTexture2D::initWithString(const char *text, const char *fontName, float fontSize)
{
	return initWithString(text, CGSizeMake(0,0), UITextAlignmentCenter, fontName, fontSize);
}
bool CCTexture2D::initWithString(const char *text, CGSize dimensions, UITextAlignment alignment, const char *fontName, float fontSize)
{
	ccxImage image;
    ccxImage::ETextAlign eAlign = (UITextAlignmentCenter == alignment) ? ccxImage::kAlignCenter
        : (UITextAlignmentLeft == alignment) ? ccxImage::kAlignLeft : ccxImage::kAlignRight;
    
    if (! image.initWithString(text, (int)dimensions.width, (int)dimensions.height, eAlign, fontName, (int)fontSize))
    {
        return false;
    }
    return initWithImage(&image);
}


// implementation CCTexture2D (Drawing)

void CCTexture2D::drawAtPoint(CGPoint point)
{
	GLfloat	coordinates[] = {	
		0.0f,	m_fMaxT,
		m_fMaxS,m_fMaxT,
		0.0f,	0.0f,
		m_fMaxS,0.0f };

	GLfloat	width = (GLfloat)m_uPixelsWide * m_fMaxS,
		height = (GLfloat)m_uPixelsHigh * m_fMaxT;

	GLfloat		vertices[] = {	
		point.x,			point.y,	0.0f,
		width + point.x,	point.y,	0.0f,
		point.x,			height  + point.y,	0.0f,
		width + point.x,	height  + point.y,	0.0f };

	glBindTexture(GL_TEXTURE_2D, m_uName);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, coordinates);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CCTexture2D::drawInRect(CGRect rect)
{
	GLfloat	coordinates[] = {	
		0.0f,	m_fMaxT,
		m_fMaxS,m_fMaxT,
		0.0f,	0.0f,
		m_fMaxS,0.0f };

	GLfloat	vertices[] = {	rect.origin.x,		rect.origin.y,							/*0.0f,*/
		rect.origin.x + rect.size.width,		rect.origin.y,							/*0.0f,*/
		rect.origin.x,							rect.origin.y + rect.size.height,		/*0.0f,*/
		rect.origin.x + rect.size.width,		rect.origin.y + rect.size.height,		/*0.0f*/ };

	glBindTexture(GL_TEXTURE_2D, m_uName);
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, coordinates);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


// implementation CCTexture2D (PVRTC)
#ifdef _POWERVR_SUPPORT_
bool CCTexture2D::initWithPVRTCData(const void *data, int level, int bpp, bool hasAlpha, int length)
{
	if( !(CCConfiguration::sharedConfiguration()->isSupportsPVRTC()) )
	{
		CCLOG("cocos2d: WARNING: PVRTC images is not supported.");
		this->release();
		return false;
	}

	glGenTextures(1, &m_uName);
	glBindTexture(GL_TEXTURE_2D, m_uName);

	this->setAntiAliasTexParameters();

	GLenum format;
	GLsizei size = length * length * bpp / 8;
	if(hasAlpha) {
		format = (bpp == 4) ? GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
	} else {
		format = (bpp == 4) ? GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
	}
	if(size < 32) {
		size = 32;
	}
	glCompressedTexImage2D(GL_TEXTURE_2D, level, format, length, length, 0, size, data);

	m_tContentSize = CGSizeMake((float)(length), (float)(length));
	m_uPixelsWide = length;
	m_uPixelsHigh = length;
	m_fMaxS = 1.0f;
	m_fMaxT = 1.0f;

	return true;
}

CCTexture2D * CCTexture2D::initWithPVRTCFile(const char* file)
{
	if (! CCConfiguration::sharedConfiguration()->isSupportsPVRTC())
	{
		CCLOG("cocos2d: WARNING: PVRTC images is not supported");
		this->release();
		return false;
	}

	CCPVRTexture *pvr = new CCPVRTexture();
	pvr = pvr->initWithContentsOfFile(file);
	if( pvr )
	{
		pvr->setRetainName(true);			// don't dealloc texture on release

		m_uName = pvr->getName();				// texture id
		m_fMaxS = 1.0f;
		m_fMaxT = 1.0f;
		m_uPixelsWide = pvr->getWidth();		// width
		m_uPixelsHigh = pvr->getHeight();		// height
		/// be careful : unsigned int to float
		m_tContentSize = CGSizeMake((float)(m_uPixelsWide), (float)(m_uPixelsHigh));

		pvr->release();

		this->setAntiAliasTexParameters();
	}
	else 
	{
		CCLOG("cocos2d: Couldn't load PVR image");
		this->release();
		return false;
	}
	return true;
}
#endif

//
// Use to apply MIN/MAG filter
//
// implementation CCTexture2D (GLFilter)

void CCTexture2D::generateMipmap()
{
	NSAssert( m_uPixelsWide == ccNextPOT(m_uPixelsWide) && m_uPixelsHigh == ccNextPOT(m_uPixelsHigh), "Mimpap texture only works in POT textures");
	glBindTexture( GL_TEXTURE_2D, this->m_uName );
	ccglGenerateMipmap(GL_TEXTURE_2D);
}

void CCTexture2D::setTexParameters(ccTexParams *texParams)
{
	NSAssert( (m_uPixelsWide == ccNextPOT(m_uPixelsWide) && m_uPixelsHigh == ccNextPOT(m_uPixelsHigh)) ||
		(texParams->wrapS == GL_CLAMP_TO_EDGE && texParams->wrapT == GL_CLAMP_TO_EDGE),
		"GL_CLAMP_TO_EDGE should be used in NPOT textures");
	glBindTexture( GL_TEXTURE_2D, this->m_uName );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texParams->minFilter );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texParams->magFilter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texParams->wrapS );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texParams->wrapT );
}

void CCTexture2D::setAliasTexParameters()
{
	ccTexParams texParams = { GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
	this->setTexParameters(&texParams);
}

void CCTexture2D::setAntiAliasTexParameters()
{
	ccTexParams texParams = { GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
	this->setTexParameters(&texParams);
}

//
// Texture options for images that contains alpha
//
// implementation CCTexture2D (PixelFormat)

void CCTexture2D::setDefaultAlphaPixelFormat(CCTexture2DPixelFormat format)
{
	g_defaultAlphaPixelFormat = format;
}


CCTexture2DPixelFormat CCTexture2D::defaultAlphaPixelFormat()
{
	return g_defaultAlphaPixelFormat;
}

void CCTexture2D::reloadAllTextures()
{
#if CC_ENABLE_CACHE_TEXTTURE_DATA
    VolatileTexture::reloadAllTextures();
#endif
}

}//namespace   cocos2d 
