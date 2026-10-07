/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RWPLCORE_H__
#define __GTA_RWPLCORE_H__

#include <rw.h>

typedef rw::int8 RwInt8;
typedef rw::int16 RwInt16;
typedef rw::int32 RwInt32;
typedef rw::uint8 RwUInt8;
typedef rw::uint16 RwUInt16;
typedef rw::uint32 RwUInt32;
typedef rw::float32 RwReal;

typedef char RwChar;
typedef RwInt32 RwBool;

#define __RWUNUSED__

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

// used for unicode
#define RWSTRING(x) x

typedef rw::V2d RwV2d;

typedef rw::V3d RwV3d;

typedef rw::Rect RwRect;

typedef rw::Sphere RwSphere;
typedef rw::Plane  RwPlane;
typedef rw::Line   RwLine;

typedef rw::LLLink   RwLLLink;
typedef rw::LinkList RwLinkList;

#define RWFORCEENUMSIZEINT ((RwInt32)((~((RwUInt32)0))>>1))

#define RwRealMAXVAL (RwReal)(3.40282347e+38)
#define RwRealMINVAL (RwReal)(1.17549435e-38)

#define rwPI      ((RwReal)(3.1415926535f))
#define rwPIOVER2 (rwPI / (RwReal)(2.0f))
#define RWRAD2DEG(_x) ((_x) * (((RwReal)180) / (rwPI)))
#define RWDEG2RAD(_x) ((_x) * ((rwPI) / ((RwReal)180)))

#define rwLLLinkGetData(linkvar, type, entry) ((type *)(((RwUInt8 *)(linkvar)) - offsetof(type, entry)))
#define rwLLLinkGetNext(linkvar)              ((linkvar)->next)
#define rwLLLinkGetPrevious(linkvar)          ((linkvar)->prev)
#define rwLinkListGetFirstLLLink(list)        ((list)->link.next)
#define rwLinkListGetLastLLLink(list)         ((list)->link.prev)
#define rwLinkListGetTerminator(list)         (&((list)->link))

enum RwTextureCoordinateIndex
{
	rwNARWTEXTURECOORDINATEINDEX = 0,
	rwTEXTURECOORDINATEINDEX0,
	rwTEXTURECOORDINATEINDEX1,
	rwTEXTURECOORDINATEINDEX2,
	rwTEXTURECOORDINATEINDEX3,
	rwTEXTURECOORDINATEINDEX4,
	rwTEXTURECOORDINATEINDEX5,
	rwTEXTURECOORDINATEINDEX6,
	rwTEXTURECOORDINATEINDEX7,
};

typedef rw::TexCoords RwTexCoords;

typedef rw::SurfaceProperties RwSurfaceProperties;

#define RWRGBALONG(r,g,b,a)                                             \
    ((RwUInt32) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))


#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))

enum RwPluginVendor
{
	rwVENDORID_CORE             = 0x000000L,
	rwVENDORID_CRITERIONTK      = 0x000001L,
	rwVENDORID_REDLINERACER     = 0x000002L,
	rwVENDORID_CSLRD            = 0x000003L,
	rwVENDORID_CRITERIONINT     = 0x000004L,
	rwVENDORID_CRITERIONWORLD   = 0x000005L,
	rwVENDORID_BETA             = 0x000006L,
	rwVENDORID_CRITERIONRM      = 0x000007L,
	rwVENDORID_CRITERIONRWA     = 0x000008L,
	rwVENDORID_CRITERIONRWP     = 0x000009L,
};

