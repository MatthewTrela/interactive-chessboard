#pragma once
#include <Arduino.h>

extern TaskHandle_t GameLoopTaskHandle;
extern TaskHandle_t UITaskHandle;
extern TaskHandle_t EngineTaskHandle;

//initialization for RTOS
void initTasks();

// Task Declarations
void GameLoopTask(void *pvParameters);
void UITask(void *pvParameters);
void EngineTask(void *pvParameters);