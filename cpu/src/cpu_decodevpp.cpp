/*############################################################################
  # Copyright (C) 2020 Intel Corporation
  #
  # SPDX-License-Identifier: MIT
  ############################################################################*/

#include "src/cpu_decodevpp.h"
#include <algorithm>
#include <memory>
#include <utility>
#include "src/cpu_workstream.h"

constexpr mfxU16 decoderChannelID = 0;

CpuDecodeVPP::CpuDecodeVPP(CpuWorkstream *session)
        : m_cpuVPP(nullptr),
          m_vppChParams(nullptr),
          m_surfOut(nullptr),
          m_numVPPCh(0),
          m_numSurfs(0),
          m_session(session) {
    m_mfxsession = reinterpret_cast<mfxSession>(m_session);
}

mfxStatus CpuDecodeVPP::InitDecodeVPP(mfxVideoParam *par,
                                      mfxVideoChannelParam **vpp_par_array,
                                      mfxU32 num_vpp_par) {
    // init decoder
    RET_ERROR(MFXVideoDECODE_Init(m_mfxsession, par));

    // init vpp, separate for reset
    return InitVPP(par, vpp_par_array, num_vpp_par);
}

mfxStatus CpuDecodeVPP::InitVPP(mfxVideoParam *par,
                                mfxVideoChannelParam **vpp_par_array,
                                mfxU32 num_vpp_par) {
    // create vpp instances as many as number of channels
    m_numVPPCh    = num_vpp_par;
    m_numSurfs    = m_numVPPCh + 1; // one more for decode at 0th location
    m_vppChParams = vpp_par_array;

    if (m_cpuVPP != nullptr) {
        delete[] m_cpuVPP;
        m_cpuVPP = nullptr;
    }

    // init vpp instance for each channels
    m_cpuVPP = new CpuVPP[m_numVPPCh];

    mfxVideoParam param = { 0 };
    memcpy_s(&param, sizeof(mfxVideoParam), par, sizeof(mfxVideoParam));
    memcpy_s(&param.vpp.In, sizeof(mfxFrameInfo), &par->mfx.FrameInfo, sizeof(mfxFrameInfo));
    param.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    for (mfxU32 i = 0; i < m_numVPPCh; i++) {
        m_cpuVPP[i].SetSession(m_session);
        memcpy_s(&param.vpp.Out,
                 sizeof(mfxFrameInfo),
                 &(vpp_par_array[i]->VPP),
                 sizeof(mfxFrameInfo));
        RET_ERROR(m_cpuVPP[i].InitVPP(&param));
    }

    // prepare mfx surfaces for 1 decode and n vpps
    if (m_surfOut) {
        for (mfxU32 i = 0; i < m_numSurfs; i++) {
            delete m_surfOut[i];
        }
        delete[] m_surfOut;

        m_surfOut = nullptr;
    }

    m_surfOut = new mfxFrameSurface1 *[m_numSurfs];
    for (mfxU32 i = 0; i < m_numSurfs; i++) {
        m_surfOut[i] = new mfxFrameSurface1;
        memset(m_surfOut[i], 0, sizeof(mfxFrameSurface1));
    }

    return MFX_ERR_NONE;
}

mfxStatus CpuDecodeVPP::Close() {
    MFXVideoDECODE_Close(m_mfxsession);

    if (m_cpuVPP) {
        delete[] m_cpuVPP;
        m_cpuVPP = nullptr;
    }

    return MFX_ERR_NONE;
}

CpuDecodeVPP::~CpuDecodeVPP() {
    Close();
}

