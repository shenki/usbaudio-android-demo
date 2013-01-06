#ifndef FREEDV_USB_H
#define FREEDV_USB_H

/* Setup is done once, after permission has been obtained. */
int usb_setup(void);

/* Once setup has succeded, this is called once to start transfers. */
int usb_start_transfers(int fd);

#endif
