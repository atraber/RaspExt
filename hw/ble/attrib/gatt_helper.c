/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2010  Nokia Corporation
 *  Copyright (C) 2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <bluetooth/uuid.h>
#include "att.h"
#include "hw/ble/btio/btio.h"
#include "gattrib.h"
#include "gatt.h"
#include "gatt_helper.h"


GIOChannel *gatt_connect(const char *src, const char *dst,
            const char *dst_type, const char *sec_level,
            int psm, int mtu, BtIOConnect connect_cb,
            gpointer user_data,
            GError **gerr)
{
    GIOChannel *chan;
    bdaddr_t sba, dba;
    uint8_t dest_type;
    GError *tmp_err = NULL;
    BtIOSecLevel sec;

    str2ba(dst, &dba);

    /* Local adapter */
    if (src != NULL) {
        if (!strncmp(src, "hci", 3))
            hci_devba(atoi(src + 3), &sba);
        else
            str2ba(src, &sba);
    } else
        bacpy(&sba, BDADDR_ANY);

    /* Not used for BR/EDR */
    if (strcmp(dst_type, "random") == 0)
        dest_type = BDADDR_LE_RANDOM;
    else
        dest_type = BDADDR_LE_PUBLIC;

    if (strcmp(sec_level, "medium") == 0)
        sec = BT_IO_SEC_MEDIUM;
    else if (strcmp(sec_level, "high") == 0)
        sec = BT_IO_SEC_HIGH;
    else
        sec = BT_IO_SEC_LOW;

    if (psm == 0)
        chan = bt_io_connect(connect_cb, user_data, NULL, &tmp_err,
                BT_IO_OPT_SOURCE_BDADDR, &sba,
                BT_IO_OPT_DEST_BDADDR, &dba,
                BT_IO_OPT_DEST_TYPE, dest_type,
                BT_IO_OPT_CID, ATT_CID,
                BT_IO_OPT_SEC_LEVEL, sec,
                BT_IO_OPT_INVALID);
    else
        chan = bt_io_connect(connect_cb, user_data, NULL, &tmp_err,
                BT_IO_OPT_SOURCE_BDADDR, &sba,
                BT_IO_OPT_DEST_BDADDR, &dba,
                BT_IO_OPT_PSM, psm,
                BT_IO_OPT_IMTU, mtu,
                BT_IO_OPT_SEC_LEVEL, sec,
                BT_IO_OPT_INVALID);

    if (tmp_err) {
        g_propagate_error(gerr, tmp_err);
        return NULL;
    }

    return chan;
}
