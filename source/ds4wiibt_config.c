#include <stdlib.h>
#include <string.h>
#include "ds4wiibt_config.h"

void ds4wiibt_config_initialize(struct ds4wiibt_config_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
}

int ds4wiibt_config_read(const char *filename, struct ds4wiibt_config_ctx *ctx)
{
	ctx->fd = fopen(filename, "rb");
	if (ctx->fd == NULL) return 0;
	fread(&ctx->hdr, 1, sizeof(ctx->hdr), ctx->fd);
	fseek(ctx->fd, ctx->hdr.list_offset,  SEEK_SET);
	ctx->MAC_list = malloc(ctx->hdr.n_devices * sizeof(unsigned char *));
	int i;
	for (i = 0; i < ctx->hdr.n_devices; i++) {
		ctx->MAC_list[i] = malloc(MAC_ADDR_SIZE);
		fread(ctx->MAC_list[i], 1, MAC_ADDR_SIZE, ctx->fd);
	}
	fclose(ctx->fd);
	return 1;
}

void ds4wiibt_config_add(struct ds4wiibt_config_ctx *ctx, unsigned char *newMAC)
{
	ctx->MAC_list = realloc(ctx->MAC_list, (ctx->hdr.n_devices+1) * sizeof(unsigned char *));
	ctx->MAC_list[ctx->hdr.n_devices] = malloc(MAC_ADDR_SIZE);
	memcpy(ctx->MAC_list[ctx->hdr.n_devices], newMAC, MAC_ADDR_SIZE);
	ctx->hdr.n_devices++;
}

int ds4wiibt_config_write(const char *filename, struct ds4wiibt_config_ctx *ctx)
{
	FILE *wfd = fopen(filename, "wb");
	if (wfd < 0) return 0;
	ctx->hdr.list_offset = sizeof(ctx->hdr);
	fwrite(&ctx->hdr, 1, sizeof(ctx->hdr), wfd);
	int i;
	for (i = 0; i < ctx->hdr.n_devices; i++) {
		fwrite(ctx->MAC_list[i], 1, MAC_ADDR_SIZE, wfd);
	}
	fclose(wfd);
	return 1;
}

int ds4wiibt_config_MAC_exists(struct ds4wiibt_config_ctx *ctx, unsigned char *MAC)
{
	int i;
	for (i = 0; i < ctx->hdr.n_devices; i++) {
		if (memcmp(ctx->MAC_list[i], MAC, MAC_ADDR_SIZE) == 0)
			return 1;
	}
	return 0;
}

void ds4wiibt_config_free(struct ds4wiibt_config_ctx *ctx)
{
	if (ctx->MAC_list) {
		int i;
		for (i = 0; i < ctx->hdr.n_devices; i++) {
			free(ctx->MAC_list[i]);
		}
		free(ctx->MAC_list);
	}
}
