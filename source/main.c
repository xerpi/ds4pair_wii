#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <bte/bte.h>
#include <bte/bd_addr.h>

#define VID 0x054C
#define PID 0x05C4

int run = 1, bd_addr_read = 0;
int heap_id = -1;
void *xfb = NULL;
GXRModeObj *rmode = NULL;
void init_video();
void print_bd_addr(struct bd_addr *bdaddr);
void print_mac(uint8_t *mac);
int bte_read_bdaddr_cb(s32 result, void *userdata);
struct bd_addr bdaddr;

void find_and_set_mac();
int get_bdaddrs(int fd, unsigned char *paired_mac, unsigned char *ds4_mac);
int set_paired_mac(int fd, unsigned char *mac);
int get_link_key(int fd, unsigned char *link_key);

int main(int argc, char **argv)
{
	init_video();
	WPAD_Init();
	
	heap_id = iosCreateHeap(4096);
	
	printf("ds4pair Wii version by xerpi\n");
	printf("Connect a DS4 controller to the USB and press A.  Press HOME to exit.\n\n");
	
	while(run) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);
		if (pressed & WPAD_BUTTON_A) {
			find_and_set_mac();
		}
		
		if (pressed & WPAD_BUTTON_HOME) run = 0;
		VIDEO_WaitVSync();
	}
	exit(0);
	return 0;
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
			
			unsigned char paired[6] = {0}, ds4_mac[6] = {0};
			get_bdaddrs(fd, paired, ds4_mac);
			
			printf("Controller's bluetooth MAC address: ");
			print_mac(ds4_mac);
			printf("\n");
			printf("\nCurrent controller paired address: ");
			print_mac(paired);
			
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
