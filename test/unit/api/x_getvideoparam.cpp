/*############################################################################
  # Copyright (C) 2020 Intel Corporation
  #
  # SPDX-License-Identifier: MIT
  ############################################################################*/

#include <gtest/gtest.h>
#include "vpl/mfxjpeg.h"
#include "vpl/mfxvideo.h"

/* GetVideoParam overview
   Retrieves current working parameters to the specified output structure.

MFX_ERR_NONE The function completed successfully. 

*/

TEST(EncodeGetVideoParam, InitializedEncodeReturnsParams) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam mfxEncParams = { 0 };

    mfxEncParams.mfx.CodecId                 = MFX_CODEC_JPEG;
    mfxEncParams.mfx.TargetUsage             = MFX_TARGETUSAGE_BALANCED;
    mfxEncParams.mfx.TargetKbps              = 4000;
    mfxEncParams.mfx.RateControlMethod       = MFX_RATECONTROL_VBR;
    mfxEncParams.mfx.FrameInfo.FrameRateExtN = 30;
    mfxEncParams.mfx.FrameInfo.FrameRateExtD = 1;
    mfxEncParams.mfx.FrameInfo.FourCC        = MFX_FOURCC_I420;
    mfxEncParams.mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    mfxEncParams.mfx.FrameInfo.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
    mfxEncParams.mfx.FrameInfo.CropX         = 0;
    mfxEncParams.mfx.FrameInfo.CropY         = 0;
    mfxEncParams.mfx.FrameInfo.CropW         = 128;
    mfxEncParams.mfx.FrameInfo.CropH         = 96;
    mfxEncParams.mfx.FrameInfo.Width         = 128;
    mfxEncParams.mfx.FrameInfo.Height        = 96;
    mfxEncParams.IOPattern                   = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

    sts = MFXVideoENCODE_Init(session, &mfxEncParams);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam par;
    sts = MFXVideoENCODE_GetVideoParam(session, &par);
    ASSERT_EQ(sts, MFX_ERR_NONE);
    ASSERT_EQ(128, par.mfx.FrameInfo.Width);
    ASSERT_EQ(96, par.mfx.FrameInfo.Height);
    ASSERT_EQ(MFX_RATECONTROL_VBR, par.mfx.RateControlMethod);

    sts = MFXVideoENCODE_Close(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(EncodeGetVideoParam, UninitializedEncodeReturnsNotInitialized) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam par;
    sts = MFXVideoENCODE_GetVideoParam(session, &par);
    ASSERT_EQ(sts, MFX_ERR_NOT_INITIALIZED);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(EncodeGetVideoParam, NullSessionReturnsInvalidHandle) {
    mfxStatus sts = MFXVideoENCODE_GetVideoParam(0, nullptr);
    ASSERT_EQ(sts, MFX_ERR_INVALID_HANDLE);
}

TEST(EncodeGetVideoParam, NullParamsOutReturnsErrNull) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    sts = MFXVideoENCODE_GetVideoParam(session, nullptr);
    ASSERT_EQ(sts, MFX_ERR_NULL_PTR);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(DecodeGetVideoParam, InitializedDecodeReturnsParams) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam mfxDecParams = { 0 };
    mfxDecParams.mfx.CodecId   = MFX_CODEC_HEVC;
    mfxDecParams.IOPattern     = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    mfxDecParams.mfx.FrameInfo.FourCC       = MFX_FOURCC_I420;
    mfxDecParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    mfxDecParams.mfx.FrameInfo.CropW        = 128;
    mfxDecParams.mfx.FrameInfo.CropH        = 96;
    mfxDecParams.mfx.FrameInfo.Width        = 128;
    mfxDecParams.mfx.FrameInfo.Height       = 96;

    sts = MFXVideoDECODE_Init(session, &mfxDecParams);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam testparam = { 0 };
    sts                     = MFXVideoDECODE_GetVideoParam(session, &testparam);
    ASSERT_EQ(sts, MFX_ERR_NONE);
    ASSERT_EQ(testparam.IOPattern, MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(DecodeGetVideoParam, DecodeUninitializedReturnsNotInitialized) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam testparam = { 0 };
    sts                     = MFXVideoDECODE_GetVideoParam(session, &testparam);
    ASSERT_EQ(sts, MFX_ERR_NOT_INITIALIZED);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(DecodeGetVideoParam, NullSessionReturnsInvalidHandle) {
    mfxStatus sts = MFXVideoDECODE_GetVideoParam(0, nullptr);
    ASSERT_EQ(sts, MFX_ERR_INVALID_HANDLE);
}

TEST(DecodeGetVideoParam, NullParamsOutReturnsErrNull) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    sts = MFXVideoDECODE_GetVideoParam(session, nullptr);
    ASSERT_EQ(sts, MFX_ERR_NULL_PTR);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(VPPGetVideoParam, InitializedVPPReturnsParams) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam mfxVPPParams = { 0 };

    mfxVPPParams.vpp.In.FourCC        = MFX_FOURCC_I420;
    mfxVPPParams.vpp.In.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    mfxVPPParams.vpp.In.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
    mfxVPPParams.vpp.In.FrameRateExtN = 30;
    mfxVPPParams.vpp.In.FrameRateExtD = 1;
    mfxVPPParams.vpp.In.CropW         = 128;
    mfxVPPParams.vpp.In.CropH         = 96;
    mfxVPPParams.vpp.In.Width         = 128;
    mfxVPPParams.vpp.In.Height        = 96;

    mfxVPPParams.vpp.Out.FourCC        = MFX_FOURCC_I420;
    mfxVPPParams.vpp.Out.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    mfxVPPParams.vpp.Out.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
    mfxVPPParams.vpp.Out.FrameRateExtN = 30;
    mfxVPPParams.vpp.Out.FrameRateExtD = 1;
    mfxVPPParams.vpp.Out.CropW         = 320;
    mfxVPPParams.vpp.Out.CropH         = 240;
    mfxVPPParams.vpp.Out.Width         = 320;
    mfxVPPParams.vpp.Out.Height        = 240;

    mfxVPPParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    sts = MFXVideoVPP_Init(session, &mfxVPPParams);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam par;
    sts = MFXVideoVPP_GetVideoParam(session, &par);
    ASSERT_EQ(sts, MFX_ERR_NONE);
    ASSERT_EQ(128, par.vpp.In.Width);
    ASSERT_EQ(96, par.vpp.In.Height);
    ASSERT_EQ(320, par.vpp.Out.Width);
    ASSERT_EQ(240, par.vpp.Out.Height);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(VPPGetVideoParam, VPPUninitializedReturnsNotInitialized) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam par;
    sts = MFXVideoVPP_GetVideoParam(session, &par);
    ASSERT_EQ(sts, MFX_ERR_NOT_INITIALIZED);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(VPPGetVideoParam, NullSessionReturnsInvalidHandle) {
    mfxStatus sts = MFXVideoVPP_GetVideoParam(0, nullptr);
    ASSERT_EQ(sts, MFX_ERR_INVALID_HANDLE);
}

TEST(VPPGetVideoParam, NullParamsOutReturnsErrNull) {
    mfxVersion ver = {};
    mfxSession session;
    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    sts = MFXVideoVPP_GetVideoParam(session, nullptr);
    ASSERT_EQ(sts, MFX_ERR_NULL_PTR);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);
}

TEST(DecodeVPPGetChannelParam, InitializedDecodeVPPReturnsParams) {
    mfxVersion ver = {};
    mfxSession session;
    ver.Major = 2;
    ver.Minor = 1;

    mfxStatus sts = MFXInit(MFX_IMPL_SOFTWARE, &ver, &session);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoParam mfxDecParams = { 0 };
    mfxDecParams.mfx.CodecId   = MFX_CODEC_HEVC;
    mfxDecParams.IOPattern     = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    mfxDecParams.mfx.FrameInfo.FourCC        = MFX_FOURCC_I420;
    mfxDecParams.mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    mfxDecParams.mfx.FrameInfo.CropW         = 128;
    mfxDecParams.mfx.FrameInfo.CropH         = 96;
    mfxDecParams.mfx.FrameInfo.Width         = 128;
    mfxDecParams.mfx.FrameInfo.Height        = 96;
    mfxDecParams.mfx.FrameInfo.FrameRateExtN = 30;
    mfxDecParams.mfx.FrameInfo.FrameRateExtD = 1;

    mfxVideoChannelParam *mfxVPPChParams = new mfxVideoChannelParam;
    memset(mfxVPPChParams, 0, sizeof(mfxVideoChannelParam));

    // scaled output to 320x240
    mfxVPPChParams->VPP.FourCC        = MFX_FOURCC_I420;
    mfxVPPChParams->VPP.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    mfxVPPChParams->VPP.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
    mfxVPPChParams->VPP.FrameRateExtN = 30;
    mfxVPPChParams->VPP.FrameRateExtD = 1;
    mfxVPPChParams->VPP.CropW         = 320;
    mfxVPPChParams->VPP.CropH         = 240;
    mfxVPPChParams->VPP.Width         = 320;
    mfxVPPChParams->VPP.Height        = 240;
    mfxVPPChParams->VPP.ChannelId     = 1;
    mfxVPPChParams->Protected         = 0;
    mfxVPPChParams->IOPattern   = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    mfxVPPChParams->ExtParam    = NULL;
    mfxVPPChParams->NumExtParam = 0;

    sts = MFXVideoDECODE_VPP_Init(session, &mfxDecParams, &mfxVPPChParams, 1);
    ASSERT_EQ(sts, MFX_ERR_NONE);

    mfxVideoChannelParam par = { 0 };
    sts                      = MFXVideoDECODE_VPP_GetChannelParam(session, &par, 1);
    ASSERT_EQ(sts, MFX_ERR_NONE);
    ASSERT_EQ(320, par.VPP.Width);
    ASSERT_EQ(240, par.VPP.Height);

    sts = MFXClose(session);
    EXPECT_EQ(sts, MFX_ERR_NONE);

    delete mfxVPPChParams;
}