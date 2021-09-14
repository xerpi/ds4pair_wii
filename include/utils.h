#ifndef UTILS_H
#define UTILS_H

#define VID 0x054C
#define PID 0x05C4

#include <gccore.h>
#include <bte/bte.h>

extern int run;

void init_stuff();
void init_video();
void print_bd_addr(struct bd_addr *bdaddr);
void print_mac(uint8_t *mac);
int bte_read_bdaddr_cb(s32 result, void *userdata);

void find_and_set_mac();
int config_add_mac(const uint8_t *mac);
int get_bdaddrs(int fd, unsigned char *paired_mac, unsigned char *ds4_mac);
int set_paired_mac(int fd, unsigned char *mac);
int get_link_key(int fd, unsigned char *link_key);


#endif
