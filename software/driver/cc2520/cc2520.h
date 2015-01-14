#ifndef CC2520_H
#define CC2520_H

#include <linux/types.h>
#include <linux/semaphore.h>  /* Semaphore */
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>

#include "packet.h"


//////////////////////////////
// Configuration for driver
/////////////////////////////

// Number of cc2520 radio devices
// #define CC2520_NUM_DEVICES 2

// DEMUX values for the CS lines
// #define CC2520_CS_MUX_INDEX_0 0
// #define CC2520_CS_MUX_INDEX_1 1

// Amplified radio booleans
// #define CC2520_AMP0 0
// #define CC2520_AMP1 1

// Default first minor number
// #define CC2520_DEFAULT_MINOR 0

// Physical mapping of GPIO pins on the CC2520
// to GPIO pins on the linux microcontroller.

// Pins on cc2520_0:
// #define CC2520_0_GPIO_0 -1
// #define CC2520_0_GPIO_1 115 //FIFO0
// #define CC2520_0_GPIO_2 49  //FIFOP0
// #define CC2520_0_GPIO_3 48  //CCA0
// #define CC2520_0_GPIO_4 50  //SFD0
// #define CC2520_0_GPIO_5 -1

// // Pins on cc2520_1:
// #define CC2520_1_GPIO_0 -1
// #define CC2520_1_GPIO_1 46  //FIFO1
// #define CC2520_1_GPIO_2 23  //FIFOP1
// #define CC2520_1_GPIO_3 26  //CCA1
// #define CC2520_1_GPIO_4 47  //SFD1
// #define CC2520_1_GPIO_5 -1

// // Reset Pins
// #define CC2520_0_RESET  60
// #define CC2520_1_RESET  61

// Logical mapping of CC2520 GPIO pins to
// functions, we're going to keep these static
// for now, and map to the defaults, which
// mirror the CC2420 implementation.

// High when at least one byte is
// in the RX FIFO buffer
// #define CC2520_0_FIFO CC2520_0_GPIO_1
// #define CC2520_1_FIFO CC2520_1_GPIO_1

// High when the number of bytes exceeds
// a programmed theshold or a full packet
// has been received.
// #define CC2520_0_FIFOP CC2520_0_GPIO_2
// #define CC2520_1_FIFOP CC2520_1_GPIO_2

// Clear channel assessment
// #define CC2520_0_CCA CC2520_0_GPIO_3
// #define CC2520_1_CCA CC2520_1_GPIO_3

// Start frame delimiter
// #define CC2520_0_SFD CC2520_0_GPIO_4
// #define CC2520_1_SFD CC2520_1_GPIO_4

// For Beaglebone Black we're using the following
// SPI bus and CS pin.
// #define SPI_BUS 1
// #define SPI_BUS_CS0 0
// #define SPI_BUS_SPEED 500000
#define SPI_BUFF_SIZE 256
#define PKT_BUFF_SIZE 127

// Defaults for Radio Operation
#define CC2520_DEF_CHANNEL 26
#define CC2520_DEF_RFPOWER 0x32 // 0 dBm
#define CC2520_DEF_PAN 0x22
#define CC2520_DEF_SHORT_ADDR 0x01
#define CC2520_DEF_EXT_ADDR 0x01

// All these timing parameters are in microseconds.
#define CC2520_DEF_ACK_TIMEOUT 2500
#define CC2520_DEF_MIN_BACKOFF 320
#define CC2520_DEF_INIT_BACKOFF 4960
#define CC2520_DEF_CONG_BACKOFF 2240
#define CC2520_DEF_CSMA_ENABLED true

// We go for around a 1% duty cycle of the radio
// for LPL stuff.
#define CC2520_DEF_LPL_WAKEUP_INTERVAL 512000
#define CC2520_DEF_LPL_LISTEN_WINDOW 5120
#define CC2520_DEF_LPL_ENABLED true

