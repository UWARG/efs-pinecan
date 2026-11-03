#include "main.h"
#include "can.h"
#include "dma.h"
#include "tim.h"
#include "can_manager.hpp"
#include "lighting_controller.hpp"

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

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void System_Clock_Config(void);
/* USER CODE BEGIN PFP */

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

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	/* USER CODE END Init */

	/* Configure the system clock */
	System_Clock_Config();

	/* USER CODE BEGIN SysInit */
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	Mx_Dma_Init();
	Mx_Can1_Init();
	Mx_Tim2_Init();

	/* USER CODE BEGIN 2 */

	if (Init_Can() != PINECAN_OK)
	{
		Error_Handler();
	}

	// initialise the LED PWM/DMA output and set GND as default
	Led_Init();

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	uint64_t raw_vehicle_state = 0;
	bool new_data = false;
	uint8_t flight_state = 0;

	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		while (!new_data)
		{
		
			static uint32_t last_led_tick = 0;
			static uint32_t tick = 0;
			if (HAL_GetTick() - last_led_tick >= 20)
			{
				last_led_tick = HAL_GetTick();
				Generate_Leds(tick); // advance animations, expand active zones into colour_buffer
				Push_Leds();		 // bit-encode colour_buffer into the DMA-streamed bank buffer
				tick++;
			}

			// Service PineCAN housekeeping (1ms tick-gated pinecan1ms call)
			CAN_Manager::Service();

			// Re-read the cache each inner iteration so new CAN data is visible
			raw_vehicle_state = CAN_Manager::Get_Latest_Vehicle_State();

			// Step 2: check if data changed since last call
			new_data = CAN_Manager::Vehicle_State_Changed(raw_vehicle_state);
		}

		// read cache and decode — gives us the current flight state
		flight_state = Interpret_Vehicle_State(raw_vehicle_state);

		Select_Pattern(flight_state);

		new_data = false;
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void System_Clock_Config(void)
{
	RCC_OscInitTypeDef rcc_osc_init_struct = {0};
	RCC_ClkInitTypeDef rcc_clk_init_struct = {0};

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	rcc_osc_init_struct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	rcc_osc_init_struct.MSIState = RCC_MSI_ON;
	rcc_osc_init_struct.MSICalibrationValue = 0;
	rcc_osc_init_struct.MSIClockRange = RCC_MSIRANGE_11;
	rcc_osc_init_struct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&rcc_osc_init_struct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	rcc_clk_init_struct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	rcc_clk_init_struct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	rcc_clk_init_struct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	rcc_clk_init_struct.APB1CLKDivider = RCC_HCLK_DIV1;
	rcc_clk_init_struct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&rcc_clk_init_struct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}
/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
