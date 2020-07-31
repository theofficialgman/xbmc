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
#include <libavcodec/codec_id.h>

extern "C" {
#include <libavutil/opt.h>
}

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
        m_codecName = "pt-dsd";
        break;

      case 16:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF ? AE_FMT_DSD_U16_LE : AE_FMT_DSD_U16_BE;
        m_codecName = "pt-dsd";
        break;

      case 32:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF ? AE_FMT_DSD_U32_LE : AE_FMT_DSD_U32_BE;
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
        m_codecName = "pt-dsd";
        break;

      case 16:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF_PLANAR ? AE_FMT_DSD_U16_LE : AE_FMT_DSD_U16_BE;
        m_codecName = "pt-dsd8";
        break;

      case 32:
        m_format.m_dataFormat = hints.codec == AV_CODEC_ID_DSD_LSBF_PLANAR ? AE_FMT_DSD_U32_LE : AE_FMT_DSD_U32_BE;
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

  unsigned char *pData(const_cast<uint8_t*>(packet.pData));
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
  }

  if (pData)
  {
    if (m_bufferSize < iSize)
    {
      m_bufferSize = std::max(61440, static_cast<int>(m_bufferSize));
      m_buffer = static_cast<uint8_t*>(realloc(m_buffer, m_bufferSize));
    }

    memcpy(m_buffer, pData, iSize);
    m_dataSize = iSize;
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
  frame.format.m_dataFormat = m_format.m_dataFormat;
  frame.format.m_channelLayout = m_format.m_channelLayout;
  frame.framesize = frame.format.m_channelLayout.Count();
  if(frame.framesize == 0)
    return;

  frame.nb_frames = bytes/frame.framesize;
  frame.framesOut = 0;
  frame.planes = 1;

  frame.bits_per_sample = 1;
  frame.format.m_sampleRate = m_format.m_sampleRate;
  frame.matrix_encoding = GetMatrixEncoding();
  frame.audio_service_type = GetAudioServiceType();
  frame.profile = GetProfile();

  // compute duration.
  if (frame.format.m_sampleRate)
    frame.duration = ((double)frame.nb_frames * DVD_TIME_BASE) / frame.format.m_sampleRate;
  else
    frame.duration = 0.0;

  frame.pts = m_currentPts;
  frame.hasDownmix = m_hasDownmix;

  if (frame.hasDownmix)
  {
    frame.centerMixLevel = m_downmixInfo.center_mix_level;
  }
}

int CDVDAudioCodecDSD::GetData(uint8_t** dst)
{
  if (!m_dataSize)
    AddData(DemuxPacket());

  m_format.m_dataFormat = GetDataFormat();
  m_format.m_channelLayout = GetChannelMap();
  m_format.m_sampleRate = GetSampleRate();
  m_format.m_frameSize = m_format.m_channelLayout.Count();

  *dst = m_buffer;

  size_t bytes(m_bufferSize);
  m_bufferSize = 0;

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
  return m_sampleRate;
}

enum AEDataFormat CDVDAudioCodecDSD::GetDataFormat()
{
  return m_format.m_dataFormat;
}

int CDVDAudioCodecDSD::GetBitRate()
{
  return 0;
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

static unsigned count_bits(int64_t value)
{
  unsigned bits = 0;
  for(;value;++bits)
    value &= value - 1;
  return bits;
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
