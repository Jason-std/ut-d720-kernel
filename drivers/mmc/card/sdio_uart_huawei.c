/*BEGIN: DTS2014032201528 n00203913 2014-03-22 ADDED for SD Modem*/
/*
 * linux/drivers/mmc/card/sdio_uart.c - SDIO UART/GPS driver
 *
 * Based on drivers/serial/8250.c and drivers/serial/serial_core.c
 * by Russell King.
 *
 * Author:    Nicolas Pitre
 * Created:    June 15, 2007
 * Copyright:    MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

/*
 * Note: Although this driver assumes a 16550A-like UART implementation,
 * it is not possible to leverage the common 8250/16550 driver, nor the
 * core UART infrastructure, as they assumes direct access to the hardware
 * registers, often under a spinlock.  This is not possible in the SDIO
 * context as SDIO access functions must be able to sleep.
 *
 * Because we need to lock the SDIO host to ensure an exclusive access to
 * the card, we simply rely on that lock to also prevent and serialize
 * concurrent access to the same port.
 */

    
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/serial_reg.h>
#include <linux/circ_buf.h>
#include <linux/gfp.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/slab.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/scatterlist.h>

/*BEGIN:DTS2014042209407 n00203193 added for SD Modem 2014.04.15*/  
#include <linux/time.h>
/*END:DTS2014042209407 n00203193 added for SD Modem 2014.04.15*/  



//add yjj 20110324 begin 
/* debug utility */
#if 1
#define FUNC_ENTER printk("%s: >>>\n", __FUNCTION__);
#define FUNC_EXIT printk("%s: <<<\n", __FUNCTION__);
#define FUNC_MSG(x) printk("%s: %s\n", __FUNCTION__,x);
#define HW_DEBUG printk
#define HW_DBG(f, x...) printk("%s (%d): " f, __func__, __LINE__, ## x)
#else
#define FUNC_ENTER  //printk("%s: >>>\n", __FUNCTION__);
#define FUNC_EXIT  //printk("%s: <<<\n", __FUNCTION__);
#define FUNC_MSG(x) //printk("%s: %s\n", __FUNCTION__,x);
#define HW_DEBUG //printk
#define HW_DBG(f, x...) //printk("%s (%d): " f, __func__, __LINE__, ## x)
#endif
#define RX_FIFO_LENGTH_LSB                     0x08  /* READ FIFO LSB length address */
#define RX_FIFO_LENGTH_MSB                    0x09  /* READ FIFO MSB length address*/
#define TX_FIFO_LENGTH_LSB                     0x0A  /* WRITE FIFO LSB length address */
#define TX_FIFO_LENGTH_MSB                    0x0B  /* WRITE FIFO MSB length address */
#define SDIO_UART_MAX_READ_SIZE         2048  /* Max data read size */
#define SDIO_UART_MAX_WRITE_SIZE       2048  /* Maximuxm write fifo buffer size */
/*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/
/*BEGIN: DTS2014051203188 n00203913 2014-05-12 modified for SD Modem*/
#define MODEM_GATHER_MAX                  32  //16 //2//4 //8       /*Modem port data package gather max number*/
/*END: DTS2014051203188 n00203913 2014-05-12 modified for SD Modem*/
/*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/

/*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/  
unsigned int g_modem_pkg_gather = 0;
/*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/
unsigned char modem_uart_data_buf[200*1024];
/*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/
unsigned char sdio_send_buff[200*1024] = {0};
/*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/
unsigned char uart_data_buf[2*1024];
//unsigned char diag_uart_data_buf[SDIO_UART_MAX_WRITE_SIZE];
unsigned char g_read_buffer[256*1024];

//extern int sdio_read_common_cis(struct mmc_card *card);
/*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/
//struct timeval time1;
//struct timeval time2;
//struct timeval time3;
//struct timeval time4;
/*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/

struct sg_table g_sg_table;
#define MODEM_SIG_READ  0
#define MODEM_SIG_WRITE 1
unsigned char bExit = 0;
unsigned char bInit = 0;
unsigned char data_trans_thread_exit = 0;
unsigned char use_tans_thread = 0;

#define PORT_COUNT         3
unsigned int modem_packet_count;

typedef struct _uart_buf_info
{
    unsigned int size;
    unsigned int wt_pos;
    unsigned int rd_pos;
    unsigned int rtn_pos;
    unsigned char* buf;
}UART_BUF_INFO, *PUART_BUF_INFO;

typedef struct _mux_head
{
    unsigned char head_tag;
    unsigned char port_index;
    unsigned char ctrl_tag;
    unsigned short data_len;
}MUX_HEAD,*PMUX_HEAD;

UART_BUF_INFO tty_data_buf;
static wait_queue_head_t  thread_wait_q;
static wait_queue_head_t  remove_wait_q;
MUX_HEAD mux_head[PORT_COUNT];
unsigned char modem_opened = 0;

typedef struct tagMODEM_MSC_STRU
{
    unsigned int OP_Dtr     :    1;          /*DTR CHANGE FLAG*/
    unsigned int OP_Dsr     :    1;          /*DSR CHANGE FLAG*/
    unsigned int OP_Cts     :    1;          /*CTSCHANGE FLAG*/
    unsigned int OP_Rts     :    1;          /*RTS CHANGE FLAG*/
    unsigned int OP_Ri      :    1;          /*RI CHANGE FLAG*/
    unsigned int OP_Dcd     :    1;          /*DCD CHANGE FLAG*/
    unsigned int OP_Fc      :    1;          /*FC CHANGE FLAG*/
    unsigned int OP_Brk     :    1;          /*BRK CHANGE FLAG*/
    unsigned int OP_Spare   :    24;         /*reserve*/
    unsigned char   ucDtr;                     /*DTR  VALUE*/
    unsigned char   ucDsr;                     /*DSR  VALUE*/
    unsigned char   ucCts;                     /*DTS VALUE*/
    unsigned char   ucRts;                     /*RTS  VALUE*/
    unsigned char   ucRi;                      /*RI VALUE*/
    unsigned char   ucDcd;                     /*DCD  VALUE*/
    unsigned char   ucFc;                      /*FC  VALUE*/
    unsigned char   ucBrk;                     /*BRK  VALUE*/
    unsigned char   ucBrkLen;                  /*BRKLEN VALUE*/
} MODEM_MSC_STRU, *PMODEM_MSC_STRU,AT_DCE_MSC_STRU;


#define UART_NR        8    /* Number of UARTs this driver can handle */


#define UART_XMIT_SIZE    PAGE_SIZE
#define WAKEUP_CHARS    256

#define circ_empty(circ)    ((circ)->head == (circ)->tail)
#define circ_clear(circ)    ((circ)->head = (circ)->tail = 0)

#define circ_chars_pending(circ) \
        (CIRC_CNT((circ)->head, (circ)->tail, UART_XMIT_SIZE))

#define circ_chars_free(circ) \
        (CIRC_SPACE((circ)->head, (circ)->tail, UART_XMIT_SIZE))

struct uart_icount {
    __u32    cts;
    __u32    dsr;
    __u32    rng;
    __u32    dcd;
    __u32    rx;
    __u32    tx;
    __u32    frame;
    __u32    overrun;
    __u32    parity;
    __u32    brk;
};

struct sdio_uart_port {
    struct kref        kref;
    struct tty_struct    *tty;
    struct tty_port		port;
    unsigned int        index;
    unsigned int        opened;
    struct mutex        open_lock;
    struct sdio_func    *func;
    struct mutex        func_lock;
    struct task_struct    *in_sdio_uart_irq;
    unsigned int        regs_offset;
    struct circ_buf        xmit;
    spinlock_t        write_lock;
    struct uart_icount    icount;
    unsigned int        uartclk;
    unsigned int        mctrl;
    unsigned int        read_status_mask;
    unsigned int        ignore_status_mask;
    unsigned char        x_char;
    unsigned char           ier;
    unsigned char           lcr;
    PUART_BUF_INFO buf_info;
    unsigned char modem_sig_read_done;
    unsigned char msr;
    spinlock_t read_lock;
    struct semaphore read_write_sema;
};

struct work_struct sdio_write_init;
struct sdio_uart_port *active_port;
static struct sdio_uart_port *sdio_uart_table[UART_NR];
static DEFINE_SPINLOCK(sdio_uart_table_lock);


#include <linux/scatterlist.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>

#include "../core/core.h"
#include "../core/sdio_ops.h"


void transmit_packet_to_sdio(void);
void transmit_packets_to_sdio(void);


int huawei_mmc_io_rw_extended(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz, 
	struct scatterlist* sg, int sg_count)
{
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	//struct scatterlist sg;

	BUG_ON(!card);
	BUG_ON(fn > 7);
	BUG_ON(blocks == 1 && blksz > 512);
	WARN_ON(blocks == 0);
	WARN_ON(blksz == 0);

	/* sanity check */
	if (addr & ~0x1FFFF)
		return -EINVAL;

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = SD_IO_RW_EXTENDED;
	cmd.arg = write ? 0x80000000 : 0x00000000;
	cmd.arg |= fn << 28;
	cmd.arg |= incr_addr ? 0x04000000 : 0x00000000;
	cmd.arg |= addr << 9;
	if (blocks == 1 && blksz <= 512)
		cmd.arg |= (blksz == 512) ? 0 : blksz;	/* byte mode */
	else
		cmd.arg |= 0x08000000 | blocks;		/* block mode */
	cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;

	data.blksz = blksz;
	data.blocks = blocks;
	data.flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
	data.sg = sg;
	data.sg_len = sg_count;

	//sg_init_one(&sg, buf, blksz * blocks);
	
	mmc_set_data_timeout(&data, card);

	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error)
		return cmd.error;
	if (data.error)
		return data.error;

	if (mmc_host_is_spi(card->host)) {
		/* host driver already reported errors */
	} else {
		if (cmd.resp[0] & R5_ERROR)
			return -EIO;
		if (cmd.resp[0] & R5_FUNCTION_NUMBER)
			return -EINVAL;
		if (cmd.resp[0] & R5_OUT_OF_RANGE)
			return -ERANGE;
	}

	return 0;
}



static inline unsigned int sdio_max_byte_size(struct sdio_func *func)
{
	unsigned mval =	min(func->card->host->max_seg_size,
			    func->card->host->max_blk_size);

	if (mmc_blksz_for_byte_mode(func->card))
		mval = min(mval, func->cur_blksize);
	else
		mval = min(mval, func->max_blksize);

	return min(mval, 512u); /* maximum size for byte mode */
}



/*************************************************************************
description: this routine uses sg table to send data, it need the sending
             data length is multiple of block size, this means size%block_size 
             equal zero. only support block mode transmiting.

*************************************************************************/