enum RwCorePluginID
{
	rwID_NAOBJECT               = 0x00,
	rwID_STRUCT                 = 0x01,
	rwID_STRING                 = 0x02,
	rwID_EXTENSION              = 0x03,
	rwID_CAMERA                 = 0x05,
	rwID_TEXTURE                = 0x06,
	rwID_MATERIAL               = 0x07,
	rwID_MATLIST                = 0x08,
	rwID_ATOMICSECT             = 0x09,
	rwID_PLANESECT              = 0x0A,
	rwID_WORLD                  = 0x0B,
	rwID_SPLINE                 = 0x0C,
	rwID_MATRIX                 = 0x0D,
	rwID_FRAMELIST              = 0x0E,
	rwID_GEOMETRY               = 0x0F,
	rwID_CLUMP                  = 0x10,
	rwID_LIGHT                  = 0x12,
	rwID_UNICODESTRING          = 0x13,
	rwID_ATOMIC                 = 0x14,
	rwID_TEXTURENATIVE          = 0x15,
	rwID_TEXDICTIONARY          = 0x16,
	rwID_ANIMDATABASE           = 0x17,
	rwID_IMAGE                  = 0x18,
	rwID_SKINANIMATION          = 0x19,
	rwID_GEOMETRYLIST           = 0x1A,
	rwID_HANIMANIMATION         = 0x1B,
	rwID_TEAM                   = 0x1C,
	rwID_CROWD                  = 0x1D,
	rwID_DMORPHANIMATION        = 0x1E,
	rwID_RIGHTTORENDER          = 0x1f,
	rwID_MTEFFECTNATIVE         = 0x20,
	rwID_MTEFFECTDICT           = 0x21,
	rwID_TEAMDICTIONARY         = 0x22,
	rwID_PITEXDICTIONARY        = 0x23,
	rwID_TOC                    = 0x24,
	rwID_PRTSTDGLOBALDATA       = 0x25,
	rwID_ALTPIPE                = 0x26,
	rwID_PIPEDS                 = 0x27,
	rwID_PATCHMESH              = 0x28,
	rwID_CHUNKGROUPSTART        = 0x29,
	rwID_CHUNKGROUPEND          = 0x2A,
	rwID_UVANIMDICT             = 0x2B,
	rwID_COLLTREE               = 0x2C,
	rwID_ENVIRONMENT            = 0x2D,
	/* Insert before MAX and increment MAX */
	rwID_COREPLUGINIDMAX        = 0x2E,
};


/*
 ***********************************************
 *
 * RwObject
 *
 ***********************************************
 */

//struct RwObject;
typedef rw::Object RwObject;
typedef rw::Frame RwFrame;

typedef RwObject *(*RwObjectCallBack)(RwObject *object, void *data);

// These take any kind of object in RenderWare (they're macros there)
#define RwObjectGetType(_obj)   (((const RwObject *)(_obj))->type)
#define rwObjectGetParent(_obj) ((RwFrame *)((const RwObject *)(_obj))->parent)
#define rwObjectSetParent(_obj, _parent) (((RwObject *)(_obj))->parent = (void *)(_parent))
#define rwObjectGetSubType(_obj)         (((const RwObject *)(_obj))->subType)
#define rwObjectSetSubType(_obj, _t)     (((RwObject *)(_obj))->subType = (RwUInt8)(_t))
#define rwObjectGetFlags(_obj)           (((const RwObject *)(_obj))->flags)
#define rwObjectSetFlags(_obj, _f)       (((RwObject *)(_obj))->flags = (RwUInt8)(_f))
#define rwObjectTestFlags(_obj, _f)      ((((const RwObject *)(_obj))->flags) & (RwUInt8)(_f))
#define rwObjectGetPrivateFlags(_obj)    (((const RwObject *)(_obj))->privateFlags)
#define rwObjectSetPrivateFlags(_obj, _f) (((RwObject *)(_obj))->privateFlags = (RwUInt8)(_f))
#define rwObjectTestPrivateFlags(_obj, _f) ((((const RwObject *)(_obj))->privateFlags) & (RwUInt8)(_f))

// Anything that starts with a `rw::ObjectWithFrame` (camera, light, atomic)
#define rwObjectHasFrameSetFrame(_obj, _frame) (((rw::ObjectWithFrame *)(_obj))->setFrame(_frame))
#define rwObjectHasFrameReleaseFrame(_obj)     (((rw::ObjectWithFrame *)(_obj))->setFrame(nullptr))

#define rwsprintf   sprintf
#define rwvsprintf  vsprintf
#define rwstrcpy    strcpy
#define rwstrncpy   strncpy
#define rwstrcat    strcat
#define rwstrncat   strncat
#define rwstrrchr   strrchr
#define rwstrchr    strchr
#define rwstrstr    strstr
#define rwstrcmp    strcmp
#define rwstricmp   stricmp
#define rwstrlen    strlen
#define rwstrupr    strupr
#define rwstrlwr    strlwr
#define rwstrtok    strtok
#define rwsscanf    sscanf


/*
 ***********************************************
 *
 * Memory
 *
 ***********************************************
 */

