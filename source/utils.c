#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fat.h>
#include <wiiuse/wpad.h>
#include "utils.h"

/*
config header:
	offset  |  size  |  value
	   0    |   4    |  number of paired devices
	   4    |   6    |  BT MAC of the 1st device
	  4+6*i |   6    |  BT MAC of the i-th device
*/
static const char conf_file[] ATTRIBUTE_ALIGN(32) = "/shared2/sys/ds4wiibt.dat";

int run = 1;
static int bd_addr_read = 0;
static int heap_id = -1;
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static void button_cb()
{
	run = 0;
}

void init_stuff()
{
	SYS_SetResetCallback(button_cb);
	SYS_SetPowerCallback(button_cb);
	fatInitDefault();
	init_video();
	WPAD_Init();
	heap_id = iosCreateHeap(4096);
}

void find_and_set_mac()
{
	bd_addr_read = 0;
	BTE_ReadBdAddr(&bdaddr, bte_read_bdaddr_cb);
	
	//Wait until we get the callback...
	while (!bd_addr_read) usleep(1000);
		
	//Find the DS4 controller
	usb_device_entry dev_entry[8];
	unsigned char dev_count;
	if (USB_GetDeviceList(dev_entry, 8, USB_CLASS_HID, &dev_count) < 0) {
		printf("Error getting USB device list.\n"); return;
	}
		
	int i;
	usb_device_entry *de;
	for (i = 0; i < dev_count; ++i) {
		de = &dev_entry[i];
		
		if( de->vid == VID && de->pid == PID) {
			printf("DS4 found.\n");
			
			int fd;
			if (USB_OpenDevice(de->device_id, VID, PID, &fd) < 0) {
				printf("Could not open the DS4.\n"); return;
			}
		
			printf("Wii local BD MAC: ");
			print_mac(bdaddr.addr);
			printf("\n");
			
			unsigned char paired[6] = {0},
				ds4_mac[6] ATTRIBUTE_ALIGN(32) = {0};
			get_bdaddrs(fd, paired, ds4_mac);
			printf("Controller's bluetooth MAC address: ");
			print_mac(ds4_mac);
			printf("\n");
			printf("\nCurrent controller paired address: ");
			print_mac(paired);
			printf("\n");
			
			//Write the MAC to the NAND
			if (config_add_mac(ds4_mac)) {
				printf("DS4 MAC added to the config file!\n");
			}

			struct bd_addr bd2;
			memcpy(bd2.addr, paired, sizeof(uint8_t) * 6);
			if (bd_addr_cmp(&bdaddr, &bd2)) {
				printf("\n\nAddress is already set! Press HOME to exit.\n");
				return;
			}		
			
			printf("\nSetting the pair address...");
			uint8_t *mac2 = bdaddr.addr;
			set_paired_mac(fd, mac2);
			u8 mac[6] = {0};
			get_bdaddrs(fd, mac, NULL);
			printf("\nController's pair address set to: ");
			print_mac(mac);
			
			memcpy(bd2.addr, mac2, sizeof(uint8_t) * 6);
			if (bd_addr_cmp(&bdaddr, &bd2)) {
				printf("\n\nAddress set correctly! Press HOME to exit.\n");	  
			} else {
				printf("\n\nPair MAC Address could not be set correctly.\n");	
			}
			
			USB_CloseDevice(&fd);
			return;
		}
	}
	printf("No controller found on USB busses.\n");
}

//Deletes the config file and adds this mac
int config_add_mac(const uint8_t *mac)
{
	ISFS_Initialize();
	int fd = ISFS_Open(conf_file, ISFS_OPEN_RW);
	if (fd >= 0) {
		ISFS_Close(fd);
		ISFS_Delete(conf_file);
	}
	printf("Creating config file... ");
	int ret = ISFS_CreateFile(conf_file, 0, ISFS_OPEN_RW, ISFS_OPEN_RW, ISFS_OPEN_RW);
	if (ret < 0) {
		printf("Error creating \"%s\" : %d\n", conf_file, ret);
		return -1;
	}
	printf("done!\n");
	fd = ISFS_Open(conf_file, ISFS_OPEN_RW);
	ISFS_Seek(fd, 0, SEEK_SET);
	int n = ISFS_Write(fd, mac, 6);
	ISFS_Close(fd);
	ISFS_Deinitialize();
	return (n == 6);
}

int set_paired_mac(int fd, unsigned char *mac)
{
	u8 ATTRIBUTE_ALIGN(32) buf[] = {
		0x13,
		mac[5], mac[4], mac[3], mac[2], mac[1], mac[0],
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	return USB_WriteCtrlMsg(fd,
		USB_REQTYPE_INTERFACE_SET,
		USB_REQ_SETCONFIG,
		(USB_REPTYPE_FEATURE<<8) | 0x13,
		0,
		sizeof(buf),
		buf);
}

int get_link_key(int fd, unsigned char *link_key)
{
	u8 ATTRIBUTE_ALIGN(32) buf[0x10];
	int ret = USB_ReadCtrlMsg(fd,
		USB_REQTYPE_INTERFACE_GET,
		USB_REQ_CLEARFEATURE,
		(USB_REPTYPE_FEATURE<<8) | 0x13,
		0,
		sizeof(buf),
		buf);
		
	int i;
	for(i = 0; i < ret; i++) {
		link_key[i] = buf[i];
	}
	return ret;
}

int get_bdaddrs(int fd, unsigned char *paired_mac, unsigned char *ds4_mac)
{
	u8 ATTRIBUTE_ALIGN(32) buf[0x10];
	int ret = USB_ReadCtrlMsg(fd,
		USB_REQTYPE_INTERFACE_GET,
		USB_REQ_CLEARFEATURE,
		(USB_REPTYPE_FEATURE<<8) | 0x12,
		0,
		sizeof(buf),
		buf);
	
	if (paired_mac) {
		paired_mac[0] = buf[15];
		paired_mac[1] = buf[14];
		paired_mac[2] = buf[13];
		paired_mac[3] = buf[12];
		paired_mac[4] = buf[11];
		paired_mac[5] = buf[10];
	}
	if (ds4_mac) {
		ds4_mac[0] = buf[6];
		ds4_mac[1] = buf[5];
		ds4_mac[2] = buf[4];
		ds4_mac[3] = buf[3];
		ds4_mac[4] = buf[2];
		ds4_mac[5] = buf[1];
	}

	return ret;
}


int bte_read_bdaddr_cb(s32 result, void *userdata)
{
	bd_addr_read = 1;
	return 1;
}

void print_mac(uint8_t *mac)
{
	printf("%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void init_video()
{
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	printf("\x1b[1;0H");
}