static int huawei_sdio_io_rw(struct sdio_func *func, int write,
	unsigned addr, int incr_addr, u8 *buf, unsigned size, struct scatterlist* sg)
{
	unsigned remainder = size;
	unsigned max_blocks;
    unsigned int offset = 0;
	int ret, i= 0, sg_count;
    struct scatterlist* psg = sg;

    if (sg == NULL)
    {
        printk("%s: sg is null\n", __FUNCTION__);
        return -1;
    }
    if (!func->card->cccr.multi_block)
    {
        printk("%s: multi bolck transmiting not support\n", __FUNCTION__);
        return -1;
    }
    if (size % func->cur_blksize)
    {
        printk("%s: data length is not multiple of block size,cur_blksize=%d\n",  __FUNCTION__,func->cur_blksize);
        return -1;
    }

    max_blocks = size/func->cur_blksize;
    if (max_blocks > func->card->host->max_blk_count ||
        max_blocks > 511u)
    {
        printk("%s: data is too long to transmit\n", __FUNCTION__);
        return -1;
    }

    //init sg table entry
    sg_count = size/func->card->host->max_seg_size;
    if (size%func->card->host->max_seg_size)
    {
        sg_count++;
    }
    
    for (i = 0; i < sg_count; i++)
    {
        sg_init_one(psg, buf + offset, func->card->host->max_seg_size);
        offset += func->card->host->max_seg_size;

        //clear the end sg mark
        psg->page_link &= ~0x2;
        psg++;
    }

    //mark the end sg
    psg--;
    psg->page_link |= 0x2;
    if (size%func->card->host->max_seg_size)
    {
        psg->length = size%func->card->host->max_seg_size;
    }
    
    ret = huawei_mmc_io_rw_extended(func->card, write,func->num, addr, 
                      incr_addr, buf,max_blocks, func->cur_blksize, sg, sg_count);
    return ret;
           
	/* Do the bulk of the transfer using block mode (if supported). */
	if (func->card->cccr.multi_block && (size > sdio_max_byte_size(func))) 
    {
		/* Blocks per command is limited by host count, host transfer
		 * size (we use sg table ) and the maximum for
		 * IO_RW_EXTENDED of 511 blocks. */
		max_blocks = min(func->card->host->max_blk_count,
			func->card->host->max_seg_size*
            func->card->host->max_segs / func->cur_blksize);
		max_blocks = min(max_blocks, 511u);
        
		while (remainder > func->cur_blksize) 
        {
			unsigned blocks;

            blocks = remainder / func->cur_blksize;
			if (blocks > max_blocks)
				blocks = max_blocks;
			size = blocks * func->cur_blksize;


            sg_count = size/func->card->host->max_seg_size;
            
            if (remainder%func->card->host->max_seg_size)
            {
                sg_count++;
            }
            //printk("%s: sg count=%d\n", __FUNCTION__, sg_count);
            
            for (i = 0; i < sg_count; i++)
            {
                sg_init_one(psg, buf + offset, func->card->host->max_seg_size);
                offset += func->card->host->max_seg_size;

                //clear the end sg mark
                psg->page_link &= ~0x2;
                //printk("%s:addr=0x%p, len=%d, offset=%d, link=0x%x\n", 
                //__FUNCTION__, psg->dma_address, psg->length, psg->offset, 
                //psg->page_link);

                psg++;
            }

            //mark the end sg
            psg--;
            psg->page_link |= 0x2;
            
			ret = huawei_mmc_io_rw_extended(func->card, write,
				func->num, addr, incr_addr, buf,
				blocks, func->cur_blksize, sg, sg_count);
           if (ret)
				return ret;

			remainder -= size;
			buf += size;
			if (incr_addr)
				addr += size;
		}
	}

	
	return 0;
}

int huawei_sdio_readsb(struct sdio_func *func, void *dst, unsigned int addr,
	int count, struct scatterlist* sg)
{
	return huawei_sdio_io_rw(func, 0, addr, 0, dst, count, sg);
}


int huawei_sdio_writesb(struct sdio_func *func, unsigned int addr, void *src,
	int count, struct scatterlist* sg)
{
	return huawei_sdio_io_rw(func, 1, addr, 0, src, count, sg);
}



static int sdio_uart_add_port(struct sdio_uart_port *port)
{
    int index, ret = -EBUSY;

    kref_init(&port->kref);
    mutex_init(&port->open_lock);
    mutex_init(&port->func_lock);
    spin_lock_init(&port->write_lock);
    spin_lock_init(&port->read_lock);

    spin_lock(&sdio_uart_table_lock);
    for (index = 0; index < UART_NR; index++) {
        if (!sdio_uart_table[index]) {
            port->index = index;
            sdio_uart_table[index] = port;
            ret = 0;
            break;
        }
    }
    spin_unlock(&sdio_uart_table_lock);

    return ret;
}

static struct sdio_uart_port *sdio_uart_port_get(unsigned index)
{
    struct sdio_uart_port *port;

    if (index >= UART_NR)
        return NULL;

    spin_lock(&sdio_uart_table_lock);
    
    port = sdio_uart_table[index];
    if (port)
        kref_get(&port->kref);
    spin_unlock(&sdio_uart_table_lock);

    return port;
}

static void sdio_uart_port_destroy(struct kref *kref)
{
    printk("%s:enter\n", __FUNCTION__);
    struct sdio_uart_port *port =
        container_of(kref, struct sdio_uart_port, kref);
    kfree(port);
}

static void sdio_uart_port_put(struct sdio_uart_port *port)
{
    kref_put(&port->kref, sdio_uart_port_destroy);
}

static void sdio_uart_port_remove(struct sdio_uart_port *port)
{
    struct sdio_func *func;

    BUG_ON(sdio_uart_table[port->index] != port);

    spin_lock(&sdio_uart_table_lock);
    sdio_uart_table[port->index] = NULL;
    spin_unlock(&sdio_uart_table_lock);

    /*
     * We're killing a port that potentially still is in use by
     * the tty layer. Be careful to prevent any further access
     * to the SDIO function and arrange for the tty layer to
     * give up on that port ASAP.
     * Beware: the lock ordering is critical.
     */
    mutex_lock(&port->open_lock);
    mutex_lock(&port->func_lock);
    func = port->func;
    port->func = NULL;
    mutex_unlock(&port->func_lock);
    if (port->opened)
        tty_hangup(port->tty);
    mutex_unlock(&port->open_lock);
    //sdio_claim_host(func);
    //sdio_release_irq(func);
    //sdio_disable_func(func);
    //sdio_release_host(func);

    //sdio_uart_port_put(port);
}

static int sdio_uart_claim_func(struct sdio_uart_port *port)
{
    mutex_lock(&port->func_lock);
    if (unlikely(!port->func)) {
        mutex_unlock(&port->func_lock);
        return -ENODEV;
    }
    if (likely(port->in_sdio_uart_irq != current))
        sdio_claim_host(port->func);
    mutex_unlock(&port->func_lock);
    return 0;
}

static inline void sdio_uart_release_func(struct sdio_uart_port *port)
{
    if (likely(port->in_sdio_uart_irq != current))
        sdio_release_host(port->func);
}

static inline unsigned int sdio_in(struct sdio_uart_port *port, int offset)
{
    unsigned char c;
    
    //spin_lock(&port->write_lock);
    //c = sdio_readb(port->func, port->regs_offset + offset, NULL);
    sdio_readsb(port->func, &c, port->regs_offset + offset, sizeof(unsigned char));
    //spin_unlock(&port->write_lock);

    return c;
}

static inline void sdio_out(struct sdio_uart_port *port, int offset, int value)
{
    //spin_lock(&port->write_lock);
    //sdio_writeb(port->func, value, port->regs_offset + offset, NULL);
    sdio_writesb(port->func, port->regs_offset + offset, &value, sizeof(unsigned char));
    //spin_unlock(&port->write_lock);

}


static unsigned int sdio_uart_get_mctrl(struct sdio_uart_port *port)
{
    unsigned char status;
    unsigned int ret;
    unsigned char tmp[50];
    PMUX_HEAD phead;
    unsigned int len;
    int rtn;

    #if 0
    FUNC_ENTER

    if (port->index != 0) // not modem port
    {
        return 0;
    }

    if (port->modem_sig_read_done)
    {
        sdio_uart_release_func(port);
        goto done;
    }
    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, &mux_head[port->index], sizeof(MUX_HEAD));
    tmp[sizeof(MUX_HEAD)] = MODEM_SIG_READ;
    phead = (PMUX_HEAD)tmp;
    phead->data_len = 2;
    phead->port_index |= 0xf0;
    len = sizeof(MUX_HEAD) + 2;

    if (len%4)
    {
        len = (len/4 + 1)*4;
    }

    rtn = sdio_writesb(port->func, 0x00, tmp, len);
    sdio_uart_release_func(port);
    
    if (rtn)
    {
        printk("%s:send modem sig read cmd fail\n", __FUNCTION__);
        return 0;
    }
    
    while(!port->modem_sig_read_done)
    {
        mdelay(10);
    }

    done:

    port->modem_sig_read_done = 0;
    #endif
    
    status = port->msr;
    ret = 0;

    if (status & UART_MSR_DCD)
        ret |= TIOCM_CAR;
    if (status & UART_MSR_RI)
        ret |= TIOCM_RNG;
    if (status & UART_MSR_DSR)
        ret |= TIOCM_DSR;
    if (status & UART_MSR_CTS)
        ret |= TIOCM_CTS;
    return ret;
}

static void sdio_uart_write_mctrl(struct sdio_uart_port *port, unsigned int mctrl)
{
    unsigned char mcr = 0;
    unsigned char tmp[50];
    PMUX_HEAD phead;
    unsigned int len;
    int rtn;

    FUNC_ENTER

    if (mctrl & TIOCM_RTS)
        mcr |= UART_MCR_RTS;
    if (mctrl & TIOCM_DTR)
        mcr |= UART_MCR_DTR;
    if (mctrl & TIOCM_OUT1)
        mcr |= UART_MCR_OUT1;
    if (mctrl & TIOCM_OUT2)
        mcr |= UART_MCR_OUT2;
    if (mctrl & TIOCM_LOOP)
        mcr |= UART_MCR_LOOP;

    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, &mux_head[port->index], sizeof(MUX_HEAD));
    tmp[sizeof(MUX_HEAD)] = MODEM_SIG_WRITE;
    tmp[sizeof(MUX_HEAD) + 1] = mcr;
    phead = (PMUX_HEAD)tmp;
    phead->data_len = 2;
    phead->port_index |= 0xf0;
    len = sizeof(MUX_HEAD) + 2;

    if (len%4)
    {
        len = (len/4 + 1)*4;
    }

    rtn = sdio_writesb(port->func, 0x00, tmp, len);
    if (rtn)
    {
        printk("%s:write modem sig fail\n", __FUNCTION__);
    }
    
    //sdio_out(port, UART_MCR, mcr);
}