// SACK Stuff
#define CC2520_DEF_SACK_ENABLED false

// Error codes
#define CC2520_TX_SUCCESS 0
#define CC2520_TX_BUSY 1
#define CC2520_TX_LENGTH 2
#define CC2520_TX_ACK_TIMEOUT 3
#define CC2520_TX_FAILED 4

// XOSC Period in nanoseconds.
#define CC2520_XOSC_PERIOD 31

#define CC2520_TX_UNDERFLOW (1<<3)
//////////////////////////////
// Structs and definitions
/////////////////////////////

struct timer_struct {
    struct hrtimer timer;
    struct cc2520_dev *dev;
};

struct wq_struct {
    struct work_struct work;
    struct cc2520_dev *dev;
};

struct cc2520_dev {
    // Device index
    int id;

    // Packet Length
    int len;

    // CS Demux Pin Settings
    unsigned int chipselect_demux_index;

    // Is there an amplifier attached?
    bool amplified;

    // Pin config
    int reset;
    int fifo;
    int fifop;
    int cca;
    int sfd;

    // IRQ
    unsigned int fifop_irq;
    unsigned int sfd_irq;

    // Character device struct
    struct cdev cdev;

    // RADIO.c state
    u16 short_addr;
    u64 extended_addr;
    u16 pan_id;
    u8  channel;

    struct spi_message  msg;
    struct spi_transfer tsfer0;
    struct spi_transfer tsfer1;
    struct spi_transfer tsfer2;
    struct spi_transfer tsfer3;
    struct spi_transfer tsfer4;

    struct spi_message  rx_msg;
    struct spi_transfer rx_tsfer;

    u8 tx_buf[SPI_BUFF_SIZE];
    u8 rx_buf[SPI_BUFF_SIZE];
    u8 rx_out_buf[SPI_BUFF_SIZE];
    u8 rx_in_buf[SPI_BUFF_SIZE];
    u8 tx_buf_r[PKT_BUFF_SIZE];
    u8 rx_buf_r[PKT_BUFF_SIZE];
    u8 tx_buf_r_len;

    u64 sfd_nanos_ts;

    spinlock_t radio_sl;
    spinlock_t radio_pending_rx_sl;
    spinlock_t radio_rx_buf_sl;
    bool       radio_pending_rx;

    int radio_state;

    unsigned long radio_flags;
    unsigned long radio_flags1;


    // SACK.c state
    u8 ack_buf[IEEE154_ACK_FRAME_LENGTH + 1];
    u8 cur_tx_buf[PKT_BUFF_SIZE];
    u8 cur_rx_buf[PKT_BUFF_SIZE];
    u8 cur_rx_buf_len;
    unsigned long sack_flags;
    bool sack_enabled;

    struct timer_struct timeout_timer;
    int ack_timeout; //in microseconds
    int sack_state;
    spinlock_t sack_sl;



    // CSMA.c state
    int backoff_min;
    int backoff_max_init;
    int backoff_max_cong;
    bool csma_enabled;

    struct timer_struct backoff_timer;

    u8 csma_cur_tx_buf[PKT_BUFF_SIZE];
    u8 csma_cur_tx_len;

    spinlock_t state_sl;

    struct workqueue_struct *wq;
    struct wq_struct work_s;

    int csma_state;

    unsigned long csma_flags;


    // LPL.c state
    int lpl_window;
    int lpl_interval;
    bool lpl_enabled;

    struct timer_struct lpl_timer;

    u8 lpl_cur_tx_buf[PKT_BUFF_SIZE];
    u8 lpl_cur_tx_len;

    spinlock_t lpl_state_sl;

    unsigned long lpl_flags;

    int lpl_state;


    // UNIQUE.c state
    struct list_head nodes;


    // INTERFACE.c state
// static unsigned int major;
// static unsigned int minor = CC2520_DEFAULT_MINOR;
// static unsigned int num_devices = CC2520_NUM_DEVICES;
// static struct class* cl;
// struct cc2520_dev* cc2520_devices;

