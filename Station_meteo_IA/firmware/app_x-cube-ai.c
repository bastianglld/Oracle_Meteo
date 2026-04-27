/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body - Oracle 5 Jours (CPU)
  ******************************************************************************
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#if defined ( __ICCARM__ )
#define AI_AXISRAM2   _Pragma("location=\"AI_AXISRAM2\"")
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#define AI_AXISRAM2   __attribute__((section(".AI_AXISRAM2")))
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"
#include "network.h"
#include "network_data.h"

/* IO buffers ----------------------------------------------------------------*/
AI_ALIGNED(4) ai_i8 data_in_1[AI_NETWORK_IN_1_SIZE_BYTES];
ai_i8* data_ins[AI_NETWORK_IN_NUM] = { data_in_1 };

AI_ALIGNED(4) ai_i8 data_out_1[AI_NETWORK_OUT_1_SIZE_BYTES];
ai_i8* data_outs[AI_NETWORK_OUT_NUM] = { data_out_1 };

/* Activations buffers -------------------------------------------------------*/
ai_handle data_activations0[] = {(ai_handle) 0x34100000};

/* AI objects ----------------------------------------------------------------*/
static ai_handle network = AI_HANDLE_NULL;
static ai_buffer* ai_input;
static ai_buffer* ai_output;

static void ai_log_err(const ai_error err, const char *fct)
{
  if (fct)
    printf("TEMPLATE - Error (%s) - type=0x%02x code=0x%02x\r\n", fct, err.type, err.code);
  else
    printf("TEMPLATE - Error - type=0x%02x code=0x%02x\r\n", err.type, err.code);
  do {} while (1);
}

static int ai_boostrap(ai_handle *act_addr)
{
  ai_error err;
  err = ai_network_create_and_init(&network, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) {
    ai_log_err(err, "ai_network_create_and_init");
    return -1;
  }
  ai_input = ai_network_inputs_get(network, NULL);
  ai_output = ai_network_outputs_get(network, NULL);
  for (int idx=0; idx < AI_NETWORK_IN_NUM; idx++) {
	  ai_input[idx].data = data_ins[idx];
  }
  for (int idx=0; idx < AI_NETWORK_OUT_NUM; idx++) {
	  ai_output[idx].data = data_outs[idx];
  }
  return 0;
}

static int ai_run(void)
{
  ai_i32 batch;
  batch = ai_network_run(network, ai_input, ai_output);
  if (batch != 1) {
    ai_log_err(ai_network_get_error(network), "ai_network_run");
    return -1;
  }
  return 0;
}

/* USER CODE BEGIN 2 */
/* Récupération des valeurs lues par l'I2C dans main.c */
extern volatile float current_temp;
extern volatile float current_hum;
extern volatile float current_press;

#define SEQ_LENGTH 24
#define NUM_FEATURES 3

/* Constantes de normalisation issues de ton entraînement Python */
#define MIN_TEMP -11.20f
#define MAX_TEMP 38.00f
#define MIN_HUM  15.00f
#define MAX_HUM  100.00f
#define MIN_PRES 976.20f
#define MAX_PRES 1043.40f

/* Paramètres de quantification TFLite (Int8) */
#define IN_SCALE  0.00387543f
#define IN_ZP    -128
#define OUT_SCALE 0.00390625f
#define OUT_ZP   -128

/* Buffer historique pour stocker les 24 dernières heures */
static float history_buffer[SEQ_LENGTH * NUM_FEATURES] = {0};
static int buffer_initialized = 0;