static inline void sdio_uart_update_mctrl(struct sdio_uart_port *port,
                      unsigned int set, unsigned int clear)
{
    unsigned int old;
    FUNC_ENTER
    old = port->mctrl;
    port->mctrl = (old & ~clear) | set;
    if (old != port->mctrl)
        sdio_uart_write_mctrl(port, port->mctrl);
}

#define sdio_uart_set_mctrl(port, x)    sdio_uart_update_mctrl(port, x, 0)
#define sdio_uart_clear_mctrl(port, x)    sdio_uart_update_mctrl(port, 0, x)

static void sdio_uart_start_tx(struct sdio_uart_port *port)
{
FUNC_ENTER
    //printk("port->ier= 0x%x,    sdio_uart_start_tx\n",port->ier );//0x01
    if (!(port->ier & UART_IER_THRI)) {
        port->ier |= UART_IER_THRI;
        sdio_out(port, UART_IER, port->ier);
    }
FUNC_EXIT
}

#if 0
static void sdio_uart_change_speed(struct sdio_uart_port *port,
                   struct ktermios *termios,
                   struct ktermios *old)
{
    unsigned char cval, fcr = 0;
    unsigned int baud, quot;
    FUNC_ENTER
    switch (termios->c_cflag & CSIZE) {
    case CS5:
        cval = UART_LCR_WLEN5;
        break;
    case CS6:
        cval = UART_LCR_WLEN6;
        break;
    case CS7:
        cval = UART_LCR_WLEN7;
        break;
    default:
    case CS8:
        cval = UART_LCR_WLEN8;
        break;
    }

    if (termios->c_cflag & CSTOPB)
        cval |= UART_LCR_STOP;
    if (termios->c_cflag & PARENB)
        cval |= UART_LCR_PARITY;
    if (!(termios->c_cflag & PARODD))
        cval |= UART_LCR_EPAR;

    for (;;) {
        baud = tty_termios_baud_rate(termios);
        if (baud == 0)
            baud = 9600;  /* Special case: B0 rate. */
        if (baud <= port->uartclk)
            break;
        /*
         * Oops, the quotient was zero.  Try again with the old
         * baud rate if possible, otherwise default to 9600.
         */
        termios->c_cflag &= ~CBAUD;
        if (old) {
            termios->c_cflag |= old->c_cflag & CBAUD;
            old = NULL;
        } else
            termios->c_cflag |= B9600;
    }
    quot = (2 * port->uartclk + baud) / (2 * baud);

    if (baud < 2400)
        fcr = UART_FCR_ENABLE_FIFO | UART_FCR_TRIGGER_1;
    else
        fcr = UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10;

    port->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
    if (termios->c_iflag & INPCK)
        port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
    if (termios->c_iflag & (BRKINT | PARMRK))
        port->read_status_mask |= UART_LSR_BI;

    /*
     * Characters to ignore
     */
    port->ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR)
        port->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
    if (termios->c_iflag & IGNBRK) {
        port->ignore_status_mask |= UART_LSR_BI;
        /*
         * If we're ignoring parity and break indicators,
         * ignore overruns too (for real raw support).
         */
        if (termios->c_iflag & IGNPAR)
            port->ignore_status_mask |= UART_LSR_OE;
    }

    /*
     * ignore all characters if CREAD is not set
     */
    if ((termios->c_cflag & CREAD) == 0)
        port->ignore_status_mask |= UART_LSR_DR;

    /*
     * CTS flow control flag and modem status interrupts
     */
    port->ier &= ~UART_IER_MSI;
    if ((termios->c_cflag & CRTSCTS) || !(termios->c_cflag & CLOCAL))
        port->ier |= UART_IER_MSI;

    port->lcr = cval;

    sdio_out(port, UART_IER, port->ier);
    sdio_out(port, UART_LCR, cval | UART_LCR_DLAB);
    sdio_out(port, UART_DLL, quot & 0xff);
    sdio_out(port, UART_DLM, quot >> 8);
    sdio_out(port, UART_LCR, cval);
    sdio_out(port, UART_FCR, fcr);

    sdio_uart_write_mctrl(port, port->mctrl);
}



static void sdio_uart_stop_tx(struct sdio_uart_port *port)
{
FUNC_ENTER
    

    if (port->ier & UART_IER_THRI) {
        port->ier &= ~UART_IER_THRI;
        sdio_out(port, UART_IER, port->ier);
    }
FUNC_EXIT
}

static void sdio_uart_stop_rx(struct sdio_uart_port *port)
{
FUNC_ENTER
    port->ier &= ~UART_IER_RLSI;
    port->read_status_mask &= ~UART_LSR_DR;
    sdio_out(port, UART_IER, port->ier);
FUNC_EXIT
}

static void sdio_uart_check_modem_status(struct sdio_uart_port *port)
{
    int status;

    status = sdio_in(port, UART_MSR);
    FUNC_ENTER

    
    if ((status & UART_MSR_ANY_DELTA) == 0)
        return;

    if (status & UART_MSR_TERI)
        port->icount.rng++;
    if (status & UART_MSR_DDSR)
        port->icount.dsr++;
    if (status & UART_MSR_DDCD)
        port->icount.dcd++;
    if (status & UART_MSR_DCTS) {
        port->icount.cts++;
        if (port->tty->termios->c_cflag & CRTSCTS) {
            int cts = (status & UART_MSR_CTS);
            if (port->tty->hw_stopped) {
                if (cts) {
                    port->tty->hw_stopped = 0;
                    HW_DBG("sdio_uart_check_modem_status\n");
                    sdio_uart_start_tx(port);
                    tty_wakeup(port->tty);
                }
            } else {
                if (!cts) {
                    port->tty->hw_stopped = 1;
                    sdio_uart_stop_tx(port);
                }
            }
        }
    }
    FUNC_EXIT

}
#endif
int sdio_uart_read_mulpackets(struct sdio_func* func, u32 len)
{
    int i, ret, length, offset = 0;
    unsigned char* pbuf;
    PMUX_HEAD phead;
    int head_len = sizeof(MUX_HEAD);
    struct tty_struct *tty;
    struct sdio_uart_port *mux_port;
    static unsigned int icount = 0;

    if (len > sizeof(g_read_buffer))
    {
        printk("%s: data length is bigger than 256k\n", __FUNCTION__);
        return 1;
    }
    ret = huawei_sdio_readsb(func, g_read_buffer, 0x00, len, g_sg_table.sgl);
    //ret = sdio_readsb(func, g_read_buffer, 0x00, len);
    if (ret)
    {
        printk("%s: read data fail %d\n", __FUNCTION__, ret);
        return 1;
    }

    //icount++;
    //if (icount%50 == 0)
    {
       // printk("%s: read %d bytes data success\n", __FUNCTION__, len);
        //return 1;
    }
    
    while (offset < len)
    {
        pbuf = &g_read_buffer[offset];
        phead = (PMUX_HEAD)pbuf;
        if (phead->head_tag != 0xf9 || phead->ctrl_tag != 0xef ||
        (phead->port_index & 0x0f) < 1 ||(phead->port_index & 0x0f) > 3)
        {
            #if 0
            printk("%s:head error, head tag=0x%x, ctrl tag=0x%x port index=0x%x len=%d offset =%d\n",
                __FUNCTION__, phead->head_tag, phead->ctrl_tag, 
                phead->port_index, len, offset);
            
            for(i = offset; i < offset + head_len; i++)
            {
                if ((i - offset)%16 == 0)
                {
                    printk("\n");
                }
                printk("0x%02x, ", *(pbuf + i));
                
            }
            printk("\n");
            printk("%s: start=0x%p, err=0x%p\n", __FUNCTION__, g_read_buffer, pbuf);
            #endif
            return 1;
        }

        if (phead->data_len == 0)
        {
            return 1;
        }
        
       // printk("%s: port %d get data %d bytes\n", __FUNCTION__, phead->port_index,
        //                                                        phead->data_len);
        if (modem_opened)
        {
            for (i = 0; i < head_len + phead->data_len; i++)
            {
                printk("%s: buf[%d]=0x%x\n", __FUNCTION__, i, *(pbuf + i));
            }
           // printk("%s:get data:%s\n", __FUNCTION__, pbuf + head_len);
        }
        
        mux_port = sdio_uart_port_get((phead->port_index & 0x0f) - 1);
        if (NULL == mux_port)
        {
            printk("%s: maybe port is not opened\n", __FUNCTION__);
            goto go_on;
        }
        
        
        //tty = tty_port_tty_get(&mux_port->port);
        tty = mux_port->tty;
        if (NULL == tty)
        {
            printk("%s: %p: tty is null\n", __FUNCTION__, mux_port);
            goto go_on;
        }
        if (phead->port_index == 1)
        {
            //icount++;
        }
        if (icount == 100)
        {
            icount = 0;
            if (tty && !C_CLOCAL(tty))
            {
                printk("%s: DCD down, hang up the ppp\n", __FUNCTION__);
				tty_hangup(tty);
            }
			tty_kref_put(tty);
            return 0;
        }
        if (phead->port_index == 0xf1) // modem ctrl sig
        {
            unsigned char old_state;
            old_state = mux_port->msr;
            mux_port->msr = *(pbuf + head_len);
            mux_port->modem_sig_read_done = 1;

            if ((old_state & UART_MSR_DCD) && !(mux_port->msr & UART_MSR_DCD))
            {
                if (tty && !C_CLOCAL(tty))
                {
                    printk("%s: DCD down, hang up the ppp\n", __FUNCTION__);
					tty_hangup(tty);
                }
				//tty_kref_put(tty);
            }
            goto go_on;
        }
        
        //printk("%s: index=%d\n", __FUNCTION__, tty->index);
        ret = tty_insert_flip_string(tty, pbuf + head_len, phead->data_len);
        tty_flip_buffer_push(tty);
    	//tty_kref_put(tty);
        mux_port->icount.rx += phead->data_len; 
        /*
        if (phead->port_index == 1)
        {
            for(i= 0; i < phead->data_len; i++)
            {
                printk("%s: modem_buf[%d]=0x%x\n", __FUNCTION__, i, 
                    *(pbuf + head_len + i));
                
            }
        }
        */
    go_on:
        length = head_len + phead->data_len;
        #if 0
        if (length%512)
        {
            length = (length/512 + 1)*512;
        }
        #endif
        offset += length;
       // printk("%s: %d bytes data writed to tty from port %d\n", __FUNCTION__, 
      //      ret, phead->port_index);
    }
    
    return 0;
    
}
    
