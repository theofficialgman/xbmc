#include "NVV4L.h"

#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/NVV4LRenderer.h"
#include "utils/BitstreamConverter.h"
#include "utils/log.h"

#include <string>


#define NV_MAX_PLANES 3

NVV4LCodec::NVV4LCodec(CProcessInfo &processInfo) : 
    CDVDVideoCodec(processInfo), 
    m_dec_capture_loop(),
    m_bitstream(nullptr)
{
  CLog::Log(LOGINFO, LOGVIDEO, "NVV4LCodec::NVV4LCodec NVV4L enabled");
};


NVV4LCodec::~NVV4LCodec()
{

  m_nv_dec->abort();

  decoding = false;
  m_dec_capture_loop.join();
};


bool NVV4LCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{

  m_nv_dec = NvVideoDecoder::createVideoDecoder("dec-0");
  if (!m_nv_dec) {
    CLog::Log(LOGERROR, LOGVIDEO, "call NvVideoDecoder::createVideoDecoder failed");
    return false;
  }

  if (m_nv_dec->subscribeEvent(V4L2_EVENT_RESOLUTION_CHANGE, 0, 0) < 0) {
    CLog::Log(LOGERROR, LOGVIDEO, "call NvVideoDecoder::subscribeEvent failed");
    return false;
  }

  switch(hints.codec)
  {
    case AV_CODEC_ID_H264:
      m_codingType = V4L2_PIX_FMT_H264;
      m_pFormatName = "h264";
    break;
    case AV_CODEC_ID_H263:
    case AV_CODEC_ID_MPEG4:
      // MPEG-4, DivX 4/5 and Xvid compatible
      m_codingType = V4L2_PIX_FMT_MPEG4;
      m_pFormatName = "mpeg4";
    break;
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
      // MPEG-2
      m_codingType = V4L2_PIX_FMT_MPEG2;
      m_pFormatName = "mpeg2";
    break;
    case AV_CODEC_ID_VP8:
      // VP8
      m_codingType =  V4L2_PIX_FMT_VP8;
      m_pFormatName = "vp8";
    break;
    default:
      CLog::Log(LOGERROR, LOGVIDEO, "NVV4LCodec::Open Video codec unknown: %x", hints.codec);
      return false;
    break;
  }
  
  m_bitstream = new CBitstreamConverter;
  m_bitstream->Open(hints.codec, (uint8_t*)hints.extradata, hints.extrasize, true);


  if (m_nv_dec->setOutputPlaneFormat(m_codingType, 4000000) < 0) {
    CLog::Log(LOGERROR, LOGVIDEO, "call NvVideoDecoder::setOutputPlaneFormat failed");
    return false;
  }


  if (m_nv_dec->setFrameInputMode(1) < 0) {
    CLog::Log(LOGERROR, LOGVIDEO, "call NvVideoDecoder::setFrameInputMode failed");
    return false;
  }


  if (m_nv_dec->output_plane.setupPlane(V4L2_MEMORY_MMAP, 10, true, false) < 0) {
    CLog::Log(LOGERROR, LOGVIDEO, "setup output plane failed");
    return false;
  }

  if (m_nv_dec->output_plane.setStreamStatus(true) < 0) {
    CLog::Log(LOGERROR, LOGVIDEO, "NVV4LCodec::Open error setting stream on");
    return false;
  };

  m_dec_capture_loop = std::thread(&NVV4LCodec::cap, this);

  return true;
};