struct RwMemoryFunctions
{
	// NB: from RW 3.6 on the allocating functions take
	// a hint parameter!
	void *(*rwmalloc)(size_t size);
	void  (*rwfree)(void *mem);
	void *(*rwrealloc)(void *mem, size_t newSize);
	void *(*rwcalloc)(size_t numObj, size_t sizeObj);
};

void *RwMalloc(size_t size);
void  RwFree(void *mem);
void *RwRealloc(void *mem, size_t newSize);
void *RwCalloc(size_t numObj, size_t sizeObj);

/*
 ***********************************************
 *
 * RwStream
 *
 ***********************************************
 */

//struct RwStream;
typedef rw::Stream RwStream;

struct RwMemory
{
	RwUInt8     *start;
	RwUInt32    length;
};

enum RwStreamType
{
	rwNASTREAM = 0,
	rwSTREAMFILE,
	rwSTREAMFILENAME,
	rwSTREAMMEMORY,
	rwSTREAMCUSTOM
};

enum RwStreamAccessType
{
	rwNASTREAMACCESS = 0,
	rwSTREAMREAD,
	rwSTREAMWRITE,
	rwSTREAMAPPEND
};

RwStream *RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void *pData);
RwBool RwStreamClose(RwStream * stream, void *pData);
RwUInt32 RwStreamRead(RwStream * stream, void *buffer, RwUInt32 length);
RwStream *RwStreamWrite(RwStream * stream, const void *buffer, RwUInt32 length);
RwStream *RwStreamSkip(RwStream * stream, RwUInt32 offset);


/*
 ***********************************************
 *
 * Plugin Registry 
 *
 ***********************************************
 */

#define RWPLUGINOFFSET(_type, _base, _offset)                   \
   ((_type *)((RwUInt8 *)(_base) + (_offset)))
#define RWPLUGINOFFSETCONST(_type, _base, _offset)              \
   ((const _type *)((const RwUInt8 *)(_base) + (_offset)))

