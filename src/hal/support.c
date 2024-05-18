#include "support.h"

void *encthread = NULL;
void *ispthread = NULL;
char chncount = 0;
hal_chnstate *chnstate = NULL;
hal_platform plat = HAL_PLATFORM_UNK;

bool hal_registry(unsigned int addr, unsigned int *data, hal_register_op op) {
    static int mem_fd;
    static char *loaded_area;
    static unsigned int loaded_offset;
    static unsigned int loaded_size;

    unsigned int offset = addr & 0xffff0000;
    unsigned int size = 0xffff;
    if (!addr || (loaded_area && offset != loaded_offset))
        if (munmap(loaded_area, loaded_size))
            fprintf(stderr, "hal_registry munmap error: %s (%d)\n",
                strerror(errno), errno);

    if (!addr) {
        close(mem_fd);
        return true;
    }

    if (!mem_fd && (mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
        fprintf(stderr, "can't open /dev/mem\n");
        return false;
    }

    volatile char *mapped_area;
    if (offset != loaded_offset) {
        mapped_area = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, offset);
        if (mapped_area == MAP_FAILED) {
            fprintf(stderr, "hal_registry mmap error: %s (%d)\n",
                    strerror(errno), errno);
            return false;
        }
        loaded_area = (char *)mapped_area;
        loaded_size = size;
        loaded_offset = offset;
    } else
        mapped_area = loaded_area;

    if (op & OP_READ)
        *data = *(volatile uint32_t *)(mapped_area + (addr - offset));
    if (op & OP_WRITE)
        *(volatile uint32_t *)(mapped_area + (addr - offset)) = *data;

    return true;
}

void hal_identify(void) {
    unsigned int val = 0;
    FILE *file;
    char line[200] = {0};

    if (!access("/proc/mi_modules", 0) && 
        hal_registry(0x1F003C00, &val, OP_READ))
        switch (val) {
            case 0xEF: // Macaron (6)
            case 0xF1: // Pudding (6E)
            case 0xF2: // Ispahan (6B0)
                plat = HAL_PLATFORM_I6;
                chncount = I6_VENC_CHN_NUM;
                chnstate = (hal_chnstate*)i6_state;
                encthread = i6_encoder_thread;
                return;
            case 0xF9:
                plat = HAL_PLATFORM_I6C;
                chncount = I6C_VENC_CHN_NUM;
                chnstate = (hal_chnstate*)i6c_state;
                encthread = i6c_encoder_thread;
                return;
            case 0xFB:
                plat = HAL_PLATFORM_I6F;
                chncount = I6F_VENC_CHN_NUM;
                chnstate = (hal_chnstate*)i6f_state;
                encthread = i6f_encoder_thread;
                return;
        }

    if (file = fopen("/proc/iomem", "r"))
        while (fgets(line, 200, file))
            if (strstr(line, "uart")) {
                strtol(line, (char**)(&line + 8), 16);
                break;
            }
}