
#include "hw/ble/btio/btio.h"

#ifdef __cplusplus
extern "C" {
#endif

GIOChannel *gatt_connect(const char *src, const char *dst,
			const char *dst_type, const char *sec_level,
			int psm, int mtu, BtIOConnect connect_cb,
            gpointer user_data,
            GError **gerr);

#ifdef __cplusplus
}
#endif