    u8 tx_buf_c[PKT_BUFF_SIZE];
    u8 rx_buf_c[PKT_BUFF_SIZE];
    size_t tx_pkt_len;
    size_t rx_pkt_len;

    // Allows for only a single rx or tx
    // to occur simultaneously.
    struct semaphore tx_sem;
    struct semaphore rx_sem;

    // Used by the character driver
    // to indicate when a blocking tx
    // or rx has completed.
    struct semaphore tx_done_sem;
    struct semaphore rx_done_sem;

    // Results, stored by the callbacks
    int tx_result;

    wait_queue_head_t cc2520_interface_read_queue;




};

// struct cc2520_interface {
//     // ALWAYS the length of the packet,
//     // including the length byte itself,
//     // excluding the automatically generated
//     // FCS bytes.
//     // The packet should start with a valid
//     // 802.15.4 length.
//     int (*tx)(u8 *buf, u8 len, struct cc2520_dev *dev);
//     void (*tx_done)(u8 status, struct cc2520_dev *dev);

//     // ALWAYS the length of the packet,
//     // including the length byte itself,
//     // and including the automatically
//     // generated FCS bytes. The packet should
//     // start with a valid 802.15.4 length and
//     // end with valid FCS bytes.
//     void (*rx_done)(u8 *buf, u8 len, struct cc2520_dev *dev);
// };

///
// KEEP THESE AROUND FOR NOW, REFACTOR OUT LATER.
//

// struct cc2520_gpio_state {
// 	unsigned int fifop_irq;
// 	unsigned int sfd_irq;
// };

// DEPRECATED



// Global configuration data for the driver
struct cc2520_config {
    dev_t chr_dev;
    unsigned int major;
    struct class* cl;

    u8 num_radios; // Number of radios on board.

    struct cc2520_dev *radios;

	// Hardware
	// struct cc2520_gpio_state gpios;
	// struct spi_device *spi_device;

 //    // CURRENTLY UNUSED:
	// struct work_struct work;    /* for deferred work */
 //    struct workqueue_struct *wq;
};

extern struct cc2520_config config;
extern const char cc2520_name[];

//////////////////////////////
// Radio Opcodes and Register Defs
/////////////////////////////

// Note: Adopted from TinyOS CC2520 driver.

typedef struct cc2520_metadata_t
{
	u8 lqi;
	union
	{
		u8 power;
		u8 ack;
		u8 rssi;
	};
} cc2520_metadata_t;

enum cc2520_reg_access_enums {
    CC2520_FREG_MASK      = 0x3F,    // highest address in FREG
    CC2520_SREG_MASK      = 0x7F,    // highest address in SREG
    CC2520_CMD_TXRAM_WRITE	  = 0x80, // FIXME: not sure... might need to change
};

typedef union cc2520_status {
	u16 value;
	struct {
	  unsigned  rx_active    :1;
	  unsigned  tx_active    :1;
	  unsigned  dpu_l_active :1;
	  unsigned  dpu_h_active :1;

	  unsigned  exception_b  :1;
	  unsigned  exception_a  :1;
	  unsigned  rssi_valid   :1;
	  unsigned  xosc_stable  :1;
	};
} cc2520_status_t;

typedef union cc2520_frmctrl0 {
    u8 value;
    struct {
        unsigned tx_mode          : 2;
        unsigned rx_mode          : 2;
        unsigned energy_scan      : 1;
        unsigned autoack          : 1;
        unsigned autocrc          : 1;
        unsigned append_data_mode : 1;
    } f;
} cc2520_frmctrl0_t;

typedef union cc2520_txpower {
    u8 value;
    struct {
        unsigned pa_power: 8;
    } f;
} cc2520_txpower_t;

typedef union cc2520_ccactrl0 {
    u8 value;
    struct {
        unsigned cca_thr: 8;
    } f;
} cc2520_ccactrl0_t;

