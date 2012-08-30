#ifndef __LINUX_TI_ADC_H
#define __LINUX_TI_ADC_H

/**
 * struct adc_data	ADC Input information
 * @adc_channels:	Number of analog inputs
 *			available for ADC.
 */

struct adc_data {
	int adc_channels;
};

#endif
