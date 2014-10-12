#ifndef DS4WIIBT_CONFIG_H
#define DS4WIIBT_CONFIG_H

#include <stdio.h>

#define DS4WIIBT_CONFIG_FILE "/config/ds4wiibt_config"

#define MAC_ADDR_SIZE 6
struct ds4wiibt_config_hdr {
	unsigned int n_devices;
	unsigned int list_offset;
};
struct ds4wiibt_config_ctx {
	FILE *fd;
	struct ds4wiibt_config_hdr hdr;
	unsigned char **MAC_list;
};

void ds4wiibt_config_initialize(struct ds4wiibt_config_ctx *ctx);
int ds4wiibt_config_read(const char *filename, struct ds4wiibt_config_ctx *ctx);
void ds4wiibt_config_add(struct ds4wiibt_config_ctx *ctx, unsigned char *newMAC);
int ds4wiibt_config_write(const char *filename, struct ds4wiibt_config_ctx *ctx);
int ds4wiibt_config_MAC_exists(struct ds4wiibt_config_ctx *ctx, unsigned char *MAC);
void ds4wiibt_config_free(struct ds4wiibt_config_ctx *ctx);


#endif