typedef union cc2520_mdmctrl0 {
    u8 value;
    struct {
        unsigned tx_filter       : 1;
        unsigned preamble_length : 4;
        unsigned demod_avg_mode  : 1;
        unsigned dem_num_zeros   : 2;
    } f;
} cc2520_mdmctrl0_t;

typedef union cc2520_mdmctrl1 {
    u8 value;
    struct {
        unsigned corr_thr     : 5;
        unsigned corr_thr_sfd : 1;
        unsigned reserved0    : 2;
    } f;
} cc2520_mdmctrl1_t;

typedef union cc2520_freqctrl {
    u8 value;
    struct {
        unsigned freq       : 7;
        unsigned reserved0  : 1;
    } f;
} cc2520_freqctrl_t;

typedef union cc2520_fifopctrl {
    u8 value;
    struct {
        unsigned fifop_thr : 7;
        unsigned reserved0 : 1;
    } f;
} cc2520_fifopctrl_t;

typedef union cc2520_frmfilt0 {
    u8 value;
    struct {
        unsigned frame_filter_en         : 1;
        unsigned pan_coordinator         : 1;
        unsigned max_frame_version       : 2;
        unsigned fcf_reserved_mask       : 3;
        unsigned reserved                : 1;
    } f;
} cc2520_frmfilt0_t;


typedef union cc2520_frmfilt1 {
    u8 value;
    struct {
        unsigned reserved0               : 1;
        unsigned modify_ft_filter        : 2;
        unsigned accept_ft_0_beacon      : 1;
        unsigned accept_ft_1_data        : 1;
        unsigned accept_ft_2_ack         : 1;
        unsigned accept_ft_3_mac_cmd     : 1;
        unsigned accept_ft_4to7_reserved : 1;
    } f;
} cc2520_frmfilt1_t;

typedef union cc2520_srcmatch {
    u8 value;
    struct {
        unsigned src_match_en      : 1;
        unsigned autopend          : 1;
        unsigned pend_datareq_only : 1;
        unsigned reserved          : 5;
    } f;
} cc2520_srcmatch_t;

typedef union cc2520_rxctrl {
    u8 value;
} cc2520_rxctrl_t;

typedef union cc2520_fsctrl {
    u8 value;
} cc2520_fsctrl_t;

typedef union cc2520_fscal1 {
    u8 value;
} cc2520_fscal1_t;

typedef union cc2520_agcctrl1 {
    u8 value;
} cc2520_agcctrl1_t;

typedef union cc2520_adctest0 {
    u8 value;
} cc2520_adctest0_t;

typedef union cc2520_adctest1 {
    u8 value;
} cc2520_adctest1_t;

typedef union cc2520_adctest2 {
    u8 value;
} cc2520_adctest2_t;

typedef union cc2520_gpioctrl0 {
    u8 value;
} cc2520_gpioctrl0_t;

typedef union cc2520_gpioctrl5 {
    u8 value;
} cc2520_gpioctrl5_t;

typedef union cc2520_gpiopolarity{
    u8 value;
} cc2520_gpiopolarity_t;

typedef union cc2520_txctrl{
    u8 value;
} cc2520_txctrl_t;



enum {
    CC2520_TX_PWR_MASK  = 0xFF,
    CC2520_TX_PWR_0     = 0x03, // -18 dBm
    CC2520_TX_PWR_1     = 0x2C, //  -7 dBm
    CC2520_TX_PWR_2     = 0x88, //  -4 dBm
    CC2520_TX_PWR_3     = 0x81, //  -2 dBm
    CC2520_TX_PWR_4     = 0x32, //   0 dBm
    CC2520_TX_PWR_5     = 0x13, //   1 dBm
    CC2520_TX_PWR_6     = 0xAB, //   2 dBm
    CC2520_TX_PWR_7     = 0xF2, //   3 dBm
    CC2520_TX_PWR_8     = 0xF7, //   5 dBm
    CC2520_CHANNEL_MASK = 0x1F,
};

