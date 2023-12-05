//Implement a program for switching LEDs on/off and dimming them. The program should work as follows:
//SW1, the middle button is the on/off button. When button is pressed the state of LEDs is toggled. Program must require the button to be released before the LEDs toggle again. Holding the button may not cause LEDs to toggle multiple times.
//SW0 and SW2 are used to control dimming when LEDs are in ON state. SW0 increases brightness and SW2 decreases brightness. Holding a button makes the brightness to increase/decrease smoothly. If LEDs are in OFF state the buttons have no effect.
//When LED state is toggled to ON the program must use same brightness of the LEDs they were at when they were switched off. If LEDs were dimmed to 0% then toggling them on will set 50% brightness.
//PWM frequency divider must be configured to output 1 MHz frequency and PWM frequency must be 1 kHz.

#include <stdio.h>
#include <pico/time.h>
#include "pico/stdio.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define SW_0 9 //increase brightness
#define SW_1 8
#define SW_2 7 //decrease brightness
#define LED1 22
#define LED2 21
#define LED3 20
#define CLOCK_DIVIDER 125
#define TOP 999  //high limit (wrap takes place every TOP +1 clock cycle)

bool pressed(uint button);
void turn_on_all_lights(uint *duty_cycle, uint *slice_nr1, uint *channel1, uint *slice_nr2, uint *channel2, uint *slice_nr3, uint *channel3);
void initializePins(uint *slice_l1, uint *channel_l1, uint *slice_l2, uint *channel_l2, uint *slice_l3, uint *channel_l3, uint *duty_cycle);


int main(){

    uint duty_cycle = 0;
    uint slice_l1, slice_l2, slice_l3;
    uint channel_l1, channel_l2, channel_l3;

    initializePins(&slice_l1, &channel_l1, &slice_l2, &channel_l2, &slice_l3, &channel_l3, &duty_cycle);

    bool led_on = false;
    uint brightness_state = 0;

    while(true) {
        if (!pressed(SW_1) && !duty_cycle) {
            if (duty_cycle == 0) {
                if (brightness_state == 0) {
                    duty_cycle = 50;
                    brightness_state = duty_cycle;
                } else {
                    duty_cycle = brightness_state;
                }
                printf("brightness level is %u & duty cycle turned on %u\n", brightness_state, duty_cycle);
                turn_on_all_lights(&duty_cycle, &slice_l1, &channel_l1, &slice_l2, &channel_l2, &slice_l3, &channel_l3);
                led_on = true;
                while(!gpio_get(SW_1));
            }
            printf("Button pressed, turn on lights\n");
        }else if(!pressed(SW_1) && duty_cycle){
            brightness_state = duty_cycle;
            duty_cycle = 0;
            turn_on_all_lights(&duty_cycle, &slice_l1, &channel_l1, &slice_l2, &channel_l2, &slice_l3, &channel_l3);
            printf("Button pressed, Turn off lights\n");
            led_on = false;
            while (!gpio_get(SW_1));
        }
        if (led_on) {
            if (!gpio_get(SW_0)) {
                if (brightness_state < 100) {
                    printf("Increase brightness\n");
                    brightness_state += 5;
                    duty_cycle = brightness_state;
                    printf("Brightness: %u\n", brightness_state);
                    turn_on_all_lights(&duty_cycle, &slice_l1, &channel_l1, &slice_l2, &channel_l2, &slice_l3, &channel_l3);
                    sleep_ms(300);
                }
            }
            if (!gpio_get(SW_2)) {
                if (brightness_state > 0) {
                    printf("Decrease brightness\n");
                    brightness_state -= 5;
                    duty_cycle = brightness_state;
                    printf("Brightness: %u\n", brightness_state);
                    turn_on_all_lights(&duty_cycle, &slice_l1, &channel_l1, &slice_l2, &channel_l2, &slice_l3, &channel_l3);
                    sleep_ms(300);
                }
            }
        }
    }
    return 0;
}

bool pressed(uint button) {
    int press = 0;
    int release = 0;
    while (press < 3 && release < 3) {
        if (gpio_get(button)) {
            press++;
            release = 0;
        } else {
            release++;
            press = 0;
        }
        sleep_ms(10);
    }
    if (press > release) return true;
    else return false;
}

void turn_on_all_lights(uint *duty_cycle, uint *slice_nr1, uint *channel1, uint *slice_nr2, uint *channel2, uint *slice_nr3, uint *channel3){
    pwm_set_chan_level(*slice_nr1, *channel1, (TOP + 1) * *duty_cycle / 100);
    pwm_set_chan_level(*slice_nr2, *channel2, (TOP + 1) * *duty_cycle / 100);
    pwm_set_chan_level(*slice_nr3, *channel3, (TOP + 1) * *duty_cycle / 100);
}

void initializePins(uint *slice_l1, uint *channel_l1, uint *slice_l2, uint *channel_l2, uint *slice_l3, uint *channel_l3, uint *duty_cycle){
    //Initialize chosen serial port
    stdio_init_all();

    //Initialize led
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);
    gpio_init(LED2);
    gpio_set_dir(LED2, GPIO_OUT);
    gpio_init(LED3);
    gpio_set_dir(LED3, GPIO_OUT);

    //Initialize buttons
    gpio_set_function(SW_0, GPIO_FUNC_SIO);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);

    gpio_set_function(SW_1, GPIO_FUNC_SIO);
    gpio_set_dir(SW_1, GPIO_IN);
    gpio_pull_up(SW_1);

    gpio_set_function(SW_2, GPIO_FUNC_SIO);
    gpio_set_dir(SW_2, GPIO_IN);
    gpio_pull_up(SW_2);

    //Configure PWM led lights
    pwm_config config = pwm_get_default_config();

    // LED1: (3A)
    *slice_l1 = pwm_gpio_to_slice_num(LED1);
    *channel_l1 = pwm_gpio_to_channel(LED1);
    pwm_set_enabled(*slice_l1, false);
    pwm_config_set_clkdiv_int(&config, CLOCK_DIVIDER);
    pwm_config_set_wrap(&config, TOP);
    pwm_init(*slice_l1, &config, false);
    pwm_set_chan_level(*slice_l1, *channel_l1, (TOP + 1) * *duty_cycle / 100);
    gpio_set_function(LED1, GPIO_FUNC_PWM);
    pwm_set_enabled(*slice_l1, true);

    // LED2: (2B)
    *slice_l2 = pwm_gpio_to_slice_num(LED2);
    *channel_l2 = pwm_gpio_to_channel(LED2);
    pwm_set_enabled(*slice_l2, false);
    pwm_init(*slice_l2, &config, false);
    pwm_set_chan_level(*slice_l2, *channel_l2, (TOP + 1) * *duty_cycle / 100);
    gpio_set_function(LED2, GPIO_FUNC_PWM);
    pwm_set_enabled(*slice_l2, true);

    // LED3: (2A)
    *slice_l3 = pwm_gpio_to_slice_num(LED3);
    *channel_l3 = pwm_gpio_to_channel(LED3);
    pwm_set_enabled(*slice_l3, false);
    pwm_init(*slice_l3, &config, false);
    pwm_set_chan_level(*slice_l3, *channel_l3, (TOP + 1) * *duty_cycle / 100);
    gpio_set_function(LED3, GPIO_FUNC_PWM);
    pwm_set_enabled(*slice_l3, true);

}
