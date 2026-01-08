#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

// --------------------------------------------------------------------
// BOARD CONFIGURATION
// --------------------------------------------------------------------

#define CFG_TUSB_MCU OPT_MCU_RP2040
//#define CFG_TUSB_OS  OPT_OS_NONE
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define BOARD_DEVICE_RHPORT_NUM     0
#define BOARD_DEVICE_RHPORT_SPEED   OPT_MODE_FULL_SPEED

// --------------------------------------------------------------------
// COMMON SETTINGS
// --------------------------------------------------------------------

// Enable device stack
#define CFG_TUD_ENABLED 1

// Size of buffer for receiving data
#define CFG_TUD_ENDPOINT0_SIZE 64

// --------------------------------------------------------------------
// USB CLASSES
// --------------------------------------------------------------------

// HID Not used
#define CFG_TUD_HID 0 //Human Interface Device

// CDC (serial via usb) //Communications Device Class
#define CFG_TUD_CDC 0

// MSC //Mass Storage Class
#define CFG_TUD_MSC 0

// Vendor
#define CFG_TUD_VENDOR 1

#define CFG_TUD_VENDOR_RX_BUFSIZE 64
#define CFG_TUD_VENDOR_TX_BUFSIZE 64
#define CFG_TUD_CDC_RX_BUFSIZE 64
#define CFG_TUD_CDC_TX_BUFSIZE 64

#endif
