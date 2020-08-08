/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDAudioCodecDSD.h"
#include "ServiceBroker.h"
#include "../../DVDStreamInfo.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "DVDCodecs/DVDCodecs.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include <cstddef>
#include <string>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_id.h>

extern "C" {
#include <libavutil/opt.h>
}

static constexpr uint32_t DSD_8To32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) noexcept
{
  return uint32_t(d) | (uint32_t(c) << 8) | (uint32_t(b) << 16) | (uint32_t(a) << 24);
}

static constexpr uint32_t GenericByteSwap32(uint32_t value) noexcept
{
  return (value >> 24) | ((value >> 8) & 0x0000ff00) | ((value << 8) & 0x00ff0000) | (value << 24);
}

 const uint8_t bit_reverse[256] = {
 0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
 0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
 0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
 0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
 0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
 0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
 0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
 0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
 0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
 0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
 0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
 0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
 0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
 0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
 0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
 0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF,
 };


CDVDAudioCodecDSD::CDVDAudioCodecDSD(CProcessInfo &processInfo) : CDVDAudioCodec(processInfo)
{
  m_channels = 0;
  m_layout = 0;

  m_eof = false;
}

CDVDAudioCodecDSD::~CDVDAudioCodecDSD()
{
  Dispose();
}

bool CDVDAudioCodecDSD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (hints.cryptoSession)
  {
    CLog::Log(LOGERROR,"CDVDAudioCodecDSD::Open() CryptoSessions unsupported!");
    return false;
  }

  if (hints.codec == AV_CODEC_ID_DSD_LSBF || hints.codec == AV_CODEC_ID_DSD_MSBF) 
  {
    switch (hints.bitspersample) {
      case 8:
        m_format.m_dataFormat = AE_FMT_DSD_U8;
        m_planar = false;
        m_codecName = "pt-dsd";
        break;

      case 16:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF ? AE_FMT_DSD_U16_LE : AE_FMT_DSD_U16_BE;
        m_planar = false;
        m_codecName = "pt-dsd";
        break;

      case 32:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF ? AE_FMT_DSD_U32_LE : AE_FMT_DSD_U32_BE;
        m_planar = false;
        m_codecName = "pt-dsd";
        break;
      default:
        return false;
    }
  } 
  else if (hints.codec == AV_CODEC_ID_DSD_LSBF_PLANAR || hints.codec == AV_CODEC_ID_DSD_MSBF_PLANAR) 
  {
    switch (hints.bitspersample) {
      case 8:
        m_format.m_dataFormat = AE_FMT_DSD_U8;
        m_planar = true;
        m_codecName = "pt-dsd";
        break;

      case 16:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF_PLANAR ? AE_FMT_DSD_U16_LE : AE_FMT_DSD_U16_BE;
        m_planar = true;
        m_codecName = "pt-dsd8";
        break;

      case 32:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF_PLANAR ? AE_FMT_DSD_U32_LE : AE_FMT_DSD_U32_BE;
        m_planar = true;
        m_codecName = "pt-dsd6";
        break;
      default:
        return false;
    }
  } 
  else 
  {
    return false;
  }


  m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
  m_hasDownmix = false;

  m_channels = hints.channels;
  m_sampleRate = hints.samplerate;
  m_dataSize = 0;

  CLog::Log(LOGINFO, "CDVDAudioCodecDSD::Open() Successful opened audio DSD passthrough: %s", m_codecName);

  return true;
}

void CDVDAudioCodecDSD::Dispose()
{
  if (m_bufferSize != 0)
  {
    free(m_buffer);
    m_buffer = NULL;
    m_bufferSize = 0;
  }

  m_dataSize = 0;
}

bool CDVDAudioCodecDSD::AddData(const DemuxPacket &packet)
{
  if (m_eof)
  {
    Reset();
  }

  if (m_dataSize)
  {
    return false;
  }

  uint8_t *pData(const_cast<uint8_t*>(packet.pData));
  int iSize(packet.iSize);

  if (pData)
  {
    if (m_currentPts == DVD_NOPTS_VALUE)
    {
      if (m_nextPts != DVD_NOPTS_VALUE)
      {
        m_currentPts = m_nextPts;
        m_nextPts = packet.pts;
      }
      else if (packet.pts != DVD_NOPTS_VALUE)
      {
        m_currentPts = packet.pts;
      }
    }
    else
    {
      m_nextPts = packet.pts;
    }

    if (m_bufferSize < iSize * 2)
    {
      m_bufferSize = iSize * 2;
      m_buffer = static_cast<uint8_t*>(realloc(m_buffer, m_bufferSize));
    }



    uint32_t *p { reinterpret_cast<uint32_t*>(m_buffer) }; 
    bool marker { true };

    // repackage planar dataformat to single plane
    for (size_t i = 0; i < iSize / m_channels / 4; ++i)
    {
      for (int c = 0; c < m_channels; ++c)
      {
        const size_t fp = (iSize / m_channels) * c + (i * 4);

        *p++ = GenericByteSwap32(DSD_8To32(bit_reverse[pData[fp]],
                         bit_reverse[pData[fp + 1]],
                         bit_reverse[pData[fp + 2]],
                         bit_reverse[pData[fp + 3]]));

        /*
         **p++ = DSD_8To32( marker ? 0x05 : 0xFA, 
         *                 pData[fp], 
         *                 pData[fp+1],
         *                 0x00);
         */

        m_dataSize += 4;
      }

      marker = ! marker;
    }

  }

  return true;
}

