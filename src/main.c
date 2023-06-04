#include <drm/drm.h>
#include <drm/drm_fourcc.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <printf.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int fd = open("/dev/dri/card0", O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        printf("Failed to open /dev/dri/card0, result = %d\n", fd);
        return EXIT_FAILURE;
    }

    struct drm_mode_card_res res = {};
    if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0)
    {
        printf("Failed to ioctl DRM_IOCTL_MODE_GETRESOURCES, result = %d\n", errno);
        return EXIT_FAILURE;
    }

    if (res.count_fbs)
    {
        res.fb_id_ptr = (uint64_t)calloc(res.count_fbs, sizeof(uint32_t));
        if (!res.fb_id_ptr)
        {
            printf("Failed to calloc res.fb_id_ptr, result = %d\n", errno);
            return EXIT_FAILURE;
        }
    }

    if (res.count_crtcs)
    {
        res.crtc_id_ptr = (uint64_t)calloc(res.count_crtcs, sizeof(uint32_t));
        if (!res.crtc_id_ptr)
        {
            printf("Failed to calloc res.crtc_id_ptr, result = %d\n", errno);
            return EXIT_FAILURE;
        }
    }

    if (res.count_connectors)
    {
        res.connector_id_ptr = (uint64_t)calloc(res.count_connectors, sizeof(uint32_t));
        if (!res.connector_id_ptr)
        {
            printf("Failed to calloc res.connector_id_ptr, result = %d\n", errno);
            return EXIT_FAILURE;
        }
    }

    if (res.count_encoders)
    {
        res.encoder_id_ptr = (uint64_t)calloc(res.count_encoders, sizeof(uint32_t));
        if (!res.encoder_id_ptr)
        {
            printf("Failed to calloc res.encoder_id_ptr, result = %d\n", errno);
            return EXIT_FAILURE;
        }
    }

    if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0)
    {
        printf("Failed to ioctl DRM_IOCTL_MODE_GETRESOURCES, result = %d\n", errno);
        return EXIT_FAILURE;
    }

    uint32_t *connectors = (uint32_t *)res.connector_id_ptr;
    for (uint32_t i = 0; i < res.count_connectors; i++)
    {
        struct drm_mode_get_connector connector = {};
        connector.connector_id = connectors[i];

        if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) < 0)
        {
            printf("Failed to ioctl DRM_IOCTL_MODE_GETCONNECTOR connector = %d, result = %d\n", i,
                   errno);
            return EXIT_FAILURE;
        }

        if (connector.count_props)
        {
            connector.props_ptr = (uint64_t)calloc(connector.count_props, sizeof(uint32_t));
            if (!connector.props_ptr)
            {
                printf("Failed to calloc connector.props_ptr, result = %d\n", errno);
                return EXIT_FAILURE;
            }

            connector.prop_values_ptr = (uint64_t)calloc(connector.count_props, sizeof(uint64_t));
            if (!connector.prop_values_ptr)
            {
                printf("Failed to calloc connector.prop_values_ptr, result = %d\n", errno);
                return EXIT_FAILURE;
            }
        }

        if (connector.count_modes)
        {
            connector.modes_ptr = (uint64_t)calloc(connector.count_modes, sizeof(struct drm_mode_modeinfo));
            if (!connector.modes_ptr)
            {
                printf("Failed to calloc connector.modes_ptr, result = %d\n", errno);
                return EXIT_FAILURE;
            }
        }

        if (connector.count_encoders)
        {
            connector.encoders_ptr = (uint64_t)calloc(connector.count_encoders, sizeof(uint32_t));
            if (!connector.encoders_ptr)
            {
                printf("Failed to calloc connector.encoders_ptr, result = %d\n", errno);
                return EXIT_FAILURE;
            }
        }

        if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) < 0)
        {
            printf("Failed to ioctl DRM_IOCTL_MODE_GETCONNECTOR connector = %d, result = %d\n", i,
                   errno);
            return EXIT_FAILURE;
        }

        if (connector.connection != 1 && connector.count_modes <= 0)
        {
            printf("Ignoring connector number = %d, connection = %d\n", i, connector.connection);
            continue;
        }

        uint32_t *encoders = (uint32_t *)connector.encoders_ptr;
        for (uint32_t j = 0; j < connector.count_encoders; j++)
        {
            struct drm_mode_get_encoder encoder = {};
            encoder.encoder_id = encoders[j];

            if (ioctl(fd, DRM_IOCTL_MODE_GETENCODER, &encoder) < 0)
            {
                printf("Failed to ioctl DRM_IOCTL_MODE_GETENCODER encoder = %d, result = %d\n", j,
                       errno);
                return EXIT_FAILURE;
            }

            uint32_t *crtcs = (uint32_t *)res.crtc_id_ptr;
            struct drm_mode_modeinfo *modes = (struct drm_mode_modeinfo *)connector.modes_ptr;
            for (uint32_t k = 0; k < res.count_crtcs; k++)
            {
                uint32_t bit = 1 << k;
                if ((encoder.possible_crtcs & bit) == 0)
                    continue;

                struct drm_mode_modeinfo mode = modes[0];

                struct drm_mode_create_dumb dumb = {};
                dumb.width = mode.hdisplay;
                dumb.height = mode.vdisplay;
                dumb.bpp = 32;

                if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb) < 0)
                {
                    printf("Failed to ioctl DRM_IOCTL_MODE_CREATE_DUMB, result = %d\n", errno);
                    return EXIT_FAILURE;
                }

                uint32_t handles[4] = {dumb.handle};
                uint32_t strides[4] = {dumb.pitch};
                uint32_t offsets[4] = {0};

                struct drm_mode_fb_cmd2 cmd = {};
                cmd.width = dumb.width;
                cmd.height = dumb.height;
                cmd.pixel_format = DRM_FORMAT_XRGB8888;

                memcpy(cmd.handles, handles, 4 * sizeof(handles[0]));
                memcpy(cmd.pitches, strides, 4 * sizeof(strides[0]));
                memcpy(cmd.offsets, offsets, 4 * sizeof(offsets[0]));

                if (ioctl(fd, DRM_IOCTL_MODE_ADDFB2, &cmd) < 0)
                {
                    printf("Failed to ioctl DRM_IOCTL_MODE_ADDFB2, result = %d\n", errno);
                    return EXIT_FAILURE;
                }

                struct drm_mode_map_dumb map = {};
                map.handle = dumb.handle;

                if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0)
                {
                    printf("Failed to ioctl DRM_IOCTL_MODE_MAP_DUMB, result = %d\n", errno);
                    return EXIT_FAILURE;
                }

                uint8_t *buffer = (uint8_t *)mmap(0, dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map.offset);
                if (buffer == MAP_FAILED)
                {
                    printf("Failed to mmap, result = %d\n", errno);
                    return EXIT_FAILURE;
                }
                memset(buffer, 0xFF, dumb.size);

                struct drm_mode_crtc crtc = {};
                crtc.crtc_id = crtcs[k];
                if (ioctl(fd, DRM_IOCTL_MODE_GETCRTC, &crtc) < 0)
                {
                    printf("Failed to ioctl DRM_IOCTL_MODE_GETCRTC, result = %d\n", errno);
                    return EXIT_FAILURE;
                }

                crtc.fb_id = cmd.fb_id;
                crtc.set_connectors_ptr = (uint64_t)&connector.connector_id;
                crtc.count_connectors = 1;

                memcpy(&crtc.mode, &mode, sizeof(struct drm_mode_modeinfo));
                crtc.mode_valid = 1;

                if (ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &crtc) < 0)
                {
                    printf("Failed to ioctl DRM_IOCTL_MODE_SETCRTC, result = %d\n", errno);
                    return EXIT_FAILURE;
                }
            }
        }
    }
}
