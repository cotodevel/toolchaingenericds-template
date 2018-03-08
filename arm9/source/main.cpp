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
//C++ part
#include <iostream>
using namespace std;
#include <iostream>
#include <list>
#include <vector>

#include "socket.h"
#include "in.h"
#include <netdb.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "main.h"
#include "InterruptsARMCores_h.h"
#include "specific_shared.h"
#include "ff.h"
#include "memoryHandleTGDS.h"
#include "reent.h"
#include "sys/types.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "consoleTGDS.h"
#include "utilsTGDS.h"

#include "devoptab_devices.h"
#include "fsfatlayerTGDS.h"
#include "usrsettingsTGDS.h"

#include "videoTGDS.h"
#include "keypadTGDS.h"
#include "guiTGDS.h"

int main(int _argc, sint8 **_argv) {
	
	IRQInit();
	
	bool project_specific_console = false;	//set default console or custom console: default console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	printf("Dldi Name: %s",dldi_tryingInterface());
	
	while (1)
	{
		if (keysPressed() & KEY_A){
			printf("mylist should contain:1 10 20 30 30 20 2 3 4 5");
			std::list<int> mylist;
			std::list<int>::iterator it;
			// set some initial values:
			for (int i=1; i<=5; ++i) mylist.push_back(i); // 1 2 3 4 5

			it = mylist.begin();
			++it;       // it points now to number 2           ^

			mylist.insert (it,10);                        // 1 10 2 3 4 5

			// "it" still points to number 2                      ^
			mylist.insert (it,2,20);                      // 1 10 20 20 2 3 4 5

			--it;       // it points now to the second 20            ^

			std::vector<int> myvector (2,30);
			mylist.insert (it,myvector.begin(),myvector.end());
													// 1 10 20 30 30 20 2 3 4 5
													//               ^
			for (it=mylist.begin(); it!=mylist.end(); ++it){
				printf("%d-",*it);
			}
			
			while(keysPressed() & KEY_A){}
		}
		
		if (keysPressed() & KEY_B){
			GUI_clear();
		}
		
		IRQVBlankWait();
	}

}