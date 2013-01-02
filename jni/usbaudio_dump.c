/*
 *
 * Dumb userspace USB Audio receiver
 * Copyright 2012 Joel Stanley <joel@jms.id.au>
 *
 * Based on the following:
 *
 * libusb example program to measure Atmel SAM3U isochronous performance
 * Copyright (C) 2012 Harald Welte <laforge@gnumonks.org>
 *
 * Copied with the author's permission under LGPL-2.1 from
 * http://git.gnumonks.org/cgi-bin/gitweb.cgi?p=sam3u-tests.git;a=blob;f=usb-benchmark-project/host/benchmark.c;h=74959f7ee88f1597286cd435f312a8ff52c56b7e
 *
 * An Atmel SAM3U test firmware is also available in the above repository.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libusb.h>

#include <jni.h>

#include <android/log.h>
#define LOGD(...) \
    __android_log_print(ANDROID_LOG_DEBUG, "UsbAudioNative", __VA_ARGS__)

#define UNUSED __attribute__((unused))

/* The first PCM stereo AudioStreaming endpoint. */
#define EP_ISO_IN	0x84
#define IFACE_NUM   2

static int logfd;

static int do_exit = 1;
static struct libusb_device_handle *devh = NULL;

static unsigned long num_bytes = 0, num_xfer = 0;
static struct timeval tv_start;

static void cb_xfr(struct libusb_transfer *xfr)
{
	unsigned int i;

    int len = 0;

    for (i = 0; i < xfr->num_iso_packets; i++) {
        struct libusb_iso_packet_descriptor *pack = &xfr->iso_packet_desc[i];

        if (pack->status != LIBUSB_TRANSFER_COMPLETED) {
            LOGD("Error (status %d: %s) :", pack->status,
                    libusb_error_name(pack->status));
            /* This doesn't happen, so bail out if it does. */
            exit(EXIT_FAILURE);
        }

        const uint8_t *data = libusb_get_iso_packet_buffer_simple(xfr, i);
        write(logfd, data, pack->length);

        len += pack->length;
    }

	num_bytes += len;
	num_xfer++;

	if (libusb_submit_transfer(xfr) < 0) {
		LOGD("error re-submitting URB\n");
		exit(1);
	}
}

#define NUM_TRANSFERS 10
#define PACKET_SIZE 192
#define NUM_PACKETS 10

static int benchmark_in(uint8_t ep)
{
	static uint8_t buf[PACKET_SIZE * NUM_PACKETS];
	static struct libusb_transfer *xfr[NUM_TRANSFERS];
	int num_iso_pack = NUM_PACKETS;
    int i;

	/* NOTE: To reach maximum possible performance the program must
	 * submit *multiple* transfers here, not just one.
	 *
	 * When only one transfer is submitted there is a gap in the bus
	 * schedule from when the transfer completes until a new transfer
	 * is submitted by the callback. This causes some jitter for
	 * isochronous transfers and loss of throughput for bulk transfers.
	 *
	 * This is avoided by queueing multiple transfers in advance, so
	 * that the host controller is always kept busy, and will schedule
	 * more transfers on the bus while the callback is running for
	 * transfers which have completed on the bus.
	 */
    for (i=0; i<NUM_TRANSFERS; i++) {
        xfr[i] = libusb_alloc_transfer(num_iso_pack);
        if (!xfr[i])
            return -ENOMEM;

        libusb_fill_iso_transfer(xfr[i], devh, ep, buf,
                sizeof(buf), num_iso_pack, cb_xfr, NULL, 1000);
        libusb_set_iso_packet_lengths(xfr[i], sizeof(buf)/num_iso_pack);

        libusb_submit_transfer(xfr[i]);
    }

    logfd = open("/storage/sdcard0/test-file.raw", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);

	gettimeofday(&tv_start, NULL);

    return 1;
}

unsigned int measure(void)
{
	struct timeval tv_stop;
	unsigned int diff_msec;

	gettimeofday(&tv_stop, NULL);

	diff_msec = (tv_stop.tv_sec - tv_start.tv_sec)*1000;
	diff_msec += (tv_stop.tv_usec - tv_start.tv_usec)/1000;

	printf("%lu transfers (total %lu bytes) in %u miliseconds => %lu bytes/sec\n",
		num_xfer, num_bytes, diff_msec, (num_bytes*1000)/diff_msec);

    return num_bytes;
}

JNIEXPORT jint JNICALL
Java_au_id_jms_usbaudio_UsbAudio_measure(JNIEnv* env UNUSED, jobject foo UNUSED) {
    return measure();
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm UNUSED, void* reserved UNUSED)
{
    LOGD("libusbaudio: loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT jboolean JNICALL
Java_au_id_jms_usbaudio_UsbAudio_setup(JNIEnv* env UNUSED, jobject foo UNUSED)
{
	int rc;

	rc = libusb_init(NULL);
	if (rc < 0) {
		LOGD("Error initializing libusb: %s\n", libusb_error_name(rc));
        return false;
	}

    /* This device is the TI PCM2900C Audio CODEC default VID/PID. */
	devh = libusb_open_device_with_vid_pid(NULL, 0x08bb, 0x29c0);
	if (!devh) {
		LOGD("Error finding USB device\n");
        libusb_exit(NULL);
        return false;
	}

    rc = libusb_kernel_driver_active(devh, IFACE_NUM);
    if (rc == 1) {
        rc = libusb_detach_kernel_driver(devh, IFACE_NUM);
        if (rc < 0) {
            LOGD("Could not detach kernel driver: %s\n",
                    libusb_error_name(rc));
            libusb_close(devh);
            libusb_exit(NULL);
            return false;
        }
    }

	rc = libusb_claim_interface(devh, IFACE_NUM);
	if (rc < 0) {
		LOGD("Error claiming interface: %s\n", libusb_error_name(rc));
        libusb_close(devh);
        libusb_exit(NULL);
        return false;
    }

	rc = libusb_set_interface_alt_setting(devh, IFACE_NUM, 1);
	if (rc < 0) {
		LOGD("Error setting alt setting: %s\n", libusb_error_name(rc));
        libusb_close(devh);
        libusb_exit(NULL);
        return false;
	}

    // Good to go
    do_exit = 0;
	benchmark_in(EP_ISO_IN);
}


JNIEXPORT void JNICALL
Java_au_id_jms_usbaudio_UsbAudio_stop(JNIEnv* env UNUSED, jobject foo UNUSED) {
    do_exit = 1;
    measure();
}

JNIEXPORT bool JNICALL
Java_au_id_jms_usbaudio_UsbAudio_close(JNIEnv* env UNUSED, jobject foo UNUSED) {
    if (do_exit == 0) {
        return false;
    }
	libusb_release_interface(devh, IFACE_NUM);
	if (devh)
		libusb_close(devh);
	libusb_exit(NULL);
    return true;
}


JNIEXPORT void JNICALL
Java_au_id_jms_usbaudio_UsbAudio_loop(JNIEnv* env UNUSED, jobject foo UNUSED) {
	while (!do_exit) {
		int rc = libusb_handle_events(NULL);
		if (rc != LIBUSB_SUCCESS)
			break;
	}
}