int sdio_uart_read_chars(struct sdio_func* func, u32 len)
{
    int ret;
    PMUX_HEAD phead;
    int head_len = sizeof(MUX_HEAD);
    struct tty_struct *tty;
    struct sdio_uart_port *mux_port;
    unsigned int op_code;  
    static char icount = 1;
    
    
    ret = sdio_readsb(func, g_read_buffer, 0x00, len);
    if (ret)
    {
        printk("%s: read data fail %d\n", __FUNCTION__, ret);
        return 1;
    }

    phead = (PMUX_HEAD)g_read_buffer;

    
    if (phead->head_tag != 0xf9 || phead->ctrl_tag != 0xef ||
        (phead->port_index & 0x0f) < 1 || 
        (phead->port_index & 0x0f) > 3)
    {
        printk("%s: mux head error, head tag=0x%x, ctrl tag=0x%x\
            port index=0x%x\n", __FUNCTION__, phead->head_tag, phead->ctrl_tag,
            phead->port_index);
        return 1;
    }

    if (phead->data_len == 0)
    {
        return 1;
    }
    
   
    mux_port = sdio_uart_port_get((phead->port_index & 0x0f) - 1);
    if (NULL == mux_port)
    {
        printk("%s: maybe port is not opened\n", __FUNCTION__);
        return 1;
    }
    
    
    //tty = tty_port_tty_get(&mux_port->port);
    tty = mux_port->tty;
    if (NULL == tty)
    {
        printk("%s: %p: tty is null\n", __FUNCTION__, mux_port);
        return 1;
    }

    if (phead->port_index & 0xf0) // modem ctrl sig
    {
        mux_port->msr = g_read_buffer[head_len];
        mux_port->modem_sig_read_done = 1;
        return 0;
    }
    
    //printk("%s: index=%d\n", __FUNCTION__, tty->index);
    ret = tty_insert_flip_string(tty, &g_read_buffer[head_len], phead->data_len);
    tty_flip_buffer_push(tty);
	tty_kref_put(tty);
    mux_port->icount.rx += phead->data_len; 
   // printk("%s: %d bytes data writed to tty\n", __FUNCTION__, ret);
    if (icount == 0)
    {
        op_code = 0x90008104;
        ret = sdio_memcpy_toio(active_port->func, 0x24, &op_code, 4);
        icount++;
    }
    return 0;
    
}

/*
 * This handles the interrupt from one port.
 */
static void sdio_uart_irq(struct sdio_func *func)
{
//    struct sdio_uart_port *port = sdio_get_drvdata(func);
    struct sdio_uart_port *port = active_port;
    //unsigned int iir, lsr;
    int err_ret;
    u32 data_len = 0;
    u8 reg;
    
    static unsigned int i = 1;
    static unsigned int send_len = 5*1024;
    /*
     * In a few places sdio_uart_irq() is called directly instead of
     * waiting for the actual interrupt to be raised and the SDIO IRQ
     * thread scheduled in order to reduce latency.  However, some
     * interaction with the tty core may end up calling us back
     * (serial echo, flow control, etc.) through those same places
     * causing undesirable effects.  Let's stop the recursion here.
     */

    //FUNC_ENTER
    //spin_lock(&active_port->read_lock);

    if (i == 1)
    {
        //printk("%s: transmit 0 bytes, packet len=%d\n", __FUNCTION__, send_len);
    }
    
    sdio_claim_host(port->func);
	    
    #if 0
    // 关闭slave设备报给host的中断,  使用CMD52写FUN1的0x09寄存器, 参数为0x00
    reg = 0x00;
    sdio_writeb(func, reg, 0x09, &err_ret);
    if (err_ret)
    {
        printk("write func interrupt reg fail %d\n", __FUNCTION__, err_ret);
    }

    #endif
    
    // 读取slave设备上报的中断状态, 使用CMD52读FUN1的0x08寄存器, 得到IntStatus
    reg = sdio_readb(func, 0x08, &err_ret);
    if (err_ret)
    {
        printk("%s: read func interrupt id reg fail %d\n", __FUNCTION__, err_ret);
    }

    // 清除中断状态, 使用CMD52写FUN1的0x08寄存器，将步骤2中的IntStatus原样写回

    sdio_writeb(func, reg, 0x08, &err_ret);
    if (err_ret)
    {
        printk("%s: write func interrupt reg fail %d\n", __FUNCTION__, err_ret);
    }
    //printk("%s: func1 int id reg = 0x%x\n", __FUNCTION__, reg);

    
    if (reg & 0x01)
    {
       err_ret = sdio_memcpy_fromio(func, &data_len, 0x0c, 4);
       if (err_ret)
       {
            printk("%s: read data len fail\n", __FUNCTION__);
       }
       else
       {
            //printk("%s: data_len = %d\n", __FUNCTION__, data_len);
       }
       
       if (data_len)
       {
            sdio_uart_read_mulpackets(func, data_len);
       }
        
    }

    else if (reg & 0x04)
    {
        //bfree = 1;
        #if 0
        err_ret =sdio_memcpy_fromio(func, &data_len, 0x28, 4);
        if (err_ret)
        {
            printk("%s: read message info fail\n", __FUNCTION__);
        }
        #endif
        
        #if 0
        
        g_read_buffer[0] = i;
        
         
        reg = huawei_sdio_writesb(func, 0x00, g_read_buffer, send_len, g_sg_table.sgl);
        //reg = sdio_writesb(func, 0x00, g_read_buffer, 7*1024);
        if (reg)
        {
            printk("%s: send test packet(len=100K) fail\n", __FUNCTION__);
        }
        else
        {
            if ( i%100 == 0)
            {
                printk("%s: transmit %d * %d bytes\n", __FUNCTION__, i, send_len);
            }
            i++;
            if (i%1000 == 0)
            {
                send_len += 5*1024;
                if (send_len > 250 *1024)
                {
                    send_len = 5*1024;
                }
            }
            
            //printk("%s: send test packet(len=100K) success\n", __FUNCTION__);
        }
       #else 

       //transmit_packets_to_sdio();
       #endif
        
    }

    
    #if 0
    /* 打开slave设备报给host的中断, 使用CMD52写FUN1的0x09地址寄存器，数据为0x0f */
	reg = 0x0f;
    sdio_writeb(func, reg, 0x09, &err_ret);
    if (err_ret)
    {
        printk("%s:write func interrupt mask reg fail %d\n", __FUNCTION__, err_ret);
    }

    #endif
    
    
    sdio_release_host(port->func);

    #if 0
    if (bfree)
    {
        printk("%s: wake up write thread\n", __FUNCTION__);
    }
    #endif
    //spin_unlock(&active_port->write_lock);

   // FUNC_EXIT
}

static int sdio_func_startup(struct sdio_uart_port *port)
{
    int ret;

    FUNC_ENTER
    
    ret = sdio_uart_claim_func(port);
    if (ret)
    {
        goto end;
    }
    
    ret = sdio_enable_func(port->func);
    
    if (ret)
    {
        printk("%s: enable func fail %d\n", __FUNCTION__, ret);
        goto end;
    }

    sdio_writeb(port->func, 0x0f, 0x09, &ret);
    if (ret)
    {
        printk("%s: write 0x09 reg fail\n", __FUNCTION__);
    }
    
    if(port ->func->irq_handler)
    {
        port ->func->irq_handler = NULL;
    }
    
    ret = sdio_claim_irq(port->func, sdio_uart_irq);
    if (ret)
    {
        printk("%s: register irq fail %d\n", __FUNCTION__, ret);
        sdio_disable_func(port->func);
    }

end:
    sdio_uart_release_func(port);
    
    FUNC_EXIT
    return ret;
}

static int sdio_uart_open (struct tty_struct *tty, struct file * filp)
{
    struct sdio_uart_port *port;
    int ret = 0;

    FUNC_ENTER
    //dump_stack();
    printk("%s: index=%d\n", __FUNCTION__, tty->index);
    port = sdio_uart_port_get(tty->index - 1);

    if (tty->index == 1)
    {
       // modem_opened = 1;
    }
    
    if (!port)
    {
        printk("%s: port error\n", __FUNCTION__);
        return -ENODEV;
    }
    //printk("%s:tty index = %d port index = %d\n", __FUNCTION__, tty->index, port->index);
    mutex_lock(&port->open_lock);
    
    /*
     * Make sure not to mess up with a dead port
     * which has not been closed yet.
     */
    if (tty->driver_data && tty->driver_data != port) 
    {
         //printk("tty->driver_data = %x,port = %x\n",tty->driver_data,port);
        mutex_unlock(&port->open_lock);
        sdio_uart_port_put(port);
        printk("%s: port mismatch\n", __FUNCTION__);
        return -EBUSY;
    }

    
    tty->driver_data = port;
    port->tty = tty;
    port->opened++;
    active_port = port;
    mutex_unlock(&port->open_lock);
    bExit = 0;
    
    FUNC_EXIT
    return ret;
}

static void sdio_uart_close(struct tty_struct *tty, struct file * filp)
{
    struct sdio_uart_port *port = tty->driver_data;
    
    FUNC_ENTER

    printk("%s: index=%d\n", __FUNCTION__, tty->index);
    if (!port)
        return;
    
    mutex_lock(&port->open_lock);
    BUG_ON(!port->opened);

    /*
     * This is messy.  The tty layer calls us even when open()
     * returned an error.  Ignore this close request if tty->count
     * is larger than port->count.
     */
    if (tty->count > port->opened) {
        mutex_unlock(&port->open_lock);
        return;
    }

    if (--port->opened == 0)
    {
        tty->closing = 1;
        tty_ldisc_flush(tty);
        //printk("%s:pos2\n", __FUNCTION__);
        port->tty = NULL;
        tty->driver_data = NULL;
        tty->closing = 0;
      //  active_port = NULL;
    }
    mutex_unlock(&port->open_lock);
    
    sdio_uart_port_put(port);
    
    if (bExit)
    {
        kfree(port);
    }
    FUNC_EXIT

}

