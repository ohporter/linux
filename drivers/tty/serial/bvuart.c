/*
 * BeagleBone 6502 RemoteProc virtual UART driver
 *
 * Copyright (C) 2012 Matt Porter (matt@ohporter.com)
 * Based on Tiny Serial
 * Copyright (C) 2002-2004 Greg Kroah-Hartman (greg@kroah.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, version 2 of the License.
 *
 * This driver shows how to create a minimal serial driver.  It does not rely on
 * any backing hardware, but creates a timer that emulates data being received
 * from some kind of hardware.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/module.h>
#include <linux/io.h>


#define DRIVER_AUTHOR "Matt Porter <matt@ohporter.com>"
#define DRIVER_DESC "Bone 6502 Virtual UART driver"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

#define DELAY_TIME		HZ/64	/* 64 characters a second */

#define BVUART_SERIAL_MAJOR	240	/* experimental range */
#define BVUART_SERIAL_MINORS	1	/* only have one minor */
#define UART_NR			1	/* only use one port */

#define BVUART_SERIAL_NAME	"bvuart"

#define MY_NAME			BVUART_SERIAL_NAME

#define BVUART_TX_RDY	0x40
#define BVUART_TX_VLD	0x20
#define BVUART_RX_RDY	0x40
#define BVUART_RX_VLD	0x20

struct b6502_vuart {
	u8	tx;
	u8	txstat;
	u8	rx;
	u8	rxstat;
};

static struct timer_list *timer;
struct b6502_vuart *bvuart;

static void bvuart_stop_tx(struct uart_port *port)
{
}

static void bvuart_stop_rx(struct uart_port *port)
{
}

static void bvuart_enable_ms(struct uart_port *port)
{
}

static void bvuart_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	int count;

	if (port->x_char) {
		if (bvuart->txstat != BVUART_TX_RDY)
			return;
		bvuart->tx = port->x_char;
		bvuart->txstat = BVUART_TX_VLD;
		port->icount.tx++;
		port->x_char = 0;
		return;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		bvuart_stop_tx(port);
		return;
	}

	count = port->fifosize >> 1;
	do {
		if (bvuart->txstat != BVUART_TX_RDY)
			return;
		bvuart->tx = xmit->buf[xmit->tail];
		bvuart->txstat = BVUART_TX_VLD;
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		bvuart_stop_tx(port);
}

static void bvuart_start_tx(struct uart_port *port)
{
}

static void bvuart_timer(unsigned long data)
{
	struct uart_port *port;
	struct tty_struct *tty;


	port = (struct uart_port *)data;
	if (!port)
		return;
	if (!port->state)
		return;
	tty = port->state->port.tty;
	if (!tty)
		return;

	if (bvuart->rxstat == BVUART_RX_VLD) {
		/* add one character to the tty port */
		/* this doesn't actually push the data through unless tty->low_latency is set */
		tty_insert_flip_char(tty, bvuart->rx, 0);

		tty_flip_buffer_push(tty);
		bvuart->rxstat = BVUART_RX_RDY;
	}

	/* resubmit the timer again */
	mod_timer(timer, jiffies + DELAY_TIME);

	/* see if we have any data to transmit */
	bvuart_tx_chars(port);
}

static unsigned int bvuart_tx_empty(struct uart_port *port)
{
	return 0;
}

static unsigned int bvuart_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void bvuart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static void bvuart_break_ctl(struct uart_port *port, int break_state)
{
}

static void bvuart_set_termios(struct uart_port *port,
			     struct ktermios *new, struct ktermios *old)
{
	int baud, quot, cflag = new->c_cflag;
	/* get the byte size */
	switch (cflag & CSIZE) {
	case CS5:
		printk(KERN_DEBUG " - data bits = 5\n");
		break;
	case CS6:
		printk(KERN_DEBUG " - data bits = 6\n");
		break;
	case CS7:
		printk(KERN_DEBUG " - data bits = 7\n");
		break;
	default: // CS8
		printk(KERN_DEBUG " - data bits = 8\n");
		break;
	}

