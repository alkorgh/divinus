#pragma once

#include "m6_common.h"

typedef struct {
    unsigned int rev;
    unsigned int size;
    unsigned char data[64];
} m6_isp_iqver;

typedef struct {
    unsigned int sensorId;
    m6_isp_iqver iqVer;
    unsigned int sync3A;
} m6_isp_chn;

typedef struct {
    m6_common_hdr hdr;
    // Accepts values from 0-7
    int level3DNR;
    char mirror;
    char flip;
    // Represents 90-degree arcs
    int rotate;
} m6_isp_para;

typedef struct {
    m6_common_rect crop;
    m6_common_pixfmt pixFmt;
} m6_isp_port;

typedef struct {
    void *handle, *handleCus3a, *handleIspAlgo;
    
    int (*fnCreateDevice)(int device, unsigned int *combo);
    int (*fnDestroyDevice)(int device);

    int (*fnCreateChannel)(int device, int channel, m6_isp_chn *config);
    int (*fnDestroyChannel)(int device, int channel);
    int (*fnLoadChannelConfig)(int device, int channel, char *path, unsigned int key);
    int (*fnSetChannelParam)(int device, int channel, m6_isp_para *config);
    int (*fnStartChannel)(int device, int channel);
    int (*fnStopChannel)(int device, int channel);

    int (*fnDisablePort)(int device, int channel, int port);
    int (*fnEnablePort)(int device, int channel, int port);
    int (*fnSetPortConfig)(int device, int channel, int port, m6_isp_port *config);

    int (*fnSetColorToGray)(int device, int channel, char *enable);
} m6_isp_impl;

static int m6_isp_load(m6_isp_impl *isp_lib) {
    if (!(isp_lib->handleIspAlgo = dlopen("libispalgo.so", RTLD_LAZY | RTLD_GLOBAL)))
        HAL_ERROR("m6_isp", "Failed to load dependency library!\nError: %s\n", dlerror());


    if (!(isp_lib->handleCus3a = dlopen("libcus3a.so", RTLD_LAZY | RTLD_GLOBAL)))
        return EXIT_FAILURE;

    if (!(isp_lib->handle = dlopen("libmi_isp.so", RTLD_LAZY | RTLD_GLOBAL)))
        return EXIT_FAILURE;

    if (!(isp_lib->fnCreateDevice = (int(*)(int device, unsigned int *combo))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_CreateDevice")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnDestroyDevice = (int(*)(int device))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_DestoryDevice")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnCreateChannel = (int(*)(int device, int channel, m6_isp_chn *config))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_CreateChannel")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnDestroyChannel = (int(*)(int device, int channel))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_DestroyChannel")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnLoadChannelConfig = (int(*)(int device, int channel, char *path, unsigned int key))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_ALGO_API_CmdLoadBinFile")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnSetChannelParam = (int(*)(int device, int channel, m6_isp_para *config))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_SetChnParam")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnStartChannel = (int(*)(int device, int channel))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_StartChannel")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnStopChannel = (int(*)(int device, int channel))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_StopChannel")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnDisablePort = (int(*)(int device, int channel, int port))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_DisableOutputPort")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnEnablePort = (int(*)(int device, int channel, int port))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_EnableOutputPort")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnSetPortConfig = (int(*)(int device, int channel, int port, m6_isp_port *config))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_SetOutputPortParam")))
        return EXIT_FAILURE;

    if (!(isp_lib->fnSetColorToGray = (int(*)(int device, int channel, char *enable))
        hal_symbol_load("m6_isp", isp_lib->handle, "MI_ISP_IQ_SetColorToGray")))
        return EXIT_FAILURE;
    
    return EXIT_SUCCESS;
}

static void m6_isp_unload(m6_isp_impl *isp_lib) {
    if (isp_lib->handle) dlclose(isp_lib->handle);
    isp_lib->handle = NULL;
    if (isp_lib->handleCus3a) dlclose(isp_lib->handleCus3a);
    isp_lib->handleCus3a = NULL;
    if (isp_lib->handleIspAlgo) dlclose(isp_lib->handleIspAlgo);
    isp_lib->handleIspAlgo = NULL;
    memset(isp_lib, 0, sizeof(*isp_lib));
}