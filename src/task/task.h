#pragma once
#include <Arduino.h>

extern TaskHandle_t EncoderTaskHandle;
extern TaskHandle_t BoardSensorTaskHandle;
extern TaskHandle_t UITaskHandle;
extern TaskHandle_t EngineTaskHandle;

//initialization for RTOS
void initTasks();

// Task Declarations
void EncoderTask(void *pvParameters);
void BoardSensorTask(void *pvParameters);
void UITask(void *pvParameters);
void EngineTask(void *pvParameters);