	/* determine the parity */
	if (cflag & PARENB)
		if (cflag & PARODD)
			pr_debug(" - parity = odd\n");
		else
			pr_debug(" - parity = even\n");
	else
		pr_debug(" - parity = none\n");

	/* figure out the stop bits requested */
	if (cflag & CSTOPB)
		pr_debug(" - stop bits = 2\n");
	else
		pr_debug(" - stop bits = 1\n");

	/* figure out the flow control settings */
	if (cflag & CRTSCTS)
		pr_debug(" - RTS/CTS is enabled\n");
	else
		pr_debug(" - RTS/CTS is disabled\n");

	/* Set baud rate */
        baud = uart_get_baud_rate(port, new, old, 0, 115200);
        quot = uart_get_divisor(port, baud);
}

static int bvuart_startup(struct uart_port *port)
{
	/* this is the first time this port is opened */
	/* do any hardware initialization needed here */

	/* Initialize the BVUART to indicate ready to receive */
	bvuart->rxstat = BVUART_RX_RDY;

	/* create our timer and submit it */
	if (!timer) {
		timer = kmalloc(sizeof(*timer), GFP_KERNEL);
		if (!timer)
			return -ENOMEM;
	}
	init_timer(timer);
	timer->data = (unsigned long)port;
	timer->expires = jiffies + DELAY_TIME;
	timer->function = bvuart_timer;
	add_timer(timer);

	return 0;
}

static void bvuart_shutdown(struct uart_port *port)
{
	/* The port is being closed by the last user. */
	/* Do any hardware specific stuff here */

	/* shut down our timer */
	del_timer(timer);
}

static const char *bvuart_type(struct uart_port *port)
{
	return "bvuart";
}

static void bvuart_release_port(struct uart_port *port)
{

}

static int bvuart_request_port(struct uart_port *port)
{
	return 0;
}

static void bvuart_config_port(struct uart_port *port, int flags)
{
}

static int bvuart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	return 0;
}

static struct uart_ops bvuart_ops = {
	.tx_empty	= bvuart_tx_empty,
	.set_mctrl	= bvuart_set_mctrl,
	.get_mctrl	= bvuart_get_mctrl,
	.stop_tx	= bvuart_stop_tx,
	.start_tx	= bvuart_start_tx,
	.stop_rx	= bvuart_stop_rx,
	.enable_ms	= bvuart_enable_ms,
	.break_ctl	= bvuart_break_ctl,
	.startup	= bvuart_startup,
	.shutdown	= bvuart_shutdown,
	.set_termios	= bvuart_set_termios,
	.type		= bvuart_type,
	.release_port	= bvuart_release_port,
	.request_port	= bvuart_request_port,
	.config_port	= bvuart_config_port,
	.verify_port	= bvuart_verify_port,
};

static struct uart_port bvuart_port = {
	.ops		= &bvuart_ops,
	.type		= PORT_BVUART,
};

static struct uart_driver bvuart_reg = {
	.owner		= THIS_MODULE,
	.driver_name	= BVUART_SERIAL_NAME,
	.dev_name	= BVUART_SERIAL_NAME,
	.major		= BVUART_SERIAL_MAJOR,
	.minor		= BVUART_SERIAL_MINORS,
	.nr		= UART_NR,
};

static int __init bvuart_init(void)
{
	int result;

	printk(KERN_INFO "bvuart serial driver loaded\n");

	result = uart_register_driver(&bvuart_reg);
	if (result)
		return result;

	result = uart_add_one_port(&bvuart_reg, &bvuart_port);
	if (result)
		uart_unregister_driver(&bvuart_reg);

	/* HACK ALERT */
	bvuart = ioremap(0x4a311ff0, 4);

	return result;
}

module_init(bvuart_init);
