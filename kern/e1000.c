#include <kern/e1000.h>
//#include<string.h>

// LAB 6: Your driver code here
static void e1000_init(void);
static void init_desc(void);
int  e1000_transmit(void *pkt, uint32_t length);

int attach_fn(struct pci_func * pcif)
{
pci_func_enable(pcif);
init_desc();
mmio_e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

cprintf("Printing the status register = %x\n", mmio_e1000[E1000_STATUS]);
assert(mmio_e1000[E1000_STATUS]==0x80080783);

e1000_init();

//char arr[10]="Test";
//e1000_transmit(arr,4);
return 0;
}

static void e1000_init()
{
mmio_e1000[E1000_TDBAL] = PADDR(txdesc_array);
mmio_e1000[E1000_TDBAH] = 0x0;
mmio_e1000[E1000_TDLEN] =  TXDESC_MAXSIZE * sizeof(struct e1000_tx_desc);
mmio_e1000[E1000_TDH] = 0x0;
mmio_e1000[E1000_TDT] = 0x0;

assert(mmio_e1000[E1000_TDH] == 0);
assert(mmio_e1000[E1000_TDT] == 0);

mmio_e1000[E1000_TCTL] = VALUEATMASK(1, E1000_TCTL_EN) |
                                                VALUEATMASK(1, E1000_TCTL_PSP) |
                                                VALUEATMASK(0x10, E1000_TCTL_CT) |
                                                VALUEATMASK(0x40, E1000_TCTL_COLD);
        mmio_e1000[E1000_TIPG] = VALUEATMASK(10, E1000_TIPG_IPGT) |
                                                VALUEATMASK(8, E1000_TIPG_IPGR1) |
                                                VALUEATMASK(6, E1000_TIPG_IPGR2);

//mmio_e1000[E1000_TCTL] |= E1000_TCTL_EN;
//mmio_e1000[E1000_TCTL] |= E1000_TCTL_PSP;
//mmio_e1000[E1000_TCTL] |= E1000_TCTL_CT;

//mmio_e1000[E1000_TCTL] |= (E1000_TCTL_COLD & 0x00040000);

//mmio_e1000[E1000_TIPG] |= (0xa << E1000_TIPG_IPGT);
//mmio_e1000[E1000_TIPG] |= (0x8 << E1000_TIPG_IPGR1);
//mmio_e1000[E1000_TIPG] |= (0xc << E1000_TIPG_IPGR2);
//mmio_e1000[E1000_TIPG] = 0x00105010;
cprintf("Printing TIPG : %x\n and TCTL : %x\n",mmio_e1000[E1000_TIPG], mmio_e1000[E1000_TCTL]);

}

int e1000_transmit(void *pkt, uint32_t length )
{
 length = length > PKT_MAX_SIZE ? PKT_MAX_SIZE : length;

 uint32_t tail_idx = mmio_e1000[E1000_TDT];

 struct e1000_tx_desc * tail_desc = &txdesc_array[tail_idx];


 if(tail_desc->upper.fields.status != E1000_TXD_STAT_DD)
 {
 return -1;
 }

 memmove((void *) &txdata_buf[tail_idx], (void *) pkt, length);

 tail_desc->lower.flags.length = length;
 tail_desc->upper.fields.status = 0;
 tail_desc->lower.data |=  (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP); 

 mmio_e1000[E1000_TDT] = (tail_idx + 1) % TXDESC_MAXSIZE;
 return 0;

}
static void init_desc()
{
	int i;
	for (i = 0; i < TXDESC_MAXSIZE; i++)
	{
		txdesc_array[i].buffer_addr = PADDR(&txdata_buf[i]);
		txdesc_array[i].upper.fields.status = E1000_TXD_STAT_DD;
                //txdesc_array[i].lower.data |=(E1000_TXD_CMD_RS);
	}
}