typedef RwStream *(*RwPluginDataChunkWriteCallBack)(RwStream *stream, RwInt32 binaryLength, const void *object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef RwStream *(*RwPluginDataChunkReadCallBack)(RwStream *stream, RwInt32 binaryLength, void *object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef RwInt32(*RwPluginDataChunkGetSizeCallBack)(const void *object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef RwBool(*RwPluginDataChunkAlwaysCallBack)(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef RwBool(*RwPluginDataChunkRightsCallBack)(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject, RwUInt32 extraData);
typedef void *(*RwPluginObjectConstructor)(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef void *(*RwPluginObjectCopy)(void *dstObject, const void *srcObject, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef void *(*RwPluginObjectDestructor)(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject);

/*
 ***********************************************
 *
 * RwMatrix
 *
 ***********************************************
 */

typedef rw::Matrix RwMatrix;
typedef rw::Matrix RwMatrixTag;

enum RwOpCombineType
{
	rwCOMBINEREPLACE = rw::COMBINEREPLACE,
	rwCOMBINEPRECONCAT = rw::COMBINEPRECONCAT,
	rwCOMBINEPOSTCONCAT = rw::COMBINEPOSTCONCAT
};

enum RwMatrixType
{
	rwMATRIXTYPENORMAL = rw::Matrix::TYPENORMAL,
	rwMATRIXTYPEORTHOGANAL = rw::Matrix::TYPEORTHOGONAL,
	rwMATRIXTYPEORTHONORMAL = rw::Matrix::TYPEORTHONORMAL,
	rwMATRIXTYPEMASK = 0x00000003,
	rwMATRIXINTERNALIDENTITY = rw::Matrix::IDENTITY,
};

#define rwMatrixSetFlags(m, flagsbit)  ((m)->flags = (flagsbit))
#define rwMatrixGetFlags(m)            ((m)->flags)
#define rwMatrixTestFlags(m, flagsbit) ((m)->flags & (RwInt32)(flagsbit))

typedef rw::Matrix::Tolerance RwMatrixTolerance;

RwBool RwMatrixDestroy(RwMatrix *mpMat);
RwMatrix *RwMatrixCreate(void);
void RwMatrixCopy(RwMatrix * dstMatrix, const RwMatrix * srcMatrix);
void RwMatrixSetIdentity(RwMatrix * matrix);
RwMatrix *RwMatrixMultiply(RwMatrix * matrixOut, const RwMatrix * MatrixIn1, const RwMatrix * matrixIn2);
RwMatrix *RwMatrixTransform(RwMatrix * matrix, const RwMatrix * transform, RwOpCombineType combineOp);
RwMatrix *RwMatrixOrthoNormalize(RwMatrix * matrixOut, const RwMatrix * matrixIn);
RwMatrix *RwMatrixInvert(RwMatrix * matrixOut, const RwMatrix * matrixIn);
RwMatrix *RwMatrixScale(RwMatrix * matrix, const RwV3d * scale, RwOpCombineType combineOp);
RwMatrix *RwMatrixTranslate(RwMatrix * matrix, const RwV3d * translation, RwOpCombineType combineOp);
RwMatrix *RwMatrixRotate(RwMatrix * matrix, const RwV3d * axis, RwReal angle, RwOpCombineType combineOp);
RwMatrix *RwMatrixRotateOneMinusCosineSine(RwMatrix * matrix, const RwV3d * unitAxis, RwReal oneMinusCosine, RwReal sine, RwOpCombineType combineOp);
const RwMatrix *RwMatrixQueryRotate(const RwMatrix * matrix, RwV3d * unitAxis, RwReal * angle, RwV3d * center);
inline RwV3d *RwMatrixGetRight(RwMatrix * matrix) { return &matrix->right; }
inline RwV3d *RwMatrixGetUp(RwMatrix * matrix) { return &matrix->up; }
inline RwV3d *RwMatrixGetAt(RwMatrix * matrix) { return &matrix->at; }
inline RwV3d *RwMatrixGetPos(RwMatrix * matrix) { return &matrix->pos; }
inline const RwV3d *RwMatrixGetRight(const RwMatrix * matrix) { return &matrix->right; }
inline const RwV3d *RwMatrixGetUp(const RwMatrix * matrix) { return &matrix->up; }
inline const RwV3d *RwMatrixGetAt(const RwMatrix * matrix) { return &matrix->at; }
inline const RwV3d *RwMatrixGetPos(const RwMatrix * matrix) { return &matrix->pos; }
RwMatrix *RwMatrixUpdate(RwMatrix * matrix);
RwMatrix *RwMatrixOptimize(RwMatrix * matrix, const RwMatrixTolerance *tolerance);

/*
 ***********************************************
 *
 * RwRGBA
 *
 ***********************************************
 */

typedef rw::RGBA RwRGBA;
typedef rw::RGBAf RwRGBAReal;


inline void RwRGBAAssign(RwRGBA *target, const RwRGBA *source) { *target = *source; }


RwReal RwV3dNormalize(RwV3d * out, const RwV3d * in);
RwReal RwV3dLength(const RwV3d * in);
RwReal RwV2dLength(const RwV2d * in);
RwReal RwV2dNormalize(RwV2d * out, const RwV2d * in);
void RwV2dAssign(RwV2d * out, const RwV2d * ina);
void RwV2dAdd(RwV2d * out, const RwV2d * ina, const RwV2d * inb);
void RwV2dLineNormal(RwV2d * out, const RwV2d * ina, const RwV2d * inb);
void RwV2dSub(RwV2d * out, const RwV2d * ina, const RwV2d * inb);
void RwV2dPerp(RwV2d * out, const RwV2d * in);
void RwV2dScale(RwV2d * out, const RwV2d * in, RwReal scalar);
RwReal RwV2dDotProduct(const RwV2d * ina, const RwV2d * inb);
void RwV3dAssign(RwV3d * out, const RwV3d * ina);
void RwV3dAdd(RwV3d * out, const RwV3d * ina, const RwV3d * inb);
void RwV3dSub(RwV3d * out, const RwV3d * ina, const RwV3d * inb);
void RwV3dScale(RwV3d * out, const RwV3d * in, RwReal scalar);
void RwV3dIncrementScaled(RwV3d * out,  const RwV3d * in, RwReal scalar);
void RwV3dNegate(RwV3d * out, const RwV3d * in);
RwReal RwV3dDotProduct(const RwV3d * ina, const RwV3d * inb);
void RwV3dCrossProduct(RwV3d * out, const RwV3d * ina, const RwV3d * inb);
RwV3d *RwV3dTransformPoints(RwV3d * pointsOut, const RwV3d * pointsIn, RwInt32 numPoints, const RwMatrix * matrix);
RwV3d *RwV3dTransformVectors(RwV3d * vectorsOut, const RwV3d * vectorsIn, RwInt32 numPoints, const RwMatrix * matrix);
inline RwV3d *RwV3dTransformPoint(RwV3d * pointOut, const RwV3d * pointIn, const RwMatrix * matrix) { return RwV3dTransformPoints(pointOut, pointIn, 1, matrix); }
inline RwV3d *RwV3dTransformVector(RwV3d * vectorOut, const RwV3d * vectorIn, const RwMatrix * matrix) { return RwV3dTransformVectors(vectorOut, vectorIn, 1, matrix); }


/*
 ***********************************************
 *
 * Render States
 *
 ***********************************************
 */

// Values as in RenderWare 3.6, mapped onto librw's in the wrapper functions
enum RwRenderState
{
    rwRENDERSTATENARENDERSTATE = 0,
    rwRENDERSTATETEXTURERASTER,
    rwRENDERSTATETEXTUREADDRESS,
    rwRENDERSTATETEXTUREADDRESSU,
    rwRENDERSTATETEXTUREADDRESSV,
    rwRENDERSTATETEXTUREPERSPECTIVE,
    rwRENDERSTATEZTESTENABLE,
    rwRENDERSTATESHADEMODE,
    rwRENDERSTATEZWRITEENABLE,
    rwRENDERSTATETEXTUREFILTER,
    rwRENDERSTATESRCBLEND,
    rwRENDERSTATEDESTBLEND,
    rwRENDERSTATEVERTEXALPHAENABLE,
    rwRENDERSTATEBORDERCOLOR,
    rwRENDERSTATEFOGENABLE,
    rwRENDERSTATEFOGCOLOR,
    rwRENDERSTATEFOGTYPE,
    rwRENDERSTATEFOGDENSITY,
    rwRENDERSTATECULLMODE = 20,
    rwRENDERSTATESTENCILENABLE,
    rwRENDERSTATESTENCILFAIL,
    rwRENDERSTATESTENCILZFAIL,
    rwRENDERSTATESTENCILPASS,
    rwRENDERSTATESTENCILFUNCTION,
    rwRENDERSTATESTENCILFUNCTIONREF,
    rwRENDERSTATESTENCILFUNCTIONMASK,
    rwRENDERSTATESTENCILFUNCTIONWRITEMASK,
    rwRENDERSTATEALPHATESTFUNCTION,
    rwRENDERSTATEALPHATESTFUNCTIONREF,
    rwRENDERSTATEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
enum RwStencilOperation
{
    rwSTENCILOPERATIONNASTENCILOPERATION = 0,
    rwSTENCILOPERATIONKEEP,     
    rwSTENCILOPERATIONZERO,     
    rwSTENCILOPERATIONREPLACE,  
    rwSTENCILOPERATIONINCRSAT,  
    rwSTENCILOPERATIONDECRSAT,  
    rwSTENCILOPERATIONINVERT,   
    rwSTENCILOPERATIONINCR,     
    rwSTENCILOPERATIONDECR,     
    rwSTENCILOPERATIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
enum RwStencilFunction
{
    rwSTENCILFUNCTIONNASTENCILFUNCTION = 0,
    rwSTENCILFUNCTIONNEVER,         
    rwSTENCILFUNCTIONLESS,          
    rwSTENCILFUNCTIONEQUAL,         
    rwSTENCILFUNCTIONLESSEQUAL,     
    rwSTENCILFUNCTIONGREATER,       
    rwSTENCILFUNCTIONNOTEQUAL,      
    rwSTENCILFUNCTIONGREATEREQUAL,  
    rwSTENCILFUNCTIONALWAYS,        
    rwSTENCILFUNCTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
enum RwAlphaTestFunction
{
    rwALPHATESTFUNCTIONNAALPHATESTFUNCTION = 0,
    rwALPHATESTFUNCTIONNEVER,         
    rwALPHATESTFUNCTIONLESS,          
    rwALPHATESTFUNCTIONEQUAL,         
    rwALPHATESTFUNCTIONLESSEQUAL,     
    rwALPHATESTFUNCTIONGREATER,       
    rwALPHATESTFUNCTIONNOTEQUAL,      
    rwALPHATESTFUNCTIONGREATEREQUAL,  
    rwALPHATESTFUNCTIONALWAYS,        
    rwALPHATESTFUNCTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
enum RwDebugType
{
    rwNADEBUGTYPE = 0,          
    rwDEBUGASSERT,              
    rwDEBUGERROR,               
    rwDEBUGMESSAGE,             
    rwDEBUGTRACE,               
    rwDEBUGTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

// not supported - we only do gouraud
enum RwShadeMode
{
	rwSHADEMODENASHADEMODE = 0,
	rwSHADEMODEFLAT,
	rwSHADEMODEGOURAUD
};

enum RwBlendFunction
{
	rwBLENDNABLEND = 0,
	rwBLENDZERO = rw::BLENDZERO,
	rwBLENDONE = rw::BLENDONE,
	rwBLENDSRCCOLOR = rw::BLENDSRCCOLOR,
	rwBLENDINVSRCCOLOR = rw::BLENDINVSRCCOLOR,
	rwBLENDSRCALPHA = rw::BLENDSRCALPHA,
	rwBLENDINVSRCALPHA = rw::BLENDINVSRCALPHA,
	rwBLENDDESTALPHA = rw::BLENDDESTALPHA,
	rwBLENDINVDESTALPHA = rw::BLENDINVDESTALPHA,
	rwBLENDDESTCOLOR = rw::BLENDDESTCOLOR,
	rwBLENDINVDESTCOLOR = rw::BLENDINVDESTCOLOR,
	rwBLENDSRCALPHASAT = rw::BLENDSRCALPHASAT
};

// unsupported - we only need linear
enum RwFogType
{
	rwFOGTYPENAFOGTYPE = 0,
	rwFOGTYPELINEAR,
	rwFOGTYPEEXPONENTIAL,
	rwFOGTYPEEXPONENTIAL2
};

enum RwTextureFilterMode
{
	rwFILTERNAFILTERMODE = 0,
	rwFILTERNEAREST = rw::Texture::NEAREST,
	rwFILTERLINEAR = rw::Texture::LINEAR,
	rwFILTERMIPNEAREST = rw::Texture::MIPNEAREST,
	rwFILTERMIPLINEAR = rw::Texture::MIPLINEAR,
	rwFILTERLINEARMIPNEAREST = rw::Texture::LINEARMIPNEAREST,
	rwFILTERLINEARMIPLINEAR = rw::Texture::LINEARMIPLINEAR
};

enum RwTextureAddressMode
{
	rwTEXTUREADDRESSNATEXTUREADDRESS = 0,
	rwTEXTUREADDRESSWRAP = rw::Texture::WRAP,
	rwTEXTUREADDRESSMIRROR = rw::Texture::MIRROR,
	rwTEXTUREADDRESSCLAMP = rw::Texture::CLAMP,
	rwTEXTUREADDRESSBORDER = rw::Texture::BORDER
};

enum RwCullMode
{
	rwCULLMODENACULLMODE = 0,
	rwCULLMODECULLNONE = rw::CULLNONE,
	rwCULLMODECULLBACK = rw::CULLBACK,
	rwCULLMODECULLFRONT = rw::CULLFRONT
};

enum RwPrimitiveType
{
	rwPRIMTYPENAPRIMTYPE = rw::PRIMTYPENONE,
	rwPRIMTYPELINELIST = rw::PRIMTYPELINELIST,
	rwPRIMTYPEPOLYLINE = rw::PRIMTYPEPOLYLINE,
	rwPRIMTYPETRILIST = rw::PRIMTYPETRILIST,
	rwPRIMTYPETRISTRIP = rw::PRIMTYPETRISTRIP,
	rwPRIMTYPETRIFAN = rw::PRIMTYPETRIFAN,
	rwPRIMTYPEPOINTLIST = rw::PRIMTYPEPOINTLIST
};


RwBool RwRenderStateGet(RwRenderState state, void *value);
RwBool RwRenderStateSet(RwRenderState state, void *value);


/*
 ***********************************************
 *
 * Engine
 *
 ***********************************************
 */

struct RwEngineOpenParams
{
	void    *displayID;
};

typedef rw::SubSystemInfo RwSubSystemInfo;

enum RwVideoModeFlag
{
	rwVIDEOMODEEXCLUSIVE  = rw::VIDEOMODEEXCLUSIVE,
/*
	rwVIDEOMODEINTERLACE  = 0x2,
	rwVIDEOMODEFFINTERLACE  = 0x4,
	rwVIDEOMODEFSAA0 = 0x8,
	rwVIDEOMODEFSAA1 = 0x10
*/
};

typedef rw::VideoMode RwVideoMode;

#if 0
struct RwFileFunctions
{
	rwFnFexist  rwfexist; /**< Pointer to fexist function */
	rwFnFopen   rwfopen;  /**< Pointer to fopen function */
	rwFnFclose  rwfclose; /**< Pointer to fclose function */
	rwFnFread   rwfread;  /**< Pointer to fread function */
	rwFnFwrite  rwfwrite; /**< Pointer to fwrite function */
	rwFnFgets   rwfgets;  /**< Pointer to fgets function */
	rwFnFputs   rwfputs;  /**< Pointer to puts function */
	rwFnFeof    rwfeof;   /**< Pointer to feof function */
	rwFnFseek   rwfseek;  /**< Pointer to fseek function */
	rwFnFflush  rwfflush; /**< Pointer to fflush function */
	rwFnFtell   rwftell;  /**< Pointer to ftell function */  
};
RwFileFunctions *RwOsGetFileInterface(void);
#endif

RwBool RwEngineInit(const RwMemoryFunctions *memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize);
RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor initCB, RwPluginObjectDestructor termCB);
RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID);
RwBool RwEngineOpen(RwEngineOpenParams *initParams);
RwBool RwEngineStart(void);
RwBool RwEngineStop(void);
RwBool RwEngineClose(void);
RwBool RwEngineTerm(void);
RwInt32 RwEngineGetNumSubSystems(void);
RwSubSystemInfo *RwEngineGetSubSystemInfo(RwSubSystemInfo *subSystemInfo, RwInt32 subSystemIndex);
RwInt32 RwEngineGetCurrentSubSystem(void);
RwBool RwEngineSetSubSystem(RwInt32 subSystemIndex);
RwInt32 RwEngineGetNumVideoModes(void);
RwVideoMode *RwEngineGetVideoModeInfo(RwVideoMode *modeinfo, RwInt32 modeIndex);
RwInt32 RwEngineGetCurrentVideoMode(void);
RwBool RwEngineSetVideoMode(RwInt32 modeIndex);
RwInt32 RwEngineGetTextureMemorySize(void);
RwInt32 RwEngineGetMaxTextureSize(void);
RwUInt32 RwEngineGetVersion(void);

// The handful of engine globals game code reads directly (through `RWSRCGLOBAL`)
struct RwLibrwGlobals
{
	struct {
		RwReal zBufferNear;
		RwReal zBufferFar;
	} dOpenDevice;
	RwUInt16 renderFrame;
	RwUInt16 lightFrame;
};
RwLibrwGlobals &RwLibrwGetGlobals(void);
#define RWSRCGLOBAL(variable) (RwLibrwGetGlobals().variable)

/*
 ***********************************************
 *
 * Resources
 *
 ***********************************************
 */

// TODO(librw): librw has no resource arena, instance data hangs off the geometry (`rw::Geometry::instData`)
struct RwResEntry;
typedef void (*RwResEntryDestroyNotify)(RwResEntry *resEntry);
struct RwResEntry
{
	RwLLLink                link;
	RwInt32                 size;
	void                   *owner;
	RwResEntry            **ownerRef;
	RwResEntryDestroyNotify destroyNotify;
};


/*
 ***********************************************
 *
 * Binary stream
 *
 ***********************************************
 */

RwBool RwStreamFindChunk(RwStream *stream, RwUInt32 type, RwUInt32 *lengthOut, RwUInt32 *versionOut);

struct RwChunkHeaderInfo
{
	RwUInt32 type;
	RwUInt32 length;
	RwUInt32 version;
	RwUInt32 buildNum;
	RwBool   isComplex;
};
RwBool RwStreamReadChunkHeaderInfo(RwStream *stream, RwChunkHeaderInfo *chunkHeaderInfo);
RwStream *_rwStreamInitialize(RwStream *stream, RwBool rwOwned, RwStreamType type, RwStreamAccessType accessType, const void *pData);

/*
 ***********************************************
 *
 * Debug
 *
 ***********************************************
 */

typedef void (*RwDebugHandler)(RwDebugType type, const RwChar *string);
// No-ops, as in the release libraries of RenderWare
#define RwDebugSetHandler(handler)
#define RwDebugSetTraceState(state)

#endif // __GTA_RWPLCORE_H__