void NVV4LCodec::cap() 
{
  while (!decoding) 
  {
      struct v4l2_event ev;
      if (m_nv_dec->dqEvent(ev, 1000) == 0) {
          if (ev.type == V4L2_EVENT_RESOLUTION_CHANGE) {
              struct v4l2_format format;
 
              m_nv_dec->capture_plane.getFormat(format);
              CLog::Log(LOGINFO, LOGVIDEO, "NVV4LCodec::Open event resolution change");
 
              m_nv_dec->capture_plane.deinitPlane();

              m_nv_dec->setCapturePlaneFormat(format.fmt.pix_mp.pixelformat,
                                     format.fmt.pix_mp.width,
                                     format.fmt.pix_mp.height);

              int32_t cap_buff;
              m_nv_dec->getMinimumCapturePlaneBuffers(cap_buff);
              m_nv_dec->capture_plane.setupPlane(V4L2_MEMORY_MMAP, cap_buff + 5, false, false);

              m_nv_dec->capture_plane.setStreamStatus(true);

              for (uint32_t i = 0; i < m_nv_dec->capture_plane.getNumBuffers(); i++) {
                  struct v4l2_buffer v4l2_buf;
                  struct v4l2_plane planes[NV_MAX_PLANES];

                  memset(&v4l2_buf, 0, sizeof(v4l2_buf));
                  memset(planes, 0, sizeof(planes));

                  v4l2_buf.index = i;
                  v4l2_buf.m.planes = planes;
                
                  m_nv_dec->capture_plane.qBuffer(v4l2_buf, NULL);
              }

              decoding = true;
          }
      }
  }

  while (decoding) {
      struct v4l2_event ev;
      if (m_nv_dec->dqEvent(ev, false) == 0) {
          if (ev.type == V4L2_EVENT_RESOLUTION_CHANGE) {
              struct v4l2_format format;
 
              m_nv_dec->capture_plane.getFormat(format);
 
              m_nv_dec->capture_plane.deinitPlane();

              m_nv_dec->setCapturePlaneFormat(format.fmt.pix_mp.pixelformat,
                                     format.fmt.pix_mp.width,
                                     format.fmt.pix_mp.height);

              int32_t cap_buff;
              m_nv_dec->getMinimumCapturePlaneBuffers(cap_buff);
              m_nv_dec->capture_plane.setupPlane(V4L2_MEMORY_MMAP, cap_buff + 5, false, false);

              m_nv_dec->capture_plane.setStreamStatus(true);

              for (uint32_t i = 0; i < m_nv_dec->capture_plane.getNumBuffers(); i++) {
                  struct v4l2_buffer v4l2_buf;
                  struct v4l2_plane planes[NV_MAX_PLANES];

                  memset(&v4l2_buf, 0, sizeof(v4l2_buf));
                  memset(planes, 0, sizeof(planes));

                  v4l2_buf.index = i;
                  v4l2_buf.m.planes = planes;
                
                  m_nv_dec->capture_plane.qBuffer(v4l2_buf, NULL);
              }

          }
      }

      struct v4l2_buffer v4l2_buf;
      struct v4l2_plane planes[NV_MAX_PLANES];
      NvBuffer *buffer;

      memset(&v4l2_buf, 0, sizeof(v4l2_buf));
      memset(planes, 0, sizeof(planes));

      v4l2_buf.m.planes = planes;

      if (m_nv_dec->capture_plane.dqBuffer(v4l2_buf, &buffer, NULL, 0) < 0) {
        CLog::Log(LOGERROR, LOGVIDEO, "NVV4LCodec::GetPicture error dequeuing buffer");
      } else {
        m_buf_q.push(buffer);
      }

  }
}

bool NVV4LCodec::AddData(const DemuxPacket &packet) 
{

  m_bitstream->Convert(packet.pData, packet.iSize);

  if (!m_bitstream->CanStartDecode()) {
      return true;
  }

  struct v4l2_buffer v4l2_buf;
  struct v4l2_plane planes[NV_MAX_PLANES];
  NvBuffer *buffer;

  memset(&v4l2_buf, 0, sizeof(v4l2_buf));
  memset(planes, 0, sizeof(planes));

  v4l2_buf.m.planes = planes;


  if (queued_buffers == m_nv_dec->output_plane.getNumBuffers() ) {
      if (m_nv_dec->output_plane.dqBuffer(v4l2_buf, &buffer, NULL, -1) < 0) {
        CLog::Log(LOGERROR, LOGVIDEO, "NVV4LCodec::AddData error dequeuing empty buffer");
        return false;
      }
  } else {
      v4l2_buf.index = queued_buffers;
      buffer = m_nv_dec->output_plane.getNthBuffer(queued_buffers);

      queued_buffers++;
  }

  buffer->planes[0].bytesused = m_bitstream->GetConvertSize();
  memcpy(buffer->planes[0].data, m_bitstream->GetConvertBuffer(), buffer->planes[0].bytesused);

  v4l2_buf.m.planes[0].bytesused = buffer->planes[0].bytesused;
  
  if (m_nv_dec->output_plane.qBuffer(v4l2_buf, NULL) < 0) {
    CLog::Log(LOGERROR, LOGVIDEO, "NVV4LCodec::AddData error queuing buffer at output plane");
    return false;
  }

  return true;
};

void NVV4LCodec::Reset(void)
{

};

CDVDVideoCodec::VCReturn NVV4LCodec::GetPicture(VideoPicture *pDvdVideoPicture)
{

  if (!decoding)
      return CDVDVideoCodec::VC_BUFFER;

  if (m_buf_q.empty())
      return CDVDVideoCodec::VC_BUFFER;

  if (pDvdVideoPicture->videoBuffer) {
    NVV4L::NVV4LBuffer* b = dynamic_cast<NVV4L::NVV4LBuffer*>(pDvdVideoPicture->videoBuffer);

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[NV_MAX_PLANES];

    v4l2_buf.index = b->GetRef()->index;
    v4l2_buf.m.planes = planes;

    m_nv_dec->capture_plane.qBuffer(v4l2_buf, NULL);

    pDvdVideoPicture->videoBuffer->Release();
  }

  NvBuffer* buffer = m_buf_q.front();
  m_buf_q.pop();

  NVV4L::NVV4LBuffer *cbuffer = new NVV4L::NVV4LBuffer(0);
  cbuffer->SetRef(buffer);

  pDvdVideoPicture->videoBuffer = cbuffer;


  return CDVDVideoCodec::VCReturn::VC_PICTURE;
};

bool NVV4LCodec::Register()
{
  CDVDFactoryCodec::RegisterHWVideoCodec("nvv4l", NVV4LCodec::Create);

  CLog::Log(LOGINFO, LOGVIDEO, "NVV4LCodec::Register NVV4L registered");

  return true;
}

CDVDVideoCodec* NVV4LCodec::Create(CProcessInfo &processInfo)
{
  return new NVV4LCodec(processInfo);
}
