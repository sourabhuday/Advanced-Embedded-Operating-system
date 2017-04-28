#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include<kern/pmap.h>
#include<kern/pci.h>
#include<inc/string.h>

#define TXDESC_MAXSIZE 64
#define PKT_MAX_SIZE 4096
volatile uint32_t *mmio_e1000;
//struct e1000_tx_desc txdesc_array[TXDESC_MAXSIZE] __attribute__ ((aligned(16)));

int attach_fn(struct pci_func *pcif);
int e1000_transmit(void *pkt, uint32_t length);

#define E1000_STATUS   0x00008/4  /* Device Status - RO */
#define E1000_TDBAL    0x03800/4  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804 / 4  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808/4  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810/4  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818/4  /* TX Descripotr Tail - RW */
#define E1000_TCTL     0x00400/4  /* TX Control - RW */
#define E1000_TIPG     0x00410/4  /* TX Inter-packet gap -RW */

//#define E1000_TCTL_EN     0x00000002    /* enable tx */
//#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
//#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
//#define E1000_TCTL_CT     0x00000100    /* collision threshold */

//#define E1000_TIPG_IPGT	0
//#define E1000_TIPG_IPGR1	10
//#define E1000_TIPG_IPGR2	20

/* TX Inter-packet gap bit definitions */
#define E1000_TIPG_IPGT      0x000003FF
#define E1000_TIPG_IPGR1     0x000FFA00
#define E1000_TIPG_IPGR2     0x3FF00000

#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */

#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */
#define E1000_TXD_CMD_EOP    0x01000000 /* End of Packet */
#define E1000_TXD_CMD_RS     0x08000000 /* Report Status */

#define VALUEATMASK(value, mask) value * ((mask) & ~((mask) << 1))


/* Transmit Descriptor */
struct e1000_tx_desc {
    uint64_t buffer_addr;       /* Address of the descriptor's data buffer */
    union {
        uint32_t data;
        struct {
            uint16_t length;    /* Data buffer length */
            uint8_t cso;        /* Checksum offset */
            uint8_t cmd;        /* Descriptor control */
        } flags;
    } lower;
    union {
        uint32_t data;
        struct {
            uint8_t status;     /* Descriptor status */
            uint8_t css;        /* Checksum start */
            uint16_t special;
        } fields;
    } upper;
};

struct e1000_data
{
	char buf[PKT_MAX_SIZE];
};

struct e1000_tx_desc txdesc_array[TXDESC_MAXSIZE] __attribute__ ((aligned(16)));
struct e1000_data txdata_buf[TXDESC_MAXSIZE] __attribute__ ((aligned (PGSIZE)));
#endif	// JOS_KERN_E1000_H
