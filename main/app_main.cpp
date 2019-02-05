
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sys/uio.h"

#include "CGenius.h"


void cgTask(void *pvParameters)
{
    char *argv[]={"/sd/data/Commander Keen/base1/keen.exe", NULL};
    CG_main(1, argv);
}

extern "C" void app_main(void)
//void app_main(void)
{
	xTaskCreatePinnedToCore(&cgTask, "cgTask", 42000, NULL, 2, NULL, 0);
}