int tty_data_process(struct sdio_uart_port* port, const unsigned char* buf, int count)
{
    int len, offset=0;
    unsigned char* p = tty_data_buf.buf;

    /*
    if (count + sizeof(MUX_HEAD) > SDIO_UART_MAX_WRITE_SIZE)
    {
        printk("%s: data length is too big to send(%d)\n", __FUNCTION__, count);
        return 0;
    }
    */
    mux_head[port->index].data_len = count;
    //memcpy(tmp, &mux_head[port->index], sizeof(MUX_HEAD));

    //memcpy(tmp + sizeof(MUX_HEAD), buf, count);
    
    len = count + sizeof(MUX_HEAD);
    //nantao deleted begin 03.21
    if (len%4)
    {
        len = (len/4 + 1)*4;
    }
    //nantao deleted end 03.21

   // printk("%s: wt_pos=%d,rd_pos=%d, len=%d\n", __FUNCTION__, tty_data_buf.wt_pos, tty_data_buf.rd_pos, len);
    if (tty_data_buf.wt_pos + len >= tty_data_buf.size)
    {
        while (len >= tty_data_buf.rd_pos)
        {
            printk("%s(0): wt_pos=%d,rd_pos=%d, len=%d\n", __FUNCTION__, 
                tty_data_buf.wt_pos, tty_data_buf.rd_pos, len);
            /*BEGIN:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/
            msleep(1);
            /*END:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/
        }
        
        memcpy(p, &mux_head[port->index], sizeof(MUX_HEAD));
        offset = sizeof(MUX_HEAD);
        memcpy(p + offset, buf, count);
        /*BEGIN:DTS2014042209407  n00203193 DELETED for SD Modem 2014.04.15*/
        //*(p + offset + count) = 0;
        /*END:DTS2014042209407  n00203193 DELETED for SD Modem 2014.04.15*/
        //memcpy(tty_data_buf.buf, tmp, len);
        tty_data_buf.rtn_pos = tty_data_buf.wt_pos;
        tty_data_buf.wt_pos = len;
    }
    else
    {
        while (tty_data_buf.wt_pos < tty_data_buf.rd_pos &&
            tty_data_buf.wt_pos + len >= tty_data_buf.rd_pos)
        {
            printk("%s(1): wt_pos=%d,rd_pos=%d, len=%d\n", __FUNCTION__, 
                tty_data_buf.wt_pos, tty_data_buf.rd_pos, len);
            /*BEGIN:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/
            msleep(1);
            /*END:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/
        }
        offset = tty_data_buf.wt_pos;
        memcpy(p + offset, &mux_head[port->index], sizeof(MUX_HEAD));
        offset += sizeof(MUX_HEAD);
        memcpy(p + offset, buf, count);
        //memcpy(tty_data_buf.buf + tty_data_buf.wt_pos, tmp, len);
        tty_data_buf.wt_pos += len;
    
    }

    /*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/ 
    *(p + tty_data_buf.wt_pos) = 0xff;
    /*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/     
    
    /*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/        
    if (port->index == 0)
    {   
        //printk("%s: modem data pos=%d\n", __FUNCTION__, tty_data_buf.wt_pos);
        g_modem_pkg_gather ++;
        if(g_modem_pkg_gather == MODEM_GATHER_MAX)
        {
            g_modem_pkg_gather = 0;
            wake_up(&thread_wait_q);
        }
    }
    /*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/  
    return count;
}

static int sdio_uart_write(struct tty_struct * tty, const unsigned char *buf,
               int count)
{
    int i, ret = 0;

    /*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/ 
    //char UartWt[100] = {0};
    //int n =0;
    /*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.15*/ 
    struct sdio_uart_port *port = tty->driver_data;
    
    if (bExit)
    {
        return ENODEV;
    }
    if (port == NULL)
    {
        printk("%s: port is null\n", __FUNCTION__);
        return ENODEV;
    }
    if (buf == NULL)
    {
        printk("%s: input buf is null\n", __FUNCTION__);
        return ENODEV;
    }
    
    if (NULL == active_port)
    {
        return 0;
    }

    /*BEGIN:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/ 
    /*printk("%s: data length = %d\n", __FUNCTION__, count);
    for(i = 0; i < count && i < 16; i++)
    {
        n += sprintf((UartWt + n), "%2x ", *(buf+i));
    }
    printk("%s,UartWt=%s\n", __FUNCTION__, UartWt);
    */
    
    //printk("%s: %d receive data %s\n", __FUNCTION__, port->index, buf);

    //printk("%s,count = %d\n", __FUNCTION__, count);
    //do_gettimeofday(&time1);
    tty_data_process(port, buf, count);
    //do_gettimeofday(&time2);
    /*END:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/ 
    if (port->index == 0)
    {
        //int i, j;
        //char tmp[400];
        //memset(tmp, 0, sizeof(tmp));
        //printk("%s: port 0 write (%d bytes data)\n", __FUNCTION__, count);
        modem_packet_count++;
        #if 0
        for (i = 0, j = 0; i < count && i < 130; i++)
        {
            if (i%16 == 0 && i != 0)
            {
                sprintf(&tmp[j], "\n");
                j++;
            }
            sprintf(&tmp[j], "%02x ", *(buf + i));
            j += 3;
        }
        sprintf(&tmp[j], "\n");
        printk(tmp);
        #endif
    }
    return count;
    //printk("%s: port %d write %d bytes data\n", __FUNCTION__, port->index, count);

}

static int sdio_uart_write_room(struct tty_struct *tty)
{
    FUNC_ENTER
    //struct sdio_uart_port *port = (sdio_uart_port *)tty->driver_data;

    if (tty_data_buf.rd_pos <= tty_data_buf.wt_pos)
    {
        return (tty_data_buf.size - (tty_data_buf.wt_pos - tty_data_buf.rd_pos));
    }
    else
    {
        return (tty_data_buf.rd_pos - tty_data_buf.wt_pos);
    }
    
    //return port ? circ_chars_free(&port->xmit) : 0;
}

static int sdio_uart_chars_in_buffer(struct tty_struct *tty)
{
    struct sdio_uart_port *port = tty->driver_data;
    int ret;
    FUNC_ENTER
    ret = (port ? circ_chars_pending(&port->xmit) : 0);
    printk("chars_in_buffer=%d\n", ret);
    return ret;
}

static void sdio_uart_send_xchar(struct tty_struct *tty, char ch)
{
    struct sdio_uart_port *port = tty->driver_data;

    port->x_char = ch;
    
    printk("%s: enter\n", __FUNCTION__);
    
    if (ch && !(port->ier & UART_IER_THRI)) {
        if (sdio_uart_claim_func(port) != 0)
            return;
        HW_DBG("sdio_uart_send_xchar\n");
        sdio_uart_start_tx(port);
        HW_DBG("call sdio_uart_irq\n");
        sdio_uart_irq(port->func);
        sdio_uart_release_func(port);
    }
}

static void sdio_uart_throttle(struct tty_struct *tty)
{
    struct sdio_uart_port *port = tty->driver_data;

    printk("%s: enter\n", __FUNCTION__);
    if (!I_IXOFF(tty) && !(tty->termios->c_cflag & CRTSCTS))
        return;

    if (sdio_uart_claim_func(port) != 0)
        return;

    if (I_IXOFF(tty)) {
        port->x_char = STOP_CHAR(tty);
        HW_DBG("sdio_uart_throttle\n");
        sdio_uart_start_tx(port);
    }

    if (tty->termios->c_cflag & CRTSCTS)
        sdio_uart_clear_mctrl(port, TIOCM_RTS);
    HW_DBG("call sdio_uart_irq\n");
    sdio_uart_irq(port->func);
    sdio_uart_release_func(port);
}

static void sdio_uart_unthrottle(struct tty_struct *tty)
{
    struct sdio_uart_port *port = tty->driver_data;

    printk("%s: enter\n", __FUNCTION__);
    if (!I_IXOFF(tty) && !(tty->termios->c_cflag & CRTSCTS))
        return;

    if (sdio_uart_claim_func(port) != 0)
        return;

    if (I_IXOFF(tty)) {
        if (port->x_char) {
            port->x_char = 0;
        } else {
            port->x_char = START_CHAR(tty);
            HW_DBG("sdio_uart_unthrottle\n");
            sdio_uart_start_tx(port);
        }
    }

    if (tty->termios->c_cflag & CRTSCTS)
        sdio_uart_set_mctrl(port, TIOCM_RTS);
    HW_DBG("call sdio_uart_irq\n");
    sdio_uart_irq(port->func);
    sdio_uart_release_func(port);
}

static void sdio_uart_set_termios(struct tty_struct *tty, struct ktermios *old_termios)
{
    struct sdio_uart_port *port = tty->driver_data;
    unsigned int cflag = tty->termios->c_cflag;

    printk("%s: enter\n", __FUNCTION__);
    
    return;
#define RELEVANT_IFLAG(iflag)   ((iflag) & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

    if ((cflag ^ old_termios->c_cflag) == 0 &&
        RELEVANT_IFLAG(tty->termios->c_iflag ^ old_termios->c_iflag) == 0)
        return;

    if (sdio_uart_claim_func(port) != 0)
        return;

    //sdio_uart_change_speed(port, tty->termios, old_termios);

    /* Handle transition to B0 status */
    if ((old_termios->c_cflag & CBAUD) && !(cflag & CBAUD))
        sdio_uart_clear_mctrl(port, TIOCM_RTS | TIOCM_DTR);

    /* Handle transition away from B0 status */
    if (!(old_termios->c_cflag & CBAUD) && (cflag & CBAUD)) {
        unsigned int mask = TIOCM_DTR;
        if (!(cflag & CRTSCTS) || !test_bit(TTY_THROTTLED, &tty->flags))
            mask |= TIOCM_RTS;
        sdio_uart_set_mctrl(port, mask);
    }
#if 0
    /* Handle turning off CRTSCTS */
    if ((old_termios->c_cflag & CRTSCTS) && !(cflag & CRTSCTS)) {
        tty->hw_stopped = 0;
        HW_DBG("sdio_uart_set_termios\n");
        sdio_uart_start_tx(port);
    }

    /* Handle turning on CRTSCTS */
    if (!(old_termios->c_cflag & CRTSCTS) && (cflag & CRTSCTS)) {
        if (!(sdio_uart_get_mctrl(port) & TIOCM_CTS)) {
            tty->hw_stopped = 1;
            sdio_uart_stop_tx(port);
        }
    }
#endif
    sdio_uart_release_func(port);
}

static int sdio_uart_break_ctl(struct tty_struct *tty, int break_state)
{
    struct sdio_uart_port *port = tty->driver_data;
    int result;

    printk("%s: enter\n", __FUNCTION__);
    result = sdio_uart_claim_func(port);
    if (result != 0)
        return result;

    if (break_state == -1)
        port->lcr |= UART_LCR_SBC;
    else
        port->lcr &= ~UART_LCR_SBC;
    sdio_out(port, UART_LCR, port->lcr);

    sdio_uart_release_func(port);
    return 0;
}

static int sdio_uart_tiocmget(struct tty_struct *tty)//, struct file *file)
{
    struct sdio_uart_port *port = tty->driver_data;
    int result;
    printk("%s: enter(port %d)\n", __FUNCTION__, port->index);
    return 0;
    result = sdio_uart_claim_func(port);
    if (!result) {
        result = port->mctrl | sdio_uart_get_mctrl(port);
    //    sdio_uart_release_func(port); moved to sdio_uart_get_mctrl()
    }

    return result;
}

static int sdio_uart_tiocmset(struct tty_struct *tty, unsigned int set, 
                                                            unsigned int clear)
{
    struct sdio_uart_port *port = tty->driver_data;
    int result;

    printk("%s: port %d enter set=0x%x, clear=0x%x\n", __FUNCTION__, tty->index,
        set, clear);
    return 0;
    result =sdio_uart_claim_func(port);
    if(!result) {
        sdio_uart_update_mctrl(port, set, clear);
        sdio_uart_release_func(port);
    }

    return result;
}

static int sdio_uart_proc_show(struct seq_file *m, void *v)
{
    int i;

    seq_printf(m, "serinfo:1.0 driver%s%s revision:%s\n",
               "", "", "");
    for (i = 0; i < UART_NR; i++) {
        struct sdio_uart_port *port = sdio_uart_port_get(i);
        if (port) {
            seq_printf(m, "%d: uart:SDIO", i);
            if(capable(CAP_SYS_ADMIN)) {
                seq_printf(m, " tx:%d rx:%d",
                           port->icount.tx, port->icount.rx);
                if (port->icount.frame)
                    seq_printf(m, " fe:%d",
                               port->icount.frame);
                if (port->icount.parity)
                    seq_printf(m, " pe:%d",
                               port->icount.parity);
                if (port->icount.brk)
                    seq_printf(m, " brk:%d",
                               port->icount.brk);
                if (port->icount.overrun)
                    seq_printf(m, " oe:%d",
                               port->icount.overrun);
                if (port->icount.cts)
                    seq_printf(m, " cts:%d",
                               port->icount.cts);
                if (port->icount.dsr)
                    seq_printf(m, " dsr:%d",
                               port->icount.dsr);
                if (port->icount.rng)
                    seq_printf(m, " rng:%d",
                               port->icount.rng);
                if (port->icount.dcd)
                    seq_printf(m, " dcd:%d",
                               port->icount.dcd);
            }
            sdio_uart_port_put(port);
            seq_putc(m, '\n');
        }
    }
    return 0;
}

static int sdio_uart_proc_open(struct inode *inode, struct file *file)
{    
    return single_open(file, sdio_uart_proc_show, NULL);
}

static const struct file_operations sdio_uart_proc_fops = {
    .owner        = THIS_MODULE,
    .open        = sdio_uart_proc_open,
    .read        = seq_read,
    .llseek        = seq_lseek,
    .release    = single_release,
};

static const struct tty_operations sdio_uart_ops = {
    .open            = sdio_uart_open,
    .close            = sdio_uart_close,
    .write            = sdio_uart_write,
    .write_room        = sdio_uart_write_room,
    .chars_in_buffer    = sdio_uart_chars_in_buffer,
    .send_xchar        = sdio_uart_send_xchar,
    .throttle        = sdio_uart_throttle,
    .unthrottle        = sdio_uart_unthrottle,
    .set_termios        = sdio_uart_set_termios,
    .break_ctl        = sdio_uart_break_ctl,
    .tiocmget        = sdio_uart_tiocmget,
    .tiocmset        = sdio_uart_tiocmset,
    .proc_fops        = &sdio_uart_proc_fops,
};

static const struct tty_port_operations serial_port_ops = {
	.carrier_raised = NULL,
	.dtr_rts = NULL,
	.activate = NULL,
	.shutdown = NULL,
};

static struct tty_driver *sdio_uart_tty_driver;


/* this routine supposes that sdio slave supporting the max packet size 
   is 200*1024
*/
void transmit_packets_to_sdio(void)
{
    int len, tmp, ret, wt_pos, rd_pos, offset, rtn_pos;
    PMUX_HEAD phead;
    unsigned char temp[8] = {0xf9,0xff,0xef,0x0,0x0, 0x0, 0x0, 0x0};
    unsigned char* buf;
    static unsigned int icount = 1;
    
    //static unsigned int len_limit = 100*1024;

    
    if (tty_data_buf.wt_pos == tty_data_buf.rd_pos)
    {
        ret = sdio_writesb(active_port->func, 0x00, temp, sizeof(temp));
        if (ret)
        {
           printk("%s: send a zero length packet fail %d\n", __FUNCTION__, ret);
        }
        //else if(icount%50 == 1)
        else
        {
            if (modem_packet_count > 100)
            {
                icount++;
            }
            if (icount%100 == 0)
            {
                printk("%s:send %d packets invalid data\n", __FUNCTION__, icount);
            }
            //printk("%s: send a zero length packet success\n", __FUNCTION__);
        }
       // icount++;
        return;
    }

    wt_pos = tty_data_buf.wt_pos;
    rd_pos = tty_data_buf.rd_pos;
    rtn_pos = tty_data_buf.rtn_pos;
    buf = g_read_buffer;
    
//    printk("%s: rd_pos=%d, wt_pos = %d\n", __FUNCTION__, rd_pos, wt_pos);
    if (rd_pos < wt_pos)
    {
        len = wt_pos - rd_pos;
        memcpy(buf, tty_data_buf.buf + rd_pos, len);
    }
    else
    {

        if (tty_data_buf.rtn_pos == 0)
        {
            printk("%s: error, rnt_pos is zero\n", __FUNCTION__);
            return;
        }
        
        offset = rtn_pos - rd_pos;
        len = offset + wt_pos;
        memcpy(buf, tty_data_buf.buf + rd_pos, offset);
        memcpy(buf + offset, tty_data_buf.buf, wt_pos);
        
    }
    
    tmp = len;
    if (tmp % 512)
    {
        tmp = (tmp/512 + 1)*512;
    }
    
    ret = huawei_sdio_writesb(active_port->func, 0x00, buf, tmp, g_sg_table.sgl);
    
    //ret = sdio_writesb(active_port->func, 0x00, buf, tmp);
    if (ret)
    {
        printk("%s: send data fail %d\n", __FUNCTION__, ret);
    }
    else
    {
        //if (icount%50 == 0)
        {
           //  printk("%s: send data %d bytes success\n", __FUNCTION__, tmp);
        }
        
        tty_data_buf.rd_pos = wt_pos;
        
    }
        
    
}

void transmit_packets_to_sdio_bak(void)
{
    int len, tmp, tmp1, ret, wt_pos, rd_pos, head_len, buf_len, offset, offset1;
    PMUX_HEAD phead;
    unsigned char temp[512] = {0xf9,0xff,0xef,0x0,0x0, 0x0, 0x0, 0x0};
    unsigned char* buf;
    static unsigned int icount = 0;
    unsigned char rollback = 0;
    static unsigned int i = 1;
    static unsigned int count = 0;
    static unsigned int len_limit = 100*1024;

    
    if (tty_data_buf.wt_pos == tty_data_buf.rd_pos)
    {
        ret = sdio_writesb(active_port->func, 0x00, temp, 512);
        if (ret)
        {
           printk("%s: send a zero length packet fail %d\n", __FUNCTION__, ret);
        }
        return;
    }

    wt_pos = tty_data_buf.wt_pos;
    rd_pos = tty_data_buf.rd_pos;
    offset = rd_pos;
    buf = g_read_buffer;
    buf_len = len_limit;//sizeof(g_read_buffer);
    head_len = sizeof(MUX_HEAD);
    phead = (PMUX_HEAD)(tty_data_buf.buf + rd_pos);

    if (rd_pos < wt_pos)
    {
        len = wt_pos - rd_pos;
        if (len > buf_len)
        {
            printk("%s: pos1, wt_pos=%d, rd_pos=%d\n", __FUNCTION__, wt_pos, rd_pos);
            tmp = 0; 
            while (tmp < buf_len)
            {
                phead = (PMUX_HEAD)(tty_data_buf.buf + offset);
                if (phead->head_tag != 0xf9 || phead->ctrl_tag != 0xef ||
                    (phead->port_index & 0x0f )< 1 ||
                    (phead->port_index & 0x0f )> 3)
                {
                    printk("%s: head error\n", __FUNCTION__);
                }
                len = phead->data_len + head_len;

                if (len%4)
                {
                    len = (len/4 + 1)*4;
                }
                offset += len;

                tmp1 = tmp;
                tmp += len;
                len = tmp1;
            }
        }
        
        memcpy(buf, tty_data_buf.buf + rd_pos, len);
    }
    else
    {
        tmp = 0; 
        len = 0;
        //printk("%s: pos2, wt_pos=%d, rd_pos=%d\n", __FUNCTION__, wt_pos, rd_pos);
        while (tmp < buf_len)
        {
            
            phead = (PMUX_HEAD)(tty_data_buf.buf + offset);

            if (phead->head_tag != 0xf9 || phead->ctrl_tag != 0xef ||
                                          (phead->port_index & 0x0f) > 3)
            {
                printk("%s: rollback ! rd_pos=%d, head_tag=0x%x, ctrl_tag=0x%x, \
                    index=%d\n", __FUNCTION__, offset, phead->head_tag, phead->ctrl_tag,
                    phead->port_index);

                offset = 0;
                rollback = 1;
                offset1 = len;
                phead = (PMUX_HEAD)tty_data_buf.buf;

                if (phead->head_tag != 0xf9 || phead->ctrl_tag != 0xef ||
                    (phead->port_index & 0x0f )< 1 ||
                    (phead->port_index & 0x0f )> 3)
                {
                    printk("%s: head error\n", __FUNCTION__);
                }
                
                if (len)
                {
                    memcpy(buf, tty_data_buf.buf + rd_pos, len);
                }
            }
            len = phead->data_len + head_len;

            if (len%4)
            {
                len = (len/4 + 1)*4;
            }
            offset += len;

            tmp1 = tmp;
            tmp += len;
            len = tmp1;
            if (offset == wt_pos || tmp == buf_len)
            {
                len = tmp;
                break;
            }
        }
        
        if (rollback)
        {
            memcpy(buf + offset1, tty_data_buf.buf, len - offset1);
        }
        else
        {
            memcpy(buf, tty_data_buf.buf + rd_pos, len);
        }
    }
    
    //icount++;

    tmp = len;
    if (tmp % 512)
    {
        tmp = (tmp/512 + 1)*512;
    }
    
    ret = huawei_sdio_writesb(active_port->func, 0x00, buf, tmp, g_sg_table.sgl);
    count += tmp;
    
    //ret = sdio_writesb(active_port->func, 0x00, buf, tmp);
    if (ret)
    {
        printk("%s: port %d send data fail %d\n", __FUNCTION__, 
            phead->port_index, ret);
    }
    else
    {
        //if (icount%50 == 0)
        {
           //  printk("%s: port %d send data %d(head len=%d, pos=%d) success\n", __FUNCTION__, 
            //    phead->port_index, tmp, sizeof(MUX_HEAD), tty_data_buf.rd_pos);
        }
        
        if (rollback)
        {
            tty_data_buf.rd_pos = len - offset1;
        }
        else
        {
            tty_data_buf.rd_pos += len;
        }
    }
        
    
}
void transmit_packet_to_sdio(void)
{
    int len, send_len, ret;
    PMUX_HEAD phead;
    unsigned char tmp[512] = {0xf9,0xff,0xef,0x0,0x0, 0x0, 0x0, 0x0};
    static unsigned int icount = 0;
    if (tty_data_buf.wt_pos == tty_data_buf.rd_pos)
    {
        if (icount%50 == 1)
        {
            //printk("%s: before send zero length packet\n", __FUNCTION__);
        }
        ret = sdio_writesb(active_port->func, 0x00, tmp, 512);
        if (ret)
        {
            printk("%s: send a zero length packet fail %d\n", __FUNCTION__, ret);
        }
        else if(icount%50 == 1)
        {
            //printk("%s: send a zero length packet success\n", __FUNCTION__);
        }
        icount++;
        return;
    }

    phead = (PMUX_HEAD)(tty_data_buf.buf + tty_data_buf.rd_pos);
    if (phead->head_tag != 0xf9 || phead->ctrl_tag != 0xef ||
        (phead->port_index & 0x0f) > 3)
    {
        printk("%s:rd_pos=%d, head_tag=0x%x, ctrl_tag=0x%x, index=%d\n", __FUNCTION__,
            tty_data_buf.rd_pos, phead->head_tag, phead->ctrl_tag, phead->port_index);
        phead = (PMUX_HEAD)tty_data_buf.buf;
        tty_data_buf.rd_pos = 0;
    }
    len = phead->data_len + sizeof(MUX_HEAD);

    if (len%4)
    {
        len = (len/4 + 1)*4;
    }
    
    memcpy(g_read_buffer, (void*)phead, len);
    send_len = len;
    if (send_len%512)
    {
        send_len = (send_len/512 + 1)*512;
    }
    if (icount%50 == 1)
    {
        //printk("%s: before send data packet\n", __FUNCTION__);
    }
    icount++;
    ret = sdio_writesb(active_port->func, 0x00, g_read_buffer, send_len);
    if (ret)
    {
        printk("%s: port %d send data fail %d\n", __FUNCTION__, 
            phead->port_index, ret);
    }
    else
    {
        if (icount%50 == 1)
        {
             //printk("%s: port %d send data %d(head len=%d, pos=%d) success\n", __FUNCTION__, 
              //  phead->port_index, send_len, sizeof(MUX_HEAD), tty_data_buf.rd_pos);
        }
        tty_data_buf.rd_pos += len;
    }
        
    
}


void sdio_data_trans_thread(void* data)
{
    FUNC_ENTER

    int len, tmp, ret, wt_pos, rd_pos, offset, rtn_pos;
    unsigned int i= 1,send_len = 5*1024;
    unsigned char* buf;
    struct sdio_uart_port* port;
    //char tmp[100]={0};
    bExit = 0;
    
    port = sdio_uart_port_get(0);
    mdelay(1000);
    mdelay(1000);
    mdelay(1000);
    use_tans_thread = 1;
    while(true)
    {
        if (bExit)
        {
            data_trans_thread_exit = 1;
            break;
        }
        if (i == 1)
        {
            //printk("%s: begin to transmit first packet\n", __FUNCTION__);
        }
        
        #if 0
        ret = huawei_sdio_writesb(port->func, 0x00, g_read_buffer, send_len, g_sg_table.sgl);
        if (ret)
        {
            printk("%s: transmit data fail\n", __FUNCTION__);
        }
        else
        {
            if (i%100 == 0)
            {
                printk("%s: transmit %d*%d bytes data success\n", __FUNCTION__, i, send_len);
            }
            i++;
            if (i%1000 == 0)
            {
                send_len += 5*1024;
                if (send_len > 250*1024)
                {
                    send_len = 50*1024;
                }
            }
        }
        #endif
        
        /*BEGIN:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/
        //wait_event(thread_wait_q, (tty_data_buf.wt_pos != tty_data_buf.rd_pos));
        //wait_event_timeout(thread_wait_q, (tty_data_buf.wt_pos != tty_data_buf.rd_pos), msecs_to_jiffies(1));
        
        if (tty_data_buf.wt_pos == tty_data_buf.rd_pos)
        {
            wait_event_timeout(thread_wait_q, (tty_data_buf.wt_pos != tty_data_buf.rd_pos), 1);
        }
        else
        /*END:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/
        {
            wt_pos = tty_data_buf.wt_pos;
            rd_pos = tty_data_buf.rd_pos;
            rtn_pos = tty_data_buf.rtn_pos;
            buf = tty_data_buf.buf + rd_pos;
            
            //printk("%s: rd_pos=%d, wt_pos = %d\n", __FUNCTION__, rd_pos, wt_pos);
            #if 0
            if (rd_pos < wt_pos)
            {
                len = wt_pos - rd_pos;
                memcpy(buf, tty_data_buf.buf + rd_pos, len);
            }
            else
            {

                if (tty_data_buf.rtn_pos == 0)
                {
                    printk("%s: error, rnt_pos is zero\n", __FUNCTION__);
                    data_trans_thread_exit = 1;
                    return;
                }
                
                offset = rtn_pos - rd_pos;
                len = offset + wt_pos;
                if (offset)
                {
                    memcpy(buf, tty_data_buf.buf + rd_pos, offset);
                }
                memcpy(buf + offset, tty_data_buf.buf, wt_pos);
                
            }
            #else
            //printk("%s, buf=%p, position=%d\n", __FUNCTION__, buf, (buf -tty_data_buf.buf));
            if (rd_pos < wt_pos)
            {
                //printk("%s, pos1\n", __FUNCTION__);
                len = wt_pos - rd_pos;
                rtn_pos = rd_pos;       
              //  memcpy(buf, tty_data_buf.buf + rd_pos, len);
            }
            else
            {
                if (tty_data_buf.rtn_pos == 0)
                {
                    printk("%s: error, rnt_pos is zero\n", __FUNCTION__);
                    data_trans_thread_exit = 1;
                    return;
                }
                if (rtn_pos == rd_pos)
                {
                    //printk("%s, pos2\n", __FUNCTION__);
                    len = wt_pos;
                    /*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.12*/
                    buf = tty_data_buf.buf;
                    /*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.12*/
                }
                else
                {
                    //printk("%s, pos3\n", __FUNCTION__);
                    len = rtn_pos - rd_pos;
                }
            }
            
            #endif
            //printk("%s: data len=%d\n", __FUNCTION__, len);
            tmp = len;
            /*BEGIN:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.12*/
            memcpy(sdio_send_buff, buf, tmp);
            if (tmp % 512)
            {
                /*avoid the mistake of wrong head*/
                *(sdio_send_buff+tmp) = 0xff;
                tmp = (tmp/512 + 1)*512;
            }
            /*END:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.12*/

            /*
            for(i = 0; i < 20; i++)
            {
                printk("%s: buf[%d]=0x%x\n", __FUNCTION__, i, buf[i]);
            }
            */
/*BEGIN:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/ 
            /*UPGRADE TEST 2014.04.12*/
            /*printk("%s: CMD53_WR_Len=%d\n", __FUNCTION__, tmp);
            for(i = 0;i < 32; i++)
            {
                if(i%16 == 0)
                {
                    printk("\n");
                }
                printk("%x ", *(buf+i));
            }
            printk("\n");
            */
            /*UPGRADE TEST 2014.04.12*/
            
            //do_gettimeofday(&time3);
            sdio_claim_host(port->func);
            ret = huawei_sdio_writesb(active_port->func, 0x00, sdio_send_buff, tmp, g_sg_table.sgl);
            sdio_release_host(port->func);
            //do_gettimeofday(&time4);

            /*BEGIN:DTS2014042209407  n00203193 added for SD Modem 2014.04.12*/
            //*(buf+len) = 0xf9;
            /*END:DTS2014042209407  n00203193 added for SD Modem 2014.04.12*/

//printk("%s:\n time1=%u:%u\n time2=%u:%u\n time3=%u:%u\n time4=%u:%u\n", __FUNCTION__, time1.tv_sec, time1.tv_usec, time2.tv_sec, time2.tv_usec, time3.tv_sec, time3.tv_usec, time4.tv_sec, time4.tv_usec);
/*END:DTS2014042209407  n00203193 MODIFIED for SD Modem 2014.04.15*/ 
            //ret = sdio_writesb(active_port->func, 0x00, buf, tmp);
            if (ret)
            {
                printk("%s: send data fail %d\n", __FUNCTION__, ret);
                data_trans_thread_exit = 1;
                break;
            }
            else
            {
               // printk("%s: send data success\n", __FUNCTION__);
                if (rtn_pos == rd_pos)
                {
                    tty_data_buf.rd_pos = wt_pos;
                }
                else
                {
                    tty_data_buf.rd_pos = 0;
                }
                //tty_data_buf.rd_pos = wt_pos;
            }
        }
        
    }
    //printk("%s: exit %d\n", __FUNCTION__, data_trans_thread_exit);
}

