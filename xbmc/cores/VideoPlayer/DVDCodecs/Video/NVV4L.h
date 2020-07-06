#pragma once

#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include <string>
#include <vector>
#include <queue>
#include <thread>

#undef LOG_LEVEL_DEBUG
#include "NvVideoDecoder.h"
#include "NvVideoConverter.h"
#undef MAX_PLANES
#undef LOG_LEVEL_DEBUG
#define LOG_LEVEL_DEBUG 1

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libpostproc/postprocess.h>
}


class CBitstreamConverter;

class NVV4LCodec : public CDVDVideoCodec
{
public:
  NVV4LCodec(CProcessInfo &processInfo);
  ~NVV4LCodec();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset(void) override;
  virtual CDVDVideoCodec::VCReturn GetPicture(VideoPicture *pDvdVideoPicture) override;
  virtual unsigned GetAllowedReferences() override { return 4; }
  virtual const char* GetName(void) override { return "nvv4l"; }


  static CDVDVideoCodec* Create(CProcessInfo &processInfo);
  static bool Register();
private:
  NvVideoDecoder *m_nv_dec;
  NvVideoConverter *m_nv_conv;

  uint32_t m_codingType;
  std::string m_pFormatName;
  std::thread m_dec_capture_loop;

  uint32_t queued_buffers = 0;
  bool decoding = false;
  bool capturing = false;

  std::queue<NvBuffer*> m_buf_q;

  CBitstreamConverter *m_bitstream;

  void cap();
  void Dispose();
};
