#pragma once

#include "mathutils.h"
#include "miniwin/d3drm.h"

#include <SDL2/SDL.h>

#define NO_TEXTURE_ID 0xffffffff

struct TexCoord {
	float u, v;
};

struct GeometryVertex {
	D3DVECTOR position;
	D3DVECTOR normals;
	TexCoord texCoord;
};
static_assert(sizeof(GeometryVertex) == 32);

struct Appearance {
	SDL_Color color;
	float shininess;
	Uint32 textureId;
};

struct FColor {
	float r, g, b, a;
};

struct SceneLight {
	FColor color;
	D3DVECTOR position;
	float positional = 0.f; // position is valid if 1.f
	D3DVECTOR direction;
	float directional = 0.f; // direction is valid if 1.f
};
static_assert(sizeof(SceneLight) == 48);

class Direct3DRMRenderer : public IDirect3DDevice2 {
public:
	virtual void PushLights(const SceneLight* vertices, size_t count) = 0;
	virtual void SetProjection(const D3DRMMATRIX4D& projection, D3DVALUE front, D3DVALUE back) = 0;
	virtual Uint32 GetTextureId(IDirect3DRMTexture* texture) = 0;
	virtual DWORD GetWidth() = 0;
	virtual DWORD GetHeight() = 0;
	virtual void GetDesc(D3DDEVICEDESC* halDesc, D3DDEVICEDESC* helDesc) = 0;
	virtual const char* GetName() = 0;
	virtual HRESULT BeginFrame(const D3DRMMATRIX4D& viewMatrix) = 0;
	virtual void SubmitDraw(
		const GeometryVertex* vertices,
		const size_t count,
		const D3DRMMATRIX4D& worldMatrix,
		const Matrix3x3& normalMatrix,
		const Appearance& appearance
	) = 0;
	virtual HRESULT FinalizeFrame() = 0;
};