#if 0

static unsigned int g_reg_addr = 0;
static unsigned char g_reg_val = 0;
static unsigned int g_reg_val_long = 0;
static ssize_t sdio_uart_get_g_reg_addr(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%x\n", g_reg_addr);
}

static ssize_t sdio_uart_set_g_reg_addr(struct device *d,
		 struct device_attribute *attr, const char *buf, size_t count)
{
	g_reg_addr = simple_strtol(buf, NULL, 16);
	printk("g_reg_addr = 0x%lX\n", g_reg_addr);

	return count;
}

static ssize_t sdio_uart_get_g_reg_val(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%x\n", g_reg_val);
}

static ssize_t sdio_uart_set_g_reg_val(struct device *d,
		 struct device_attribute *attr, const char *buf, size_t count)
{
	g_reg_val = simple_strtol(buf, NULL, 16);
	printk("g_reg_addr = 0x%lX\n", g_reg_val);

	return count;
}
static ssize_t sdio_uart_get_g_reg_valll(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%x\n", g_reg_val_long);
}

static ssize_t sdio_uart_set_g_reg_valll(struct device *d,
		 struct device_attribute *attr, const char *buf, size_t count)
{
	g_reg_val_long = simple_strtol(buf, NULL, 16);
	printk("g_reg_addr = 0x%lX\n", g_reg_val_long);

