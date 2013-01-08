/*
 *
 * Dumb userspace USB Audio receiver
 * Copyright 2012 Joel Stanley <joel@jms.id.au>
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include "freedv_usb.h"

/* TI PCM2900C Audio CODEC default VID/PID. */
#define VID 0x08bb
#define PID 0x29c0

/* PCM stereo AudioStreaming endpoint. */
#define EP_ISO_IN	0x84
#define IFACE_NUM   2

/* We queue many transfers to ensure no packets are missed. */
#define NUM_TRANSFERS 10
/* Each transfer will have a max of NUM_PACKETS packets. */
#define NUM_PACKETS 10
#define PACKET_SIZE 192

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGD(...) \
    __android_log_print(ANDROID_LOG_DEBUG, "FreedvUsbNative", __VA_ARGS__)
#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, "FreedvUsbNative", __VA_ARGS__)
#else
#define LOGD(...) fprintf(stderr, __VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

#if !defined(ANDROID)
static int logfd;
#endif

static struct libusb_device_handle *devh = NULL;
static int outfd = -1;

bool is_setup = false;

static void transfer_cb(struct libusb_transfer *xfr) {
    int rc = 0;
    int len = 0;
    unsigned int i;

    /* All packets are 192 bytes. */
    uint8_t* recv = malloc(PACKET_SIZE * xfr->num_iso_packets);
    uint8_t* recv_next = recv;

    for (i = 0; i < xfr->num_iso_packets; i++) {
        struct libusb_iso_packet_descriptor *pack = &xfr->iso_packet_desc[i];
        if (pack->status != LIBUSB_TRANSFER_COMPLETED) {
            LOGE("Error (status %d: %s)\n", pack->status,
                    libusb_error_name(pack->status));
            continue;
        }
        const uint8_t *data = libusb_get_iso_packet_buffer_simple(xfr, i);
        /* PACKET_SIZE == 192 == pack->length */
        memcpy(recv_next, data, PACKET_SIZE);
        recv_next += PACKET_SIZE;
        len += pack->length;
    }
    /* At this point, recv points to a buffer containing len bytes of audio. */
#if !defined(ANDROID)
    /* Lock around this write? */
    if ((rc = write(outfd, recv, len)) < 0) {
        perror("Unable to write to descriptor");
    }
#endif
    free(recv);
    /* Sanity check. If this is true, we've overflowed the recv buffer. */
    if (len > PACKET_SIZE * xfr->num_iso_packets) {
        LOGE("Error: incoming transfer had more data than we thought.\n");
        return;
    }
	if ((rc = libusb_submit_transfer(xfr)) < 0) {
		LOGE("libusb_submit_transfer: %s.\n", libusb_error_name(rc));
	}
}

/* Setup is done once, after permission has been obtained. */
int usb_setup(void) {
	int rc = -1;

	rc = libusb_init(NULL);
	if (rc < 0) {
		LOGD("libusb_init: %s\n", libusb_error_name(rc));
        return rc;
	}

	devh = libusb_open_device_with_vid_pid(NULL, VID, PID);
	if (!devh) {
		LOGD("libusb_open_device_with_vid_pid failed.\n");
        rc = -1;
        goto out;
	}

    rc = libusb_kernel_driver_active(devh, IFACE_NUM);
    if (rc == 1) {
        rc = libusb_detach_kernel_driver(devh, IFACE_NUM);
        if (rc < 0) {
            LOGD("libusb_detach_kernel_driver: %s.\n", libusb_error_name(rc));
            goto out;
        }
    }

	rc = libusb_claim_interface(devh, IFACE_NUM);
	if (rc < 0) {
		LOGD("libusb_claim_interface: %s.\n", libusb_error_name(rc));
        goto out;
    }

	rc = libusb_set_interface_alt_setting(devh, IFACE_NUM, 1);
	if (rc < 0) {
		LOGD("libusb_set_interface_alt_setting: %s.\n", libusb_error_name(rc));
        goto out;
	}

    LOGD("Opened USB device %04x:%04x IFACE %d.\n", VID, PID, IFACE_NUM);
    is_setup = true;
    return 0;

out:
    if (devh)
        libusb_close(devh);
    libusb_exit(NULL);
    return rc;
}


/* Once setup has succeded, this is called once to start transfers. */
int usb_start_transfers(int fd) {
    if (!is_setup) {
        LOGD("Must call setup before starting.\n");
        return -1;
    }
	static uint8_t buf[PACKET_SIZE * NUM_PACKETS];
	static struct libusb_transfer *xfr[NUM_TRANSFERS];
	int num_iso_pack = NUM_PACKETS;
    int i;

    outfd = fd;
    if (outfd < 3) {
        LOGD("Set outfd to %d. Are you sure?\n", outfd);
    }

    for (i=0; i<NUM_TRANSFERS; i++) {
        xfr[i] = libusb_alloc_transfer(num_iso_pack);
        if (!xfr[i]) {
            LOGD("libusb_alloc_transfer failed.\n");
            return -ENOMEM;
        }

        libusb_fill_iso_transfer(xfr[i], devh, EP_ISO_IN, buf,
                sizeof(buf), num_iso_pack, transfer_cb, NULL, 1000);
        libusb_set_iso_packet_lengths(xfr[i], sizeof(buf)/num_iso_pack);

        libusb_submit_transfer(xfr[i]);
    }
    return 0;
}


/* Called when USB is no longer required. */
void usb_exit(void) {
    if (!is_setup) {
        LOGD("Exit called when not setup.\n");
        return;
    }
    is_setup = false;
    if (devh)
        libusb_close(devh);
    libusb_exit(NULL);
}

/* Call this in a loop. */
void usb_process(void) {
    int rc = libusb_handle_events(NULL);
    if (rc != LIBUSB_SUCCESS) {
        LOGD("libusb_handle_events: %s.\n", libusb_error_name(rc));
        return;
    }
}


