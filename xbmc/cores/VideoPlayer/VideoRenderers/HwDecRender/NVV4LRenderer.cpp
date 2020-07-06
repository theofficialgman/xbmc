#include "NVV4LRenderer.h"

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include <NvBuffer.h>

using namespace NVV4L;


NVV4LBuffer::NVV4LBuffer(int id) : CVideoBuffer(id) 
{

};

NVV4LBuffer::~NVV4LBuffer() 
{

};

void NVV4LBuffer::SetRef(NvBuffer *ref)
{
  this->m_ref = ref;
}

NvBuffer* NVV4LBuffer::GetRef()
{
  return this->m_ref;
}


NVV4LEGLRenderer::NVV4LEGLRenderer()
{

}


NVV4LEGLRenderer::~NVV4LEGLRenderer()
{

}

bool NVV4LEGLRenderer::UploadTexture(int index)
{

};

void NVV4LEGLRenderer::DeleteTexture(int index)
{

};

bool CreateTexture(int index)
{

};

bool NVV4LEGLRenderer::Supports(ERENDERFEATURE feature)
{
  return CLinuxRendererGLES::Supports(feature);
}

bool NVV4LEGLRenderer::Supports(ESCALINGMETHOD method)
{
  return CLinuxRendererGLES::Supports(method);
}
