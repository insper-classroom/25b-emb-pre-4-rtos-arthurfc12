/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t     xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

static void btn_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        int id = (gpio == BTN_PIN_R) ? 1 : (gpio == BTN_PIN_Y) ? 2 : 0;
        if (id) xQueueSendFromISR(xQueueBtn, &id, NULL);
    }
}

void btn_task(void *p) {
    int id = 0;
    for (;;) {
        if (xQueueReceive(xQueueBtn, &id, portMAX_DELAY) == pdTRUE) {
            if (id == 1) xSemaphoreGive(xSemaphoreLedR);
            if (id == 2) xSemaphoreGive(xSemaphoreLedY);
        }
    }
}

static void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_put(LED_PIN_R, 0);

    int blink = 0;
    for (;;) {
        if (!blink) {
            if (xSemaphoreTake(xSemaphoreLedR, portMAX_DELAY) == pdTRUE)
                blink = 1;
        } else {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            if (xSemaphoreTake(xSemaphoreLedR, 0) == pdTRUE) { blink = 0; gpio_put(LED_PIN_R, 0); continue; }
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            if (xSemaphoreTake(xSemaphoreLedR, 0) == pdTRUE) { blink = 0; gpio_put(LED_PIN_R, 0); }
        }
    }
}

static void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    gpio_put(LED_PIN_Y, 0);

    int blink = 0;
    for (;;) {
        if (!blink) {
            if (xSemaphoreTake(xSemaphoreLedY, portMAX_DELAY) == pdTRUE)
                blink = 1;
        } else {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            if (xSemaphoreTake(xSemaphoreLedY, 0) == pdTRUE) { blink = 0; gpio_put(LED_PIN_Y, 0); continue; }
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            if (xSemaphoreTake(xSemaphoreLedY, 0) == pdTRUE) { blink = 0; gpio_put(LED_PIN_Y, 0); }
        }
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT); gpio_put(LED_PIN_R, 0);
    gpio_init(LED_PIN_Y); gpio_set_dir(LED_PIN_Y, GPIO_OUT); gpio_put(LED_PIN_Y, 0);

    gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_Y); gpio_set_dir(BTN_PIN_Y, GPIO_IN); gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    xQueueBtn      = xQueueCreate(8, sizeof(int));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    xTaskCreate(btn_task,  "BTN_Task",  256, NULL, 2, NULL);
    xTaskCreate(led_r_task, "LED_R",    256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Y",    256, NULL, 1, NULL);

    vTaskStartScheduler();
    while(1){}
    return 0;
}