
/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/
#include "common_shared.h"
#include "specific_shared.h"
#include "interrupts.h"

#ifdef ARM7
#include <string.h>

#include "main.h"
#include "wifi_arm7.h"

#endif

#ifdef ARM9

#include <stdbool.h>

#include "main.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "InterruptsARMCores_h.h"
#include "wifi_arm9.h"

#endif


//inherits what is defined in: common_shared.c
#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HandleFifoNotEmptyWeakRef(uint32 cmd1,uint32 cmd2,uint32 cmd3,uint32 cmd4){
	
	switch (cmd1) {		
		//Shared 
		case(WIFI_SYNC):{
			Wifi_Sync();
		}
		break;
		
		//Process the packages (signal) that sent earlier FIFO_SEND_EXT
		case(FIFO_SOFTFIFO_READ_EXT):{
		
		}
		break;
		
		case(FIFO_SOFTFIFO_WRITE_EXT):{
			SetSoftFIFO(cmd2);
		}
		break;
		
		
		//ARM7 command handler
		#ifdef ARM7
		
		//ARM7 Only
		case(FIFO_POWERCNT_ON):{
			powerON((uint16)cmd2);
		}
		break;
		
		case (FIFO_POWERMGMT_WRITE):{
			PowerManagementDeviceWrite(PM_SOUND_AMP, (int)cmd2>>16);  // void * data == command2
		}
		break;
		
		//arm9 wants to send a WIFI context block address / userdata is always zero here
		case(WIFI_STARTUP):{
			//	wifiAddressHandler( void * address, void * userdata )
			wifiAddressHandler((Wifi_MainStruct *)(uint32)cmd2, 0);
		}
		break;
		
		#endif
		
		
		
		//ARM9 command handler
		#ifdef ARM9
		//exception handler: arm7
		case(EXCEPTION_ARM7):{
			
			if((uint32)cmd2 == (uint32)unexpectedsysexit_7){
				exception_handler((uint32)unexpectedsysexit_7);	//r0 = EXCEPTION_ARM7 / r1 = unexpectedsysexit_7
			}
		}
		break;
		
		
		#endif
	
		
	}
	
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2,uint32 cmd3,uint32 cmd4){
}
