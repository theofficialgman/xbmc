#pragma once

#include <vector>

#include "windowing/GraphicContext.h"
#include "../RenderFlags.h"
#include "../LinuxRendererGLES.h"
#include "cores/VideoSettings.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "utils/Geometry.h"


class NvBuffer;

namespace NVV4L
{

class NVV4LPool : public IVideoBufferPool
{
public:
  NVV4LPool(const char *component_name, bool input, uint32_t num_buffers, uint32_t buffer_size, uint32_t encoding);
  ~NVV4LPool();

/*
  virtual CVideoBuffer* Get() override;
  virtual void Return(int id) override;
  virtual void Configure(AVPixelFormat format, int size) override;
  virtual bool IsConfigured() override;
  virtual bool IsCompatible(AVPixelFormat format, int size) override;
*/
};

class NVV4LBuffer : public CVideoBuffer
{
public:
  NVV4LBuffer(int id);
   ~NVV4LBuffer();
  
  void SetRef(NvBuffer *ref);
  NvBuffer* GetRef();
private:
  NvBuffer* m_ref;
}; // NVV4LBuffer class;


class NVV4LEGLRenderer : public CLinuxRendererGLES
{
public:
  NVV4LEGLRenderer();
  virtual ~NVV4LEGLRenderer();


  // Feature support
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static void Register();

protected:

  // textures
  bool UploadTexture(int index) override;
  void DeleteTexture(int index) override;
  bool CreateTexture(int index) override;
};


}; // namespace;