	return count;
}

static ssize_t sdio_uart_read_dummy(struct device *d,
		 struct device_attribute *attr, const char *buf)
{
	sdio_memcpy_fromio(active_port->func,  &g_reg_val_long, g_reg_addr, 4);
	printk("sdio_uart_writeb_dummy: address is %x, value is %x.\n", g_reg_addr, g_reg_val_long);
	return sprintf(buf, "%x\n", g_reg_val);
	
	
}
static ssize_t sdio_uart_write_dummy(struct device *d,
		 struct device_attribute *attr, const char *buf, size_t count)
{
	printk("sdio_uart_writeb_dummy: address is %x, value is %x.\n", g_reg_addr, g_reg_val_long);
	sdio_memcpy_toio(active_port->func, g_reg_addr, &g_reg_val_long, 4);
	return count;
}
static ssize_t sdio_uart_readb_dummy(struct device *d,
		 struct device_attribute *attr, const char *buf)
{
	g_reg_val = sdio_readb(active_port->func, g_reg_addr, NULL);
	printk("sdio_uart_writeb_dummy: address is %x, value is %x.\n", g_reg_addr, g_reg_val);
	return sprintf(buf, "%x\n", g_reg_val);
	
	
}
static ssize_t sdio_uart_writeb_dummy(struct device *d,
		 struct device_attribute *attr, const char *buf, size_t count)
{
	printk("sdio_uart_writeb_dummy: address is %x, value is %x.\n", g_reg_addr, g_reg_val);
	sdio_writeb(active_port->func, (unsigned char)g_reg_val, g_reg_addr, NULL);
	return count;
}
static DEVICE_ATTR(to_g_reg_addr, 0666, sdio_uart_get_g_reg_addr, sdio_uart_set_g_reg_addr);
static DEVICE_ATTR(to_g_reg_val, 0666, sdio_uart_get_g_reg_val, sdio_uart_set_g_reg_val);
static DEVICE_ATTR(to_read_write, 0666, sdio_uart_read_dummy, sdio_uart_write_dummy);
static DEVICE_ATTR(to_read_writeb, 0666, sdio_uart_readb_dummy, sdio_uart_writeb_dummy);
static DEVICE_ATTR(to_g_reg_val_long, 0666, sdio_uart_get_g_reg_valll, sdio_uart_set_g_reg_valll);


