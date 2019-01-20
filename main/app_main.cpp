
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
    char *argv[]={"", NULL};
    CG_main(1, argv);
}

extern "C" void app_main(void)
//void app_main(void)
{
	xTaskCreatePinnedToCore(&cgTask, "cgTask", 12000, NULL, 5, NULL, 0);
}