enum cc2520_register_enums
{
    // FREG Registers
    CC2520_FRMFILT0     = 0x00,
    CC2520_FRMFILT1     = 0x01,
    CC2520_SRCMATCH     = 0x02,
    CC2520_SRCSHORTEN0  = 0x04,
    CC2520_SRCSHORTEN1  = 0x05,
    CC2520_SRCSHORTEN2  = 0x06,
    CC2520_SRCEXTEN0    = 0x08,
    CC2520_SRCEXTEN1    = 0x09,
    CC2520_SRCEXTEN2    = 0x0A,
    CC2520_FRMCTRL0     = 0x0C,
    CC2520_FRMCTRL1     = 0x0D,
    CC2520_RXENABLE0    = 0x0E,
    CC2520_RXENABLE1    = 0x0F,
    CC2520_EXCFLAG0     = 0x10,
    CC2520_EXCFLAG1     = 0x11,
    CC2520_EXCFLAG2     = 0x12,
    CC2520_EXCMASKA0    = 0x14,
    CC2520_EXCMASKA1    = 0x15,
    CC2520_EXCMASKA2    = 0x16,
    CC2520_EXCMASKB0    = 0x18,
    CC2520_EXCMASKB1    = 0x19,
    CC2520_EXCMASKB2    = 0x1A,
    CC2520_EXCBINDX0    = 0x1C,
    CC2520_EXCBINDX1    = 0x1D,
    CC2520_EXCBINDY0    = 0x1E,
    CC2520_EXCBINDY1    = 0x1F,
    CC2520_GPIOCTRL0    = 0x20,
    CC2520_GPIOCTRL1    = 0x21,
    CC2520_GPIOCTRL2    = 0x22,
    CC2520_GPIOCTRL3    = 0x23,
    CC2520_GPIOCTRL4    = 0x24,
    CC2520_GPIOCTRL5    = 0x25,
    CC2520_GPIOPOLARITY = 0x26,
    CC2520_GPIOCTRL     = 0x28,
    CC2520_DPUCON       = 0x2A,
    CC2520_DPUSTAT      = 0x2C,
    CC2520_FREQCTRL     = 0x2E,
    CC2520_FREQTUNE     = 0x2F,
    CC2520_TXPOWER      = 0x30,
    CC2520_TXCTRL       = 0x31,
    CC2520_FSMSTAT0     = 0x32,
    CC2520_FSMSTAT1     = 0x33,
    CC2520_FIFOPCTRL    = 0x34,
    CC2520_FSMCTRL      = 0x35,
    CC2520_CCACTRL0     = 0x36,
    CC2520_CCACTRL1     = 0x37,
    CC2520_RSSI         = 0x38,
    CC2520_RSSISTAT     = 0x39,
    CC2520_RXFIRST      = 0x3C,
    CC2520_RXFIFOCNT    = 0x3E,
    CC2520_TXFIFOCNT    = 0x3F,

    // SREG registers
    CC2520_CHIPID       = 0x40,
    CC2520_VERSION      = 0x42,
    CC2520_EXTCLOCK     = 0x44,
    CC2520_MDMCTRL0     = 0x46,
    CC2520_MDMCTRL1     = 0x47,
    CC2520_FREQEST      = 0x48,
    CC2520_RXCTRL       = 0x4A,
    CC2520_FSCTRL       = 0x4C,
    CC2520_FSCAL0       = 0x4E,
    CC2520_FSCAL1       = 0x4F,
    CC2520_FSCAL2       = 0x50,
    CC2520_FSCAL3       = 0x51,
    CC2520_AGCCTRL0     = 0x52,
    CC2520_AGCCTRL1     = 0x53,
    CC2520_AGCCTRL2     = 0x54,
    CC2520_AGCCTRL3     = 0x55,
    CC2520_ADCTEST0     = 0x56,
    CC2520_ADCTEST1     = 0x57,
    CC2520_ADCTEST2     = 0x58,
    CC2520_MDMTEST0     = 0x5A,
    CC2520_MDMTEST1     = 0x5B,
    CC2520_DACTEST0     = 0x5C,
    CC2520_DACTEST1     = 0x5D,
    CC2520_ATEST        = 0x5E,
    CC2520_DACTEST2     = 0x5F,
    CC2520_PTEST0       = 0x60,
    CC2520_PTEST1       = 0x61,
    CC2520_RESERVED     = 0x62,
    CC2520_DPUBIST      = 0x7A,
    CC2520_ACTBIST      = 0x7C,
    CC2520_RAMBIST      = 0x7E,
};