#endif



static int sdio_uart_probe(struct sdio_func *func,
               const struct sdio_device_id *id)
{
    struct sdio_uart_port *port;
    PMUX_HEAD phead;
    int ret, i;
    unsigned int max_segs;
    struct device *dev;
FUNC_ENTER


   
#if 0    
    if (func->class == SDIO_CLASS_UART||func->class == SDIO_CLASS_GPS) {

        /*
         * We need tuple 0x91.  It contains SUBTPL_SIOREG
         * and SUBTPL_RCVCAPS.
         */
        struct sdio_func_tuple *tpl;
        for (tpl = func->tuples; tpl; tpl = tpl->next) {
            if (tpl->code != 0x91)
                continue;
            if (tpl->size < 10)
                continue;
            if (tpl->data[1] == 0)  /* SUBTPL_SIOREG */
                break;
        }
        if (!tpl) {
            printk(KERN_WARNING
                   "%s: can't find tuple 0x91 subtuple 0 (SUBTPL_SIOREG) for GPS class\n",
                   sdio_func_id(func));
            kfree(port);
            return -EINVAL;
        }
        printk(KERN_DEBUG "%s: Register ID = 0x%02x, Exp ID = 0x%02x\n",
               sdio_func_id(func), tpl->data[2], tpl->data[3]);
        port->regs_offset = (tpl->data[4] << 0) |
                    (tpl->data[5] << 8) |
                    (tpl->data[6] << 16);
        printk(KERN_DEBUG "%s: regs offset = 0x%x\n",
               sdio_func_id(func), port->regs_offset);
        port->uartclk = tpl->data[7] * 115200;
        if (port->uartclk == 0)
            port->uartclk = 115200;
        printk(KERN_DEBUG "%s: clk %d baudcode %u 4800-div %u\n",
               sdio_func_id(func), port->uartclk,
               tpl->data[7], tpl->data[8] | (tpl->data[9] << 8));
    } else {
        HW_DEBUG("ql_sdio_uart: find class 0x%x device\n", func->class);
        kfree(port);
        return -EINVAL;
    }


    #endif

    printk("%s: vendor=0x%x, device=0x%x, class=%d\n", __FUNCTION__, func->vendor,
    func->device, func->class);
    
    tty_data_buf.size = sizeof(modem_uart_data_buf);
    tty_data_buf.wt_pos = 0;
    tty_data_buf.rd_pos = 0;
    tty_data_buf.rtn_pos = 0;
    tty_data_buf.buf = (unsigned char*)modem_uart_data_buf;
    memset(&mux_head, 0, sizeof(mux_head));
    
    for (i = 0; i < PORT_COUNT; i++)
    {
        port = kzalloc(sizeof(struct sdio_uart_port), GFP_KERNEL);
        if (!port)
        {
            printk("%s: alloc memory fail\n", __FUNCTION__);
            return -ENOMEM;
        }
        //tty_port_init(&port->port);
        //port->port.ops = &serial_port_ops;
        port->regs_offset = 0;
        port->func = func;
        sdio_set_drvdata(func, port);

        ret = sdio_uart_add_port(port);
        if (ret)
        {
            kfree(port);
        }
        else
        {
            
            dev = tty_register_device(sdio_uart_tty_driver, port->index + 1, &func->dev);
            printk("%s:probe port(%p)_index = %x\n",__FUNCTION__, port, port->index);
            //HW_DBG("lkf-%s :  port->index[%d]\n", __func__, port->index);
            if (IS_ERR(dev))
             {
                printk("%s: register tty device SDIO%d fail\n", __FUNCTION__, port->index+1);
                sdio_uart_port_remove(port);
                ret = PTR_ERR(dev);
            }
            //port->buf_info = &uart_data_buf[i];
            port->modem_sig_read_done = 0;
        }
        
        phead = &mux_head[i];
        phead->ctrl_tag = 0xef;
        phead->head_tag = 0xf9;
        phead->port_index = i+1;


        #if 0
        device_create_file(dev, &dev_attr_to_g_reg_addr);
		device_create_file(dev, &dev_attr_to_g_reg_val);
		device_create_file(dev, &dev_attr_to_read_write);
		device_create_file(dev, &dev_attr_to_read_writeb);
		device_create_file(dev, &dev_attr_to_g_reg_val_long);
        #endif
        
    }

    active_port = port;
    //INIT_WORK(&sdio_write_init,sdio_write_work);
    sdio_func_startup(port);

    ///max_segs = func->card->host->max_segs;
    ret = sg_alloc_table(&g_sg_table, 256, GFP_KERNEL);
    if (ret)
    {
        printk("%s: malloc scatterlist fail\n", __FUNCTION__);
    }
    /*added for test*/
   // schedule_work(&sdio_write_init);
   init_waitqueue_head(&thread_wait_q);
   init_waitqueue_head(&remove_wait_q);
   kthread_run(sdio_data_trans_thread, NULL, "sdio_data_trans_thread%d", 1);
   modem_packet_count = 0;
FUNC_EXIT
    return ret;
}


static void sdio_uart_remove_huawei(struct sdio_func *func)
{
    struct sdio_uart_port *port  = sdio_uart_table[0];
    int i;

    dump_stack();
    //return;
    
    bExit = 1;
    printk("%s:pos0\n", __FUNCTION__);
    while(data_trans_thread_exit == 0 && use_tans_thread)
    {
        wait_event_timeout(remove_wait_q, 0, 1);
        printk("%s: exit=%d\n", __FUNCTION__, data_trans_thread_exit);
    }
    printk("%s:pos1\n", __FUNCTION__);

    for (i = 0; i < PORT_COUNT; i++)
    {
        port = sdio_uart_table[i];
        tty_unregister_device(sdio_uart_tty_driver, port->index + 1);
        sdio_uart_port_remove(port);
        printk("%s:pos2\n", __FUNCTION__);
        if (!port->opened)
        {
            printk("%s:pos3\n", __FUNCTION__);
            kfree(port);
        }
    }
    sg_free_table(&g_sg_table);

}


static const struct sdio_device_id sdio_uart_ids[] = {
    { SDIO_DEVICE_CLASS(SDIO_CLASS_UART)        },
    { SDIO_DEVICE_CLASS(SDIO_CLASS_GPS)        },
    {.class = SDIO_ANY_ID, .vendor = 0x12d1, .device = SDIO_ANY_ID},
    { /* end: all zeroes */                },
};

MODULE_DEVICE_TABLE(sdio, sdio_uart_ids);

static struct sdio_driver sdio_uart_driver = {
    .probe        = sdio_uart_probe,
    .remove        = sdio_uart_remove_huawei,
    .name        = "sdio_uart",
    .id_table    = sdio_uart_ids,
};

static int __init sdio_uart_init(void)
{
    int ret;
    struct tty_driver *tty_drv;
        FUNC_ENTER
    sdio_uart_tty_driver = tty_drv = alloc_tty_driver(UART_NR);
    if (!tty_drv)
        return -ENOMEM;
    tty_drv->owner = THIS_MODULE;
    tty_drv->driver_name = "sdio_uart";
    tty_drv->name =   "ttySDIO";
    tty_drv->major = 0;  /* dynamically allocated */
    tty_drv->minor_start = 0;
    tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
    tty_drv->subtype = SERIAL_TYPE_NORMAL;
    tty_drv->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
    tty_drv->init_termios = tty_std_termios;
    tty_drv->init_termios.c_cflag = B4800 | CS8 | CREAD | HUPCL | CLOCAL;
    tty_drv->init_termios.c_ispeed = 4800;
    tty_drv->init_termios.c_ospeed = 4800;
    tty_set_operations(tty_drv, &sdio_uart_ops);

    ret = tty_register_driver(tty_drv);
    if (ret)
        goto err1;

    ret = sdio_register_driver(&sdio_uart_driver);
    if (ret)
        goto err2;
        FUNC_EXIT
    return 0;

err2:
    tty_unregister_driver(tty_drv);
err1:
    put_tty_driver(tty_drv);
    return ret;
}

static void __exit sdio_uart_exit(void)
{
    printk("%s:enter\n", __FUNCTION__);
    sdio_unregister_driver(&sdio_uart_driver);
    tty_unregister_driver(sdio_uart_tty_driver);
    put_tty_driver(sdio_uart_tty_driver);
}

module_init(sdio_uart_init);
module_exit(sdio_uart_exit);

MODULE_AUTHOR("Nicolas Pitre");
MODULE_LICENSE("GPL");
/*END: DTS2014032201528 n00203913 2014-03-22 ADDED for SD Modem*/