// bs == 0 is a signal to drain
mfxStatus CpuDecodeVPP::DecodeVPPFrame(mfxBitstream *bs,
                                       mfxU32 *skip_channels,
                                       mfxU32 num_skip_channels,
                                       mfxSurfaceArray **surf_array_out) {
    if (num_skip_channels != 0) {
        RET_IF_FALSE(skip_channels, MFX_ERR_NULL_PTR);
    }

    //skip channels
    std::vector<mfxU32> skipChannels;
    if (num_skip_channels != 0) {
        skipChannels.assign(skip_channels, skip_channels + num_skip_channels);
    }

    mfxSyncPoint syncp             = { 0 };
    mfxFrameSurface1 *pWorkSurface = nullptr;

    // m_surfOut[0]   : surface for decode out
    // m_surfOut[1] ~ : surfaces for vpp out
    for (mfxU32 i = 0; i < m_numVPPCh; i++) {
        m_cpuVPP[i].GetVPPSurfaceOut(&m_surfOut[i + 1]);
    }

    // decode out from 0th channel
    RET_ERROR(
        MFXVideoDECODE_DecodeFrameAsync(m_mfxsession, bs, pWorkSurface, &m_surfOut[0], &syncp));

    //output DEC
    RAIISurfaceArray surfArray;
    mfxFrameSurface1 *decChannelSurf = m_surfOut[0];
    decChannelSurf->Info.ChannelId   = decoderChannelID;

    //output DEC channel
    if (skipChannels.end() ==
        std::find(skipChannels.begin(), skipChannels.end(), decoderChannelID)) {
        surfArray->AddSurface(decChannelSurf);
    }
    else {
        RET_ERROR(decChannelSurf->FrameInterface->Release(decChannelSurf));
    }

    // * Total number of output surfaces (m_surfOut[])
    //   = 1 decode out surface + m_numVPPCh
    //
    //   m_surfOut[0]   : reserved for decode output
    //   m_surfOut[1] ~ : for VPP output
    //
    // * There will be m_numVPPCh of mfxVideoChannelParam (m_vppChParams[])

    // execute process for vpp channels
    for (mfxU32 i = 0; i < m_numVPPCh; i++) {
        // check skip channels
        if (skipChannels.end() !=
            std::find(skipChannels.begin(), skipChannels.end(), m_vppChParams[i]->VPP.ChannelId)) {
            //skip this channel
            continue;
        }

        // vpp surface index in m_surfOut[]
        int vppSurfIndex = i + 1;

        // input is m_surfOut[0] (decode output),
        // vpp output to m_surfOut[vppSurfIndex]
        RET_ERROR(m_cpuVPP[i].ProcessFrame(m_surfOut[0], m_surfOut[vppSurfIndex], NULL));

        // update vpp output channel id
        m_surfOut[vppSurfIndex]->Info.ChannelId = m_vppChParams[i]->VPP.ChannelId;
        surfArray->AddSurface(m_surfOut[vppSurfIndex]);
    }

    if (surfArray->NumSurfaces == 0) {
        *surf_array_out = nullptr;
    }
    else {
        *surf_array_out = surfArray.ReleaseContent();
    }

    return MFX_ERR_NONE;
}

mfxU32 CpuDecodeVPP::GetVPPChannelCount(void) {
    return m_numVPPCh;
}

mfxStatus CpuDecodeVPP::GetChannelParam(mfxVideoChannelParam *par, mfxU32 channel_id) {
    bool bfound = false;

    for (mfxU32 i = 0; i < m_numVPPCh; i++) {
        if (m_vppChParams[i]->VPP.ChannelId == channel_id) {
            *par   = *(m_vppChParams)[i];
            bfound = true;
            break;
        }
    }

    return (bfound == true) ? MFX_ERR_NONE : MFX_ERR_NOT_FOUND;
}

mfxStatus CpuDecodeVPP::CheckVideoParamDecodeVPP(mfxVideoParam *in) {
    mfxStatus sts = CheckVideoParamCommon(in);
    RET_ERROR(sts);

    if (in->IOPattern != MFX_IOPATTERN_OUT_SYSTEM_MEMORY) {
        if (in->IOPattern == 0x40) { //MFX_IOPATTERN_OUT_OPAQUE_MEMORY
            return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
        }
        else {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }

    if (in->mfx.DecodedOrder)
        return MFX_ERR_UNSUPPORTED;

    if (in->NumExtParam)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    return MFX_ERR_NONE;
}

mfxStatus CpuDecodeVPP::CheckVideoChannelParamDecodeVPP(mfxVideoChannelParam **inChPar,
                                                        mfxU32 num_ch) {
    return MFX_ERR_NONE;
}

mfxStatus CpuDecodeVPP::IsSameVideoParam(mfxVideoParam *newPar, mfxVideoParam *oldPar) {
    return MFX_ERR_NONE;
}

mfxStatus CpuDecodeVPP::IsSameVideoChannelParam(mfxVideoChannelParam *newChPar,
                                                mfxVideoChannelParam *oldChPar) {
    return MFX_ERR_NONE;
}

mfxStatus CpuDecodeVPP::Reset(mfxVideoParam *par,
                              mfxVideoChannelParam **vpp_par_array,
                              mfxU32 num_vpp_par) {
    // decoder reset
    RET_ERROR(MFXVideoDECODE_Reset(m_mfxsession, par));

    // vpp part reset in decodevpp
    Close();
    return InitVPP(par, vpp_par_array, num_vpp_par);
}