enum cc2520_mem_enums
{
	CC2520_MEM_ADDR_BASE = 0x3EA
};

enum cc2520_spi_command_enums
{
    CC2520_CMD_SNOP           = 0x00, //
    CC2520_CMD_IBUFLD         = 0x02, //
    CC2520_CMD_SIBUFEX        = 0x03, //
    CC2520_CMD_SSAMPLECCA     = 0x04, //
    CC2520_CMD_SRES           = 0x0f, //
    CC2520_CMD_MEMORY_MASK    = 0x0f, //
    CC2520_CMD_MEMORY_READ    = 0x10, // MEMRD
    CC2520_CMD_MEMORY_WRITE   = 0x20, // MEMWR
    CC2520_CMD_RXBUF          = 0x30, //
    CC2520_CMD_RXBUFCP        = 0x38, //
    CC2520_CMD_RXBUFMOV       = 0x32, //
    CC2520_CMD_TXBUF          = 0x3A, //
    CC2520_CMD_TXBUFCP        = 0x3E, //
    CC2520_CMD_RANDOM         = 0x3C, //
    CC2520_CMD_SXOSCON        = 0x40, //
    CC2520_CMD_STXCAL         = 0x41, //
    CC2520_CMD_SRXON          = 0x42, //
    CC2520_CMD_STXON          = 0x43, //
    CC2520_CMD_STXONCCA       = 0x44, //
    CC2520_CMD_SRFOFF         = 0x45, //
    CC2520_CMD_SXOSCOFF        = 0x46, //
    CC2520_CMD_SFLUSHRX       = 0x47, //
    CC2520_CMD_SFLUSHTX       = 0x48, //
    CC2520_CMD_SACK           = 0x49, //
    CC2520_CMD_SACKPEND       = 0x4A, //
    CC2520_CMD_SNACK          = 0x4B, //
    CC2520_CMD_SRXMASKBITSET  = 0x4C, //
    CC2520_CMD_SRXMASKBITCLR  = 0x4D, //
    CC2520_CMD_RXMASKAND      = 0x4E, //
    CC2520_CMD_RXMASKOR       = 0x4F, //
    CC2520_CMD_MEMCP          = 0x50, //
    CC2520_CMD_MEMCPR         = 0x52, //
    CC2520_CMD_MEMXCP         = 0x54, //
    CC2520_CMD_MEMXWR         = 0x56, //
    CC2520_CMD_BCLR           = 0x58, //
    CC2520_CMD_BSET           = 0x59, //
    CC2520_CMD_CTR_UCTR       = 0x60, //
    CC2520_CMD_CBCMAC         = 0x64, //
    CC2520_CMD_UCBCMAC        = 0x66, //
    CC2520_CMD_CCM            = 0x68, //
    CC2520_CMD_UCCM           = 0x6A, //
    CC2520_CMD_ECB            = 0x70, //
    CC2520_CMD_ECBO           = 0x72, //
    CC2520_CMD_ECBX           = 0x74, //
    CC2520_CMD_INC            = 0x78, //
    CC2520_CMD_ABORT          = 0x7F, //
    CC2520_CMD_REGISTER_READ  = 0x80, //
    CC2520_CMD_REGISTER_WRITE = 0xC0, //
};

#endif