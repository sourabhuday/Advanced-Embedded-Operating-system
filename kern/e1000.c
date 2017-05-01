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

 mmio_e1000[E1000_TCTL] = VALUEATMASK(1, E1000_TCTL_EN) | VALUEATMASK(1, E1000_TCTL_PSP) | VALUEATMASK(0x10, E1000_TCTL_CT) | VALUEATMASK(0x40, E1000_TCTL_COLD);
        
 mmio_e1000[E1000_TIPG] = VALUEATMASK(10, E1000_TIPG_IPGT) | VALUEATMASK(8, E1000_TIPG_IPGR1) | VALUEATMASK(6, E1000_TIPG_IPGR2);

 /*Reciever initiaization*/
 mmio_e1000[E1000_EERD] = 0x0 << E1000_EEPROM_RW_ADDR_SHIFT;
 mmio_e1000[E1000_EERD] |= E1000_EEPROM_RW_REG_START;
 while (!(mmio_e1000[E1000_EERD] & E1000_EEPROM_RW_REG_DONE));
 mmio_e1000[E1000_RAL] = mmio_e1000[E1000_EERD] >> 16;
 mmio_e1000[E1000_EERD]=0x0;

 mmio_e1000[E1000_EERD] = 0x1 << E1000_EEPROM_RW_ADDR_SHIFT;
 mmio_e1000[E1000_EERD] |= E1000_EEPROM_RW_REG_START;
 while (!(mmio_e1000[E1000_EERD] & E1000_EEPROM_RW_REG_DONE));
 mmio_e1000[E1000_RAL] |= mmio_e1000[E1000_EERD] & 0xffff0000;
 mmio_e1000[E1000_EERD]=0x0;

 mmio_e1000[E1000_EERD] = 0x2 << E1000_EEPROM_RW_ADDR_SHIFT;
 mmio_e1000[E1000_EERD] |= E1000_EEPROM_RW_REG_START;
 while (!(mmio_e1000[E1000_EERD] & E1000_EEPROM_RW_REG_DONE));
 mmio_e1000[E1000_RAH] = mmio_e1000[E1000_EERD] >> 16;
 mmio_e1000[E1000_EERD]=0x0;

 // mmio_e1000[E1000_RAL]  = 0x12005452;
 // mmio_e1000[E1000_RAH] = 0x80005634; 

 cprintf("Printing MAC address, low byte: %d and higher byte %d\n", mmio_e1000[E1000_RAL], mmio_e1000[E1000_RAH]) ;
 mmio_e1000[E1000_RAH] |= E1000_RAH_AV;

 mmio_e1000[E1000_RDBAL] = PADDR(rxdesc_array);
        mmio_e1000[E1000_RDBAH] = 0x0;
        mmio_e1000[E1000_RDLEN] = RXDESC_MAXSIZE * sizeof(struct e1000_rx_desc);
        mmio_e1000[E1000_RDH] = 0x0;
        mmio_e1000[E1000_RDT] = RXDESC_MAXSIZE;
 
 mmio_e1000[E1000_RCTL] = E1000_RCTL_EN |
                                     !E1000_RCTL_LPE |
                                     E1000_RCTL_LBM_NO |
                                     E1000_RCTL_RDMTS_HALF |
                                     E1000_RCTL_MO_0 |
                                     E1000_RCTL_BAM |
                                     E1000_RCTL_BSEX |
                                     E1000_RCTL_SZ_4096 |
                                     E1000_RCTL_SECRC;


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


int e1000_recv(uint8_t * data)
{
        static uint32_t real_tail = 0;
        uint32_t tail = real_tail;
        struct e1000_rx_desc * tail_desc = &rxdesc_array[tail];
        if (!(tail_desc->status & E1000_RXD_STAT_DD))
        {
                return -1;
        }
        size_t length = tail_desc->length;
        memmove(data, &rxdata_buf[tail], length);
        tail_desc->status = 0;
        mmio_e1000[E1000_RDT] = tail;
        real_tail = (tail + 1) % RXDESC_MAXSIZE;
        return length;
}


static void init_desc()
{
	int i;
	for (i = 0; i < TXDESC_MAXSIZE; ++i)
	{
		txdesc_array[i].buffer_addr = PADDR(&txdata_buf[i]);
		txdesc_array[i].upper.fields.status = E1000_TXD_STAT_DD;
                //txdesc_array[i].lower.data |=(E1000_TXD_CMD_RS);
	}   
                for (i = 0; i < RXDESC_MAXSIZE; ++i)
        {
                rxdesc_array[i].buffer_addr = PADDR(&rxdata_buf[i]);


        }

}

void read_mac_address(uint8_t* mac_address)
{
        *(uint32_t*)mac_address = (uint32_t)mmio_e1000[E1000_RAL];
        *(uint16_t*)(mac_address + 4) = (uint16_t)mmio_e1000[E1000_RAH];
}

