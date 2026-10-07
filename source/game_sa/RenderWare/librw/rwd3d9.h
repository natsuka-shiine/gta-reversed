/*
* The Direct3D 9 specific part of the RenderWare API (`RwD3D9...`, `RxD3D9...`) for librw's D3D9 backend.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RWD3D9_H__
#define __GTA_RWD3D9_H__

#include <d3d9.h>

#define RWD3D9_MAX_TEXTURE_STAGES 8
#define RWD3D9_MAX_VERTEX_STREAMS 2

typedef rw::V4d RwV4d;

struct RxD3D9VertexStream
{
	void    *vertexBuffer;
	RwUInt32 offset;
	RwUInt32 stride;
	RwUInt16 geometryFlags;
	RwUInt8  managed;
	RwUInt8  dynamicLock;
};

typedef void (*rwD3D9DeviceRestoreCallBack)(void);

RwBool   RwD3D9DeviceSupportsDXTTexture(void);
void    *RwD3D9GetCurrentD3DDevice(void);
RwUInt32 RwD3D9EngineGetMaxMultiSamplingLevels(void);
void     RwD3D9EngineSetMultiSamplingLevels(RwUInt32 numLevels);
void     RwD3D9EngineSetRefreshRate(RwUInt32 refreshRate);
void    *RwD3D9GetCurrentD3DRenderTarget(RwUInt32 index);
RwBool   RwD3D9SetRenderTarget(RwUInt32 index, RwRaster *raster);
RwBool   RwD3D9ChangeVideoMode(RwInt32 modeIndex);
RwBool   RwD3D9ChangeMultiSamplingLevels(RwUInt32 numLevels);
RwBool   RwD3D9CameraAttachWindow(void *camera, void *hwnd);
void     RwD3D9SetStreamSource(RwUInt32 streamNumber, void *streamData, RwUInt32 offset, RwUInt32 stride);
void     _rwD3D9SetStreams(const RxD3D9VertexStream *streams, RwBool useOffsets);
void     RwD3D9SetIndices(void *indexBuffer);

void     _rwD3D9SetFVF(RwUInt32 fvf);
void     _rwD3D9SetVertexDeclaration(void *vertexDeclaration);
void     _rwD3D9SetVertexShader(void *shader);
void     _rwD3D9SetPixelShader(void *shader);
void     _rwD3D9DrawIndexedPrimitive(RwUInt32 primitiveType, RwInt32 baseVertexIndex, RwUInt32 minIndex, RwUInt32 numVertices, RwUInt32 startIndex, RwUInt32 primitiveCount);
void     _rwD3D9DrawPrimitive(RwUInt32 primitiveType, RwUInt32 startVertex, RwUInt32 primitiveCount);
void     _rwD3D9DrawIndexedPrimitiveUP(RwUInt32 primitiveType, RwUInt32 minIndex, RwUInt32 numVertices, RwUInt32 primitiveCount, const void *indexData, const void *vertexStreamZeroData, RwUInt32 vertexStreamZeroStride);
void     _rwD3D9DrawPrimitiveUP(RwUInt32 primitiveType, RwUInt32 primitiveCount, const void *vertexStreamZeroData, RwUInt32 vertexStreamZeroStride);
void     _rwD3D9SetVertexShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount);
void     _rwD3D9SetPixelShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount);
#define RwD3D9SetFVF                  _rwD3D9SetFVF
#define RwD3D9SetVertexDeclaration    _rwD3D9SetVertexDeclaration
#define RwD3D9SetVertexShader         _rwD3D9SetVertexShader
#define RwD3D9SetPixelShader          _rwD3D9SetPixelShader
#define RwD3D9DrawIndexedPrimitive    _rwD3D9DrawIndexedPrimitive
#define RwD3D9DrawPrimitive           _rwD3D9DrawPrimitive
#define RwD3D9DrawIndexedPrimitiveUP  _rwD3D9DrawIndexedPrimitiveUP
#define RwD3D9DrawPrimitiveUP         _rwD3D9DrawPrimitiveUP
#define RwD3D9SetVertexShaderConstant _rwD3D9SetVertexShaderConstant
#define RwD3D9SetPixelShaderConstant  _rwD3D9SetPixelShaderConstant

void     RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value);
void     RwD3D9GetRenderState(RwUInt32 state, void *value);
void     RwD3D9SetTextureStageState(RwUInt32 stage, RwUInt32 type, RwUInt32 value);
void     RwD3D9GetTextureStageState(RwUInt32 stage, RwUInt32 type, void *value);
void     RwD3D9SetSamplerState(RwUInt32 stage, RwUInt32 type, RwUInt32 value);
void     RwD3D9GetSamplerState(RwUInt32 stage, RwUInt32 type, void *value);
void     RwD3D9SetStencilClear(RwUInt32 stencilClear);
RwUInt32 RwD3D9GetStencilClear(void);
RwBool   RwD3D9SetTexture(RwTexture *texture, RwUInt32 stage);
RwBool   RwD3D9SetTransform(RwUInt32 state, const void *matrix);
void     RwD3D9GetTransform(RwUInt32 state, void *matrix);
RwBool   RwD3D9SetMaterial(const void *material);
RwBool   RwD3D9SetClipPlane(RwUInt32 index, const RwV4d *plane);
RwBool   RwD3D9SetTransformWorld(const RwMatrix *matrix);
RwBool   RwD3D9SetSurfaceProperties(const RwSurfaceProperties *surfaceProps, const RwRGBA *color, RwUInt32 flags);
RwBool   RwD3D9SetLight(RwInt32 index, const void *light);
void     RwD3D9GetLight(RwInt32 index, void *light);
RwBool   RwD3D9EnableLight(RwInt32 index, RwBool enable);
RwBool   RwD3D9IndexBufferCreate(RwUInt32 numIndices, void **indexBuffer);
RwBool   RwD3D9CreateVertexBuffer(RwUInt32 stride, RwUInt32 size, void **vertexBuffer, RwUInt32 *offset);
void     RwD3D9DestroyVertexBuffer(RwUInt32 stride, RwUInt32 size, void *vertexBuffer, RwUInt32 offset);
RwBool   RwD3D9DynamicVertexBufferCreate(RwUInt32 size, void **vertexBuffer);
void     RwD3D9DynamicVertexBufferDestroy(void *vertexBuffer);
RwBool   RwD3D9DynamicVertexBufferLock(RwUInt32 vertexSize, RwUInt32 numVertex, void **vertexBufferOut, void **vertexDataOut, RwUInt32 *baseIndexOut);
RwBool   RwD3D9DynamicVertexBufferUnlock(void *vertexBuffer);
RwBool   RwD3D9CreateVertexDeclaration(const void *elements, void **vertexdeclaration);
void     RwD3D9DeleteVertexDeclaration(void *vertexdeclaration);
RwBool   RwD3D9CreateVertexShader(const RwUInt32 *function, void **shader);
void     RwD3D9DeleteVertexShader(void *shader);
RwBool   RwD3D9CreatePixelShader(const RwUInt32 *function, void **shader);
void     RwD3D9DeletePixelShader(void *shader);
const void *RwD3D9GetCaps(void);
RwRaster *RwD3D9RasterCreate(RwUInt32 width, RwUInt32 height, RwUInt32 d3dFormat, RwUInt32 flags);

void     _rwD3D9DeviceSetRestoreCallback(rwD3D9DeviceRestoreCallBack callback);
rwD3D9DeviceRestoreCallBack _rwD3D9DeviceGetRestoreCallback(void);

#endif // __GTA_RWD3D9_H__
