/*
 * BQ76925.h
 *
 *  Created on: May 30, 2026
 *      Author: danie
 */

#ifndef INC_BQ76925_H_
#define INC_BQ76925_H_

#define VREF1500 1500
//#define VREF3000 3000

#ifdef VREF1500
#define VREF_SELECTION 0
#define BQ_VREF 1500
#endif
#ifdef VREF3000
#define VREF_SELECTION 1
#define BQ_VREF 3000
#endif

typedef struct {
	int8_t offset_corr;
	int8_t gain_corr;
} ADC_calibration_values;

typedef enum {
	VSS,
	CELL,
	VREF05,
	VREF085
} vcout_sel;

typedef enum {
	C1,
	C2,
	C3,
	C4,
	C5,
	C6,
	TEMP
} cells;

uint8_t test_read_reg(I2C_HandleTypeDef *hi2c1);
int init_BQ76925(I2C_HandleTypeDef *hi2c1, ADC_calibration_values cal_vals[]);
int vcout(I2C_HandleTypeDef *hi2c1, vcout_sel vcout_sel, cells cell_sel);


#endif /* INC_BQ76925_H_ */
