/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Oracle Météo - CPU Edition (Avec Vraie Calibration & I2C Fixé)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_x-cube-ai.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "hts221_reg.h"
#include "lps22hh_reg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CACHEAXI_HandleTypeDef hcacheaxi;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
volatile float current_temp = 0.0f;
volatile float current_hum = 0.0f;
volatile float current_press = 0.0f;

stmdev_ctx_t dev_ctx_hts, dev_ctx_lps;
float t0_degC, t1_degC, h0_rh, h1_rh, t0_out, t1_out, h0_t0_out, h1_t0_out;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
static void MX_CACHEAXI_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void SystemIsolation_Config(void);

/* USER CODE BEGIN PFP */
/* Redirection printf pour la console Mac */
int _write(int f, char *p, int l) {
    HAL_UART_Transmit(&huart1, (uint8_t*)p, l, 100);
    return l;
}

/* Fonctions I2C HTS221 (Température/Humidité - Adresse 0xBE) */
static int32_t platform_write_hts(void *h, uint8_t r, uint8_t *b, uint16_t l) {
    HAL_I2C_Mem_Write(h, 0xBE, r | 0x80, I2C_MEMADD_SIZE_8BIT, b, l, 1000);
    return 0;
}
static int32_t platform_read_hts(void *h, uint8_t r, uint8_t *b, uint16_t l) {
    HAL_I2C_Mem_Read(h, 0xBE, r | 0x80, I2C_MEMADD_SIZE_8BIT, b, l, 1000);
    return 0;
}

/* Fonctions I2C LPS22HH (Pression - Adresse 0xBA) */
static int32_t platform_write_lps(void *h, uint8_t r, uint8_t *b, uint16_t l) {
    HAL_I2C_Mem_Write(h, 0xBA, r, I2C_MEMADD_SIZE_8BIT, b, l, 1000);
    return 0;
}
static int32_t platform_read_lps(void *h, uint8_t r, uint8_t *b, uint16_t l) {
    HAL_I2C_Mem_Read(h, 0xBA, r, I2C_MEMADD_SIZE_8BIT, b, l, 1000);
    return 0;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CACHEAXI_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();

  /* Ordre sécurisé : Isolation AVANT le démarrage de l'IA */
  SystemIsolation_Config();
  MX_X_CUBE_AI_Init();

  /* USER CODE BEGIN 2 */
  printf("\r\n=== STATION ORACLE METEO DEMARREE (MODE CPU) ===\r\n");

  /* Initialisation Capteurs avec les fonctions séparées */
  dev_ctx_hts.write_reg = platform_write_hts;
  dev_ctx_hts.read_reg  = platform_read_hts;
  dev_ctx_hts.handle    = &hi2c1;

  dev_ctx_lps.write_reg = platform_write_lps;
  dev_ctx_lps.read_reg  = platform_read_lps;
  dev_ctx_lps.handle    = &hi2c1;

  hts221_power_on_set(&dev_ctx_hts, PROPERTY_ENABLE);
  hts221_data_rate_set(&dev_ctx_hts, HTS221_ODR_1Hz);
  lps22hh_data_rate_set(&dev_ctx_lps, LPS22HH_10_Hz);

  /* Lecture de la calibration usine du HTS221 */
  hts221_temp_deg_point_0_get(&dev_ctx_hts, &t0_degC);
  hts221_temp_deg_point_1_get(&dev_ctx_hts, &t1_degC);
  hts221_temp_adc_point_0_get(&dev_ctx_hts, &t0_out);
  hts221_temp_adc_point_1_get(&dev_ctx_hts, &t1_out);
  hts221_hum_rh_point_0_get(&dev_ctx_hts, &h0_rh);
  hts221_hum_rh_point_1_get(&dev_ctx_hts, &h1_rh);
  hts221_hum_adc_point_0_get(&dev_ctx_hts, &h0_t0_out);
  hts221_hum_adc_point_1_get(&dev_ctx_hts, &h1_t0_out);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* 1. Acquisition I2C */
      int16_t raw_h = 0, raw_t = 0;
      uint32_t raw_p = 0;

      hts221_temperature_raw_get(&dev_ctx_hts, &raw_t);
      hts221_humidity_raw_get(&dev_ctx_hts, &raw_h);
      lps22hh_pressure_raw_get(&dev_ctx_lps, &raw_p);

      /* Vraie conversion au lieu de raw / 10.0f */
      current_temp = ((t1_degC - t0_degC) * (float)(raw_t - t0_out)) / (t1_out - t0_out) + t0_degC;
      current_hum = ((h1_rh - h0_rh) * (float)(raw_h - h0_t0_out)) / (h1_t0_out - h0_t0_out) + h0_rh;
      current_press = lps22hh_from_lsb_to_hpa(raw_p);

      printf("\r\n[LIVE] T: %.1f C | H: %.1f %% | P: %.1f hPa\r\n", current_temp, current_hum, current_press);

  /* USER CODE END WHILE */

  /* 2. Lancement Inférence IA (CPU) */
  MX_X_CUBE_AI_Process();

  /* USER CODE BEGIN 3 */
      HAL_Delay(1000); /* Pause visuelle de 1 seconde */
  }
  /* USER CODE END 3 */
}

/* ================================================================== */
/* FONCTIONS PERIPHERIQUES (Générées par CubeMX)                      */
/* ================================================================== */

static void MX_CACHEAXI_Init(void)
{
  hcacheaxi.Instance = CACHEAXI;
  if (HAL_CACHEAXI_Init(&hcacheaxi) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x30C0EDFF;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void SystemIsolation_Config(void)
{
  __HAL_RCC_RIFSC_CLK_ENABLE();
  RIMC_MasterConfig_t RIMC_master = {0};
  RIMC_master.MasterCID = RIF_CID_1;
  RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV;
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_ETH1, &RIMC_master);

  HAL_GPIO_ConfigPinAttributes(GPIOC,GPIO_PIN_1,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOE,GPIO_PIN_5,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOE,GPIO_PIN_6,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOH,GPIO_PIN_9,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
