#include <main.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_i2c.h>
#include <RDA_5807.h>

int main(void)
{
    GPIO_InitTypeDef led;
    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    led.GPIO_Speed = GPIO_Speed_2MHz;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    GPIO_InitTypeDef  GPIO_InitStructure;
    I2C_InitTypeDef  I2C_InitStructure;

    /* Enable RCC clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

    /* I2C1 SDA (PB7) and SCL (PB6) configuration */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /*Enable I2C*/
    I2C_Cmd(I2C1, ENABLE);

    /* I2C1 configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Disable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_Init(I2C1, &I2C_InitStructure);

    RDA_Init(I2C1);
    RDA_SetBass(I2C1, TRUE);
    RDA_SetVolume(I2C1, 15);
    RDA_Tune(I2C1, (uint16_t)10400);
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);

    while (1)
    {
        /* code */
    }

    return 0;
}
