#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <bte/bte.h>
#include <bte/bd_addr.h>
#include <fat.h>
#include "utils.h"

int main(int argc, char **argv)
{
	init_stuff();
	
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
	
	return 0;
}
