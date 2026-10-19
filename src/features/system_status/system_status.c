#include <zephyr/sys/printk.h>
#include "system_status.h"
void system_status_announce(void)
{
    printk("Hybrid Mesh PoC: Phase 1 fresh firmware booted\n");
}
