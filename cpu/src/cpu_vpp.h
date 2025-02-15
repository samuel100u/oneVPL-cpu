/*############################################################################
  # Copyright (C) 2020 Intel Corporation
  #
  # SPDX-License-Identifier: MIT
  ############################################################################*/

#ifndef CPU_SRC_CPU_VPP_H_
#define CPU_SRC_CPU_VPP_H_

#include <memory>
#include <vector>
#include "src/cpu_common.h"
#include "src/cpu_frame_pool.h"
#include "src/frame_lock.h"

typedef enum {
    VPL_VPP_CSC       = 1,
    VPL_VPP_SCALE     = 2,
    VPL_VPP_CROP      = 4,
    VPL_VPP_COMPOSITE = 8,
    VPL_VPP_SHARP     = 16,
    VPL_VPP_BLUR      = 32
} eVPPfunction;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
} Rect;

typedef struct {
    uint32_t vpp_func;

    uint32_t src_width; // buffersrc
    uint32_t src_height; // buffersrc
    AVPixelFormat src_pixel_format; // buffersrc
    uint32_t src_fr_num;
    uint32_t src_fr_den;
    uint32_t src_shift;

    uint32_t dst_width; // scale
    uint32_t dst_height; // scale
    AVPixelFormat dst_pixel_format; // csc
    uint32_t dst_fr_num;
    uint32_t dst_fr_den;
    uint32_t dst_shift;

    Rect src_rc; // crop, composite
    Rect dst_rc; // scale, composite
    double value; // sharp, blur
} VPPBaseConfig;

class CpuWorkstream;

class CpuVPP {
public:
    CpuVPP();
    ~CpuVPP();

    static mfxStatus VPPQuery(mfxVideoParam *in, mfxVideoParam *out);
    static mfxStatus VPPQueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request);

    mfxStatus InitVPP(mfxVideoParam *par);
    mfxStatus ProcessFrame(mfxFrameSurface1 *surface_in,
                           mfxFrameSurface1 *surface_out,
                           mfxExtVppAuxData *aux);
    mfxStatus GetVideoParam(mfxVideoParam *par);
    mfxStatus GetVPPSurface(mfxFrameSurface1 **surface);
    mfxStatus GetVPPSurfaceOut(mfxFrameSurface1 **surface);
    mfxStatus IsSameVideoParam(mfxVideoParam *newPar, mfxVideoParam *oldPar);
    void SetSession(CpuWorkstream *session);

private:
    char m_vpp_filter_desc[1024];
    AVFilterGraph *m_vpp_graph;
    AVFilterContext *m_buffersrc_ctx;
    AVFilterContext *m_buffersink_ctx;
    FrameLock m_input_locker;
    AVFrame *m_avVppFrameOut;

    mfxU32 m_vppInFormat;
    mfxU32 m_vppInWidth;
    mfxU32 m_vppInHeight;

    mfxU32 m_vppOutFormat;
    mfxU32 m_vppOutWidth;
    mfxU32 m_vppOutHeight;

    mfxU32 m_vppFunc;
    mfxVideoParam m_param;
    std::unique_ptr<CpuFramePool> m_vppSurfacesIn;
    std::unique_ptr<CpuFramePool> m_vppSurfacesOut;

    bool InitFilters(void);
    void CloseFilterPads(AVFilterInOut *src_out, AVFilterInOut *sink_in);
    static mfxStatus CheckIOPattern_AndSetIOMemTypes(mfxU16 IOPattern,
                                                     mfxU16 *pInMemType,
                                                     mfxU16 *pOutMemType);
    static mfxStatus ValidateVPPParams(mfxVideoParam *par, bool canCorrect);
    static mfxStatus CheckFrameInfo(mfxFrameInfo *info);

    static bool IsConfigurable(mfxU32 filterId);
    static bool IsFilterFound(const mfxU32 *pList, mfxU32 len, mfxU32 filterName);
    static size_t GetConfigSize(mfxU32 filterId);
    static mfxStatus ExtendedQuery(mfxU32 filterName, mfxExtBuffer *pHint);
    static mfxU32 GetFilterIndex(mfxU32 *pList, mfxU32 len, mfxU32 filterName);
    static bool CheckDoUseCompatibility(mfxU32 filterName);

    void GetDoUseFilterList(mfxVideoParam *par, mfxU32 **ppList, mfxU32 *pLen);
    void GetConfigurableFilterList(mfxVideoParam *par, mfxU32 *pList, mfxU32 *pLen);
    double CalculateUMCFramerate(mfxU32 FrameRateExtN, mfxU32 FrameRateExtD);
    void ReorderPipelineListForQuality(std::vector<mfxU32> &pipelineList);
    void ReorderPipelineListForSpeed(mfxVideoParam *videoParam, std::vector<mfxU32> &pipelineList);
    void ShowPipeline(std::vector<mfxU32> pipelineList);
    mfxStatus GetPipelineList(mfxVideoParam *videoParam,
                              std::vector<mfxU32> &pipelineList,
                              bool bExtended);
    mfxStatus CheckIOPattern(mfxVideoParam *par);
    bool GetExtParamList(mfxVideoParam *par, mfxU32 *pList, mfxU32 *pLen);
    mfxStatus GetFilterParam(mfxVideoParam *par, mfxU32 filterName, mfxExtBuffer **ppHint);
    void GetDoNotUseFilterList(mfxVideoParam *par, mfxU32 **ppList, mfxU32 *pLen);
    bool CheckFilterList(mfxU32 *pList, mfxU32 count, bool bDoUseTable);
    mfxStatus CheckExtParam(mfxExtBuffer **ppExtParam, mfxU16 count);
    bool NeedWAForAlignment(mfxFrameInfo *fi, int *linesize);

    CpuWorkstream *m_session;

    /* copy not allowed */
    CpuVPP(const CpuVPP &);
    CpuVPP &operator=(const CpuVPP &);
};

#endif // CPU_SRC_CPU_VPP_H_
