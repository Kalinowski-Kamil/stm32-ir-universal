#include "main.h"
#include <stdio.h>

#include "ir_config.h"
#include "ir_capture.h"
#include "ir_decode.h"
#ifdef IR_ENABLE_SELF_TESTS
#include "ir_tests.h"
#endif

/* Peripherals */
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

/* Retarget printf to UART2 */
int __io_putchar(int ch){
  if(ch=='\n') __io_putchar('\r');
  HAL_UART_Transmit(&huart2,(uint8_t*)&ch,1,HAL_MAX_DELAY);
  return 1;
}

/* Prototypy init */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();

#ifdef IR_ENABLE_SELF_TESTS
  /* Tryb testów on-target — funkcja nie wraca */
  IR_Tests_RunUART();
#else
  /* Tryb pracy z TSOP */
  IR_Capture_AttachTimer(&htim2, TIM_CHANNEL_1);
  IR_Capture_Start();
  printf("IR ready.\r\n");

  uint32_t segs[IR_MAX_SEGS]; uint8_t n=0;
  for(;;){
    if(IR_Capture_TakeFrame(segs,&n)){
      IRDecodeOut r = ir_detect_and_decode(segs,n);
      printf("IR %s repeat=%s bits=%u ",
             ir_proto_name(r.proto), r.is_repeat?"yes":"no", r.bits);
      if(!r.is_repeat){
        if(r.bits<=32) printf("val=0x%08lX\r\n",(unsigned long)r.value);
        else           printf("val48=0x%012llX\r\n",(unsigned long long)r.value);
      } else {
        printf("\r\n");
      }
    }
  }
#endif
  /* nigdy tu nie trafimy */
  return 0;
}

/* ==== ZEGARY: 80 MHz (MSI->PLL) ==== */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_6; // 4 MHz
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM            = 1;
  RCC_OscInitStruct.PLL.PLLN            = 40;
  RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) Error_Handler();

  HAL_RCCEx_EnableMSIPLLMode();
}

/* ==== TIM2: 1 MHz CNT + Input Capture CH1 oba zbocza, filtr 8 ==== */
static void MX_TIM2_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_IC_InitTypeDef     sConfigIC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler         = 79;             // 80 MHz / (79+1) = 1 MHz
  htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim2.Init.Period            = 0xFFFFFFFF;
  htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) Error_Handler();

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) Error_Handler();

  if (HAL_TIM_IC_Init(&htim2) != HAL_OK) Error_Handler();

  sConfigIC.ICPolarity  = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter    = 8;                    // tłumienie szumu
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) Error_Handler();

  /* Upewnij się, że NVIC dla TIM2 jest aktywny (Cube zwykle ustawia w startupie):
     HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
     HAL_NVIC_EnableIRQ(TIM2_IRQn);
  */
}

/* ==== UART2: 115200 8N1 (PA2 TX, PA3 RX) ==== */
static void MX_USART2_UART_Init(void)
{
  __HAL_RCC_USART2_CLK_ENABLE();

  huart2.Instance        = USART2;
  huart2.Init.BaudRate   = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits   = UART_STOPBITS_1;
  huart2.Init.Parity     = UART_PARITY_NONE;
  huart2.Init.Mode       = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

/* ==== GPIO: LED PB3 oraz AF dla PA0/PA2/PA3 ==== */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef gi = {0};

  /* LED PB3 */
  gi.Pin   = GPIO_PIN_3;
  gi.Mode  = GPIO_MODE_OUTPUT_PP;
  gi.Pull  = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &gi);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

  /* PA0 TIM2_CH1 */
  gi.Pin       = GPIO_PIN_0;
  gi.Mode      = GPIO_MODE_AF_PP;
  gi.Pull      = GPIO_PULLUP;           // TSOP idle wysoki – pull-up pomaga
  gi.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gi.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &gi);

  /* PA2 USART2_TX, PA3 USART2_RX */
  gi.Pin       = GPIO_PIN_2|GPIO_PIN_3;
  gi.Mode      = GPIO_MODE_AF_PP;
  gi.Pull      = GPIO_PULLUP;
  gi.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gi.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &gi);
}

/* ==== ISR przekierowanie do modułu capture ==== */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
#ifndef IR_ENABLE_SELF_TESTS
  /* W trybie testów capture nie jest używany, ale callback może istnieć. */
  IR_Capture_ISR(htim);
#else
  (void)htim;
#endif
}

void Error_Handler(void)
{
  __disable_irq();
  for(;;){
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
    HAL_Delay(200);
  }
}