int acquire_and_process_data(ai_i8* data[]) {
  ai_i8 *in_buffer = data[0];

  /* 1. Initialisation au premier démarrage */
  if (!buffer_initialized) {
      for (int i = 0; i < SEQ_LENGTH; i++) {
          history_buffer[i * NUM_FEATURES + 0] = current_temp;
          history_buffer[i * NUM_FEATURES + 1] = current_hum;
          history_buffer[i * NUM_FEATURES + 2] = current_press;
      }
      buffer_initialized = 1;
  }

  /* 2. Décalage de la fenêtre (sliding window) */
  memmove(history_buffer, &history_buffer[NUM_FEATURES], (SEQ_LENGTH - 1) * NUM_FEATURES * sizeof(float));

  /* 3. Ajout des nouvelles mesures */
  history_buffer[(SEQ_LENGTH - 1) * NUM_FEATURES + 0] = current_temp;
  history_buffer[(SEQ_LENGTH - 1) * NUM_FEATURES + 1] = current_hum;
  history_buffer[(SEQ_LENGTH - 1) * NUM_FEATURES + 2] = current_press;

  /* 4. Normalisation et Quantification en Int8 */
  for (int i = 0; i < SEQ_LENGTH; i++) {
      float t_n = (history_buffer[i * NUM_FEATURES + 0] - MIN_TEMP) / (MAX_TEMP - MIN_TEMP);
      float h_n = (history_buffer[i * NUM_FEATURES + 1] - MIN_HUM) / (MAX_HUM - MIN_HUM);
      float p_n = (history_buffer[i * NUM_FEATURES + 2] - MIN_PRES) / (MAX_PRES - MIN_PRES);

      if(t_n < 0.0f) t_n = 0.0f; else if(t_n > 1.0f) t_n = 1.0f;
      if(h_n < 0.0f) h_n = 0.0f; else if(h_n > 1.0f) h_n = 1.0f;
      if(p_n < 0.0f) p_n = 0.0f; else if(p_n > 1.0f) p_n = 1.0f;

      in_buffer[i * NUM_FEATURES + 0] = (ai_i8)((t_n / IN_SCALE) + IN_ZP);
      in_buffer[i * NUM_FEATURES + 1] = (ai_i8)((h_n / IN_SCALE) + IN_ZP);
      in_buffer[i * NUM_FEATURES + 2] = (ai_i8)((p_n / IN_SCALE) + IN_ZP);
  }
  return 0;
}

int post_process(ai_i8* data[]) {
  ai_i8 *out = data[0];
  printf("\r\n========= ORACLE METEO (PREVISIONS 5j) =========\r\n");

  for(int j = 0; j < 5; j++) {
      int b = j * 4; // Index : 0=T, 1=H, 2=P, 3=Pluie

      float t_n = (float)(out[b+0] - OUT_ZP) * OUT_SCALE;
      float h_n = (float)(out[b+1] - OUT_ZP) * OUT_SCALE;
      float p_n = (float)(out[b+2] - OUT_ZP) * OUT_SCALE;
      float r_n = (float)(out[b+3] - OUT_ZP) * OUT_SCALE;

      float t_f = t_n * (MAX_TEMP - MIN_TEMP) + MIN_TEMP;
      float h_f = h_n * (MAX_HUM - MIN_HUM) + MIN_HUM;
      float p_f = p_n * (MAX_PRES - MIN_PRES) + MIN_PRES;
      float risk = r_n * 100.0f;

      printf(" J+%d | T: %4.1fC | H: %3.0f%% | P: %4.0f hPa | Pluie: %3.0f%%\r\n",
              j+1, t_f, h_f, p_f, (risk > 100.0f ? 100.0f : risk));
  }
  printf("================================================\r\n");
  return 0;
}
/* USER CODE END 2 */

/* Entry points --------------------------------------------------------------*/

void MX_X_CUBE_AI_Init(void)
{
    __HAL_RCC_AXISRAM2_MEM_CLK_ENABLE();
    __HAL_RCC_AXISRAM3_MEM_CLK_ENABLE();
    __HAL_RCC_AXISRAM4_MEM_CLK_ENABLE();
    __HAL_RCC_AXISRAM5_MEM_CLK_ENABLE();
    __HAL_RCC_AXISRAM6_MEM_CLK_ENABLE();
    RAMCFG_SRAM2_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    RAMCFG_SRAM3_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    RAMCFG_SRAM4_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    RAMCFG_SRAM5_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    RAMCFG_SRAM6_AXI->CR &= ~RAMCFG_CR_SRAMSD;

    /* USER CODE BEGIN 5 */
    ai_boostrap(data_activations0);
    /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
    /* USER CODE BEGIN 6 */
  int res = -1;

  if (network) {
      /* Le programme n'est plus bloqué ici, on passe sur le CPU 1 seule fois */
      res = acquire_and_process_data(data_ins);
      if (res == 0)
        res = ai_run();
      if (res == 0)
        res = post_process(data_outs);
  }

  if (res != 0) {
    ai_error err = {AI_ERROR_INVALID_STATE, AI_ERROR_CODE_NETWORK};
    ai_log_err(err, "Process has FAILED");
  }
    /* USER CODE END 6 */
}
#ifdef __cplusplus
}
#endif