void CDVDAudioCodecDSD::GetData(DVDAudioFrame &frame)
{
  frame.nb_frames = 0;

  int bytes = GetData(frame.data);
  if (!bytes)
  {
    return;
  }

  frame.passthrough = false;
  frame.format.m_dataFormat = GetDataFormat();
  frame.format.m_channelLayout = m_format.m_channelLayout;
  frame.framesize = (CAEUtil::DataFormatToBits(frame.format.m_dataFormat) >> 3) * frame.format.m_channelLayout.Count();

  if(frame.framesize == 0)
    return;

  frame.nb_frames = bytes / frame.framesize;
  frame.framesOut = 0;
  frame.planes = 1;

  frame.bits_per_sample = CAEUtil::DataFormatToBits(frame.format.m_dataFormat);
  frame.format.m_sampleRate = GetSampleRate();
  frame.matrix_encoding = GetMatrixEncoding();
  frame.audio_service_type = GetAudioServiceType();
  frame.profile = GetProfile();

  // compute duration.
  if (frame.format.m_sampleRate)
    frame.duration = ((double)frame.nb_frames * DVD_TIME_BASE) / frame.format.m_sampleRate;
  else
    frame.duration = 0.0; 

  frame.pts = m_currentPts;
  frame.hasDownmix = false;
}

int CDVDAudioCodecDSD::GetData(uint8_t** dst)
{

  if (!m_dataSize)
    return 0;

  m_format.m_dataFormat = GetDataFormat();
  m_format.m_channelLayout = GetChannelMap();
  m_format.m_sampleRate = GetSampleRate();
  m_format.m_frameSize = (CAEUtil::DataFormatToBits(m_format.m_dataFormat) >> 3) * m_channels;
  
  dst[0] = m_buffer;

  size_t bytes(m_dataSize);
  m_dataSize = 0;

  return bytes;
}

void CDVDAudioCodecDSD::Reset()
{
  m_eof = false;
  Dispose();
}

int CDVDAudioCodecDSD::GetChannels()
{
  return m_channels;
}

int CDVDAudioCodecDSD::GetSampleRate()
{
  return m_sampleRate / 4;
}

enum AEDataFormat CDVDAudioCodecDSD::GetDataFormat()
{
//  return AE_FMT_S32NE;
//  return m_format.m_dataFormat;
  return AE_FMT_DSD_U32_BE;
}

int CDVDAudioCodecDSD::GetBitRate()
{
  return m_format.m_sampleRate * m_format.m_frameSize * 8;
}

enum AVMatrixEncoding CDVDAudioCodecDSD::GetMatrixEncoding()
{
  return m_matrixEncoding;
}

enum AVAudioServiceType CDVDAudioCodecDSD::GetAudioServiceType()
{
  return AV_AUDIO_SERVICE_TYPE_MAIN;
}

int CDVDAudioCodecDSD::GetProfile()
{
  return 0;
}

void CDVDAudioCodecDSD::BuildChannelMap()
{
  m_channelLayout.Reset();

  /*
   *if (layout & AV_CH_FRONT_LEFT           ) m_channelLayout += AE_CH_FL  ;
   *if (layout & AV_CH_FRONT_RIGHT          ) m_channelLayout += AE_CH_FR  ;
   *if (layout & AV_CH_FRONT_CENTER         ) m_channelLayout += AE_CH_FC  ;
   *if (layout & AV_CH_LOW_FREQUENCY        ) m_channelLayout += AE_CH_LFE ;
   *if (layout & AV_CH_BACK_LEFT            ) m_channelLayout += AE_CH_BL  ;
   *if (layout & AV_CH_BACK_RIGHT           ) m_channelLayout += AE_CH_BR  ;
   *if (layout & AV_CH_FRONT_LEFT_OF_CENTER ) m_channelLayout += AE_CH_FLOC;
   *if (layout & AV_CH_FRONT_RIGHT_OF_CENTER) m_channelLayout += AE_CH_FROC;
   *if (layout & AV_CH_BACK_CENTER          ) m_channelLayout += AE_CH_BC  ;
   *if (layout & AV_CH_SIDE_LEFT            ) m_channelLayout += AE_CH_SL  ;
   *if (layout & AV_CH_SIDE_RIGHT           ) m_channelLayout += AE_CH_SR  ;
   *if (layout & AV_CH_TOP_CENTER           ) m_channelLayout += AE_CH_TC  ;
   *if (layout & AV_CH_TOP_FRONT_LEFT       ) m_channelLayout += AE_CH_TFL ;
   *if (layout & AV_CH_TOP_FRONT_CENTER     ) m_channelLayout += AE_CH_TFC ;
   *if (layout & AV_CH_TOP_FRONT_RIGHT      ) m_channelLayout += AE_CH_TFR ;
   *if (layout & AV_CH_TOP_BACK_LEFT        ) m_channelLayout += AE_CH_BL  ;
   *if (layout & AV_CH_TOP_BACK_CENTER      ) m_channelLayout += AE_CH_BC  ;
   *if (layout & AV_CH_TOP_BACK_RIGHT       ) m_channelLayout += AE_CH_BR  ;
   */

  m_channelLayout += AE_CH_FL;
  m_channelLayout += AE_CH_FR;
}

CAEChannelInfo CDVDAudioCodecDSD::GetChannelMap()
{
  BuildChannelMap();
  return m_channelLayout;
}


