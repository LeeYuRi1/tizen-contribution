#include <peripheral_io.h>

#include "log.h"

//r,g,b 색상의 빛을 연속으로 빛나게 하는 코드 
int resource_write_color_led(int pin_num[], int value)
{
	for (int i = 0; i < 3; i++) {
		switch (i) {
		case 0:
			resource_write_led(pin_num[0],value);
			delay(2000);
			break;
		case 1:
			resource_write_led(pin_num[1],value);
			delay(2000);
			break;
		case 2:
			resource_write_led(pin_num[2],value);
			delay(2000);
			break;
		default:
			break;
		}
	}
	return 0;
}

