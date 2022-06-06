

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>

#include <kernel.h>
#include <sys_clock.h>
#include <devicetree.h>
#include <sys/time_units.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <string.h>
#include <drivers/pwm.h>

#define PWM_LED0_NODE DT_ALIAS(pwm_led0) // led1

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)

#define PWM_CTLR DT_PWMS_CTLR(PWM_LED0_NODE)
#define PWM_CHANNEL DT_PWMS_CHANNEL(PWM_LED0_NODE)
#define PWM_FLAGS DT_PWMS_FLAGS(PWM_LED0_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_CTLR DT_INVALID_NODE
#define PWM_CHANNEL 0
#define PWM_FLAGS 0
#endif

#define PERIOD_USEC 20000U
#define NUM_STEPS 50U
#define STEP_USEC (PERIOD_USEC / NUM_STEPS)
#define SLEEP_MSEC 25U
#define MY_STACK_SIZE 1000
#define GPIO_MOTOR_LEFT_1 27
#define GPIO_MOTOR_LEFT_2 26
#define GPIO_MOTOR_RIGHT_1 3
#define GPIO_MOTOR_RIGHT_2 4
#define IR_LEFT 28
#define IR_RIGHT 29
#define GPIO_OUT_PIN 31
#define GPIO_INT_PIN 30
#define Set 0
#define Unset 1
#define PRIORITY 5

#define PORT "GPIO_0"
static int status_Left = 0;
static int status_Right = 0;
static uint32_t cm;
void Motor_Task(void)
{
	const struct device *pwm;
	uint32_t pulse_width = 10U; //(pwm_led0.period / 2); //50%

	const struct device *dev;
	dev = device_get_binding(PORT);
	int ret1 = gpio_pin_configure(dev, GPIO_MOTOR_LEFT_1, GPIO_OUTPUT_ACTIVE);
	int ret2 = gpio_pin_configure(dev, GPIO_MOTOR_LEFT_2, GPIO_OUTPUT_ACTIVE);
	int ret3 = gpio_pin_configure(dev, GPIO_MOTOR_RIGHT_1, GPIO_OUTPUT_ACTIVE);
	int ret4 = gpio_pin_configure(dev, GPIO_MOTOR_RIGHT_2, GPIO_OUTPUT_ACTIVE);
	pwm = DEVICE_DT_GET(PWM_CTLR);

	/*if ( !device_is_ready(pwm) ||  !device_is_ready(pwm1)) {
		printk("Error: PWM device %s is not ready\n", pwm->name);
		return;
	}
*/

	while (1) {
		//Need Lock
		if (status_Left == 1 && cm >= 5) { //black // left ir sensor activate
			pwm_pin_set_usec(pwm, PWM_CHANNEL, 20000U, 1000U, PWM_FLAGS);
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_2, Set); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_2, Unset);

			//2k 2k 8k
		}
		if (status_Right == 1 && cm >= 5) { // right ir sensor activate
			pwm_pin_set_usec(pwm, PWM_CHANNEL, 20000U, 1000U, PWM_FLAGS);
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_2, Set); //turn on§
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_2, Unset); //turn on
		}

		if (status_Left == 0 && status_Right == 0 && cm >= 5) { // both are not activate.
			pwm_pin_set_usec(pwm, PWM_CHANNEL, 20000U, 10000U, PWM_FLAGS);
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_2, Set); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_2, Set); //turn on
		}

		if (status_Left == 1 && status_Right == 1) { // both are  activate.

			gpio_pin_set(dev, GPIO_MOTOR_LEFT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_2, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_2, Unset); //turn on
		}
		printf("%d \n", cm);

		if (cm < 5) {
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_LEFT_2, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_1, Unset); //turn on
			gpio_pin_set(dev, GPIO_MOTOR_RIGHT_2, Unset); //turn on
		}

		/*gpio_pin_set(dev, GPIO_MOTOR_LEFT_1, Unset); //turn on
	gpio_pin_set(dev, GPIO_MOTOR_LEFT_2, Set); //turn on
	gpio_pin_set(dev, GPIO_MOTOR_RIGHT_1, Unset); //turn on
	gpio_pin_set(dev, GPIO_MOTOR_RIGHT_2, Set); //turn on*/

		k_sleep(K_SECONDS(0.001));
	}
}
void IR_Left(void)
{
	const struct device *dev;
	dev = device_get_binding(PORT);
	int ret3 = gpio_pin_configure(dev, IR_LEFT, GPIO_INPUT);

	while (1) {
		status_Left = gpio_pin_get(dev, IR_LEFT);
		//printf("%d \n", status_Left);
		k_sleep(K_SECONDS(0.1));
	}
}

void IR_Right(void)
{
	const struct device *dev;
	dev = device_get_binding(PORT);
	int ret3 = gpio_pin_configure(dev, IR_RIGHT, GPIO_INPUT);

	while (1) {
		status_Right = gpio_pin_get(dev, IR_RIGHT);
		//printf("%d \n", status_Right);
		k_sleep(K_SECONDS(0.1));
	}
}

void Distance_Task(void)
{
	uint32_t cycles_spent;
	uint32_t nanseconds_spent;
	uint32_t val;

	uint32_t stop_time;
	uint32_t start_time;
	struct device *dev;
	dev = device_get_binding(PORT);
	gpio_pin_configure(dev, GPIO_OUT_PIN, GPIO_OUTPUT);
	gpio_pin_configure(dev, GPIO_INT_PIN,
			   (GPIO_INPUT | GPIO_INT_EDGE | GPIO_ACTIVE_HIGH | GPIO_INT_DEBOUNCE));

	while (1) {
		gpio_pin_set(dev, GPIO_OUT_PIN, 1);
		k_sleep(K_MSEC(10));
		gpio_pin_set(dev, GPIO_OUT_PIN, 0);
		do {
			val = gpio_pin_get(dev, GPIO_INT_PIN);
		} while (val == 0);
		start_time = k_cycle_get_32();

		do {
			val = gpio_pin_get(dev, GPIO_INT_PIN);
			stop_time = k_cycle_get_32();
			cycles_spent = stop_time - start_time;
			if (cycles_spent >
			    1266720) //260cm for 84MHz (((MAX_RANGE * 58000) / 1000000000) * (CLOCK * 1000000))
			{
				break;
			}
		} while (val == 1);
		nanseconds_spent = k_cyc_to_ns_ceil32(cycles_spent);
		cm = nanseconds_spent / 58000;

		//float duration = cycles_spent;// in seconds

		//float Distance = (duration*34300)/2;
		printf("%d\n", cm);
		k_sleep(K_SECONDS(0.1));
	}
}

K_THREAD_DEFINE(Motor_ID, MY_STACK_SIZE, Motor_Task, NULL, NULL, NULL, 0, 0, 0);
K_THREAD_DEFINE(IR_LEFT_ID, MY_STACK_SIZE, IR_Left, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(IR_RIGHT_ID, MY_STACK_SIZE, IR_Right, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(DISTANCE_ID, MY_STACK_SIZE, Distance_Task, NULL, NULL, NULL, 0, 0, 0);

