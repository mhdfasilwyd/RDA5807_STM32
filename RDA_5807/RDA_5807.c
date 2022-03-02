#include <RDA_5807.h>
#include <stdlib.h>

#define WRITE_DELAY 3
#define MIN_DELAY 1

const uint16_t startBand[4] = {8700, 7600, 7600, 6500};
const uint16_t endBand[4] = {10800, 9100, 10800, 7600};
const uint16_t fmSpace[4] = {100, 200, 50, 25};

typedef struct
{
    // REG01
	RDA_Reg01 reg01;
    // REG02
	RDA_Reg02 reg02;
    // REG03
	RDA_Reg03 reg03;
    // REG04
	RDA_Reg04 reg04;
    // REG05
	RDA_Reg05 reg05;
    // REG06
	RDA_Reg06 reg06;
    // REG07
	RDA_Reg07 reg07;
    // REG08
	RDA_Reg08 reg08;
    // REG0A
	RDA_Reg0A reg0A;
    // REG0B
	RDA_Reg0B reg0B;
    // REG0C
	RDA_Reg0C reg0C;
    // REG0D
	RDA_Reg0D reg0D;
    // REG0E
	RDA_Reg0E reg0E;
    // REG0F
	RDA_Reg0F reg0F;
    // FM frequency
    uint16_t currentFrequency;
    // FM band
    uint8_t currentFMBand;
    // FM space
    uint8_t currentFMSpace;
    // FM volume
    uint8_t currentVolume;
} RDA_Handle;

static RDA_Handle handle = {};

typedef union {
    struct
    {
        uint8_t low;
        uint8_t high;
    } write;
    uint16_t all;
} wordToByte;

#ifdef SYSTICK_DELAY
#define MS_CORE (SystemCoreClock / 1000)
__IO uint32_t systickValue = 0;

void Delay_Init()
{
	SystemCoreClockUpdate(); // Update SystemCoreClock variable
	SysTick_Config(MS_CORE); // 1 Milli second
}

uint32_t getMillis()
{
	return systickValue;
}

void Delay(uint32_t delay)
{
	uint32_t startMs = getMillis();
	while ((getMillis() - startMs) < delay);
}

/*
 * SysTick Interrupt Handler
 */
void SysTick_Handler()
{
	systickValue++;
}
#endif

void I2C_Start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction)
{
	// Wait until I2Cx is not busy anymore
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY));
	// Send I2Cx START condition 
	I2C_GenerateSTART(I2Cx, ENABLE);
	// Wait for I2Cx EV5 --- Slave has acknowledged start condition
	while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
	// Send slave Address for write 
	I2C_Send7bitAddress(I2Cx, address << 1, direction);
	/* Wait for I2Cx EV6, check if
	 * either slave has acknowledged Master transmitter or
	 * master receiver mode, depending on the transmission
	 * direction
	 */
	if(direction == I2C_Direction_Transmitter)
    {
		while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	}
	else
    {
		while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
	}
}

/* This function transmits one byte to the slave device
 * Parameters:
 * I2Cx --- the I2C peripheral e.g.I2C1, I2C2
 * data --- the data byte to be transmitted
 */
void I2C_Write(I2C_TypeDef* I2Cx, uint8_t data)
{
	I2C_SendData(I2Cx, data);
	// Wait for I2Cx EV8_2 --- byte has been transmitted
	while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

/* This function reads one byte from the slave device 
 * and based on mode, acknowledges the byte (requests another byte)
 */
void I2C_Read(I2C_TypeDef* I2Cx, uint8_t mode, uint16_t *buffer)
{
    wordToByte data = {};
	// Wait until one byte has been received
	while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED));
	// Read data from I2Cx data register and return data byte
	data.write.low = I2C_ReceiveData(I2Cx);
    if(mode)
    {
        // Enable acknowledge of recieved data
        I2C_AcknowledgeConfig(I2Cx, ENABLE);
    }
    else
    {
        // Disable acknowledge of received data
        // NACK also generates stop condition after last byte received
        // See reference manual for more info
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        I2C_GenerateSTOP(I2Cx, ENABLE);
    }
    // Wait until one byte has been received
	while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED));
	// Read data from I2Cx data register and return data byte
	data.write.high = I2C_ReceiveData(I2Cx);
    // Copy the data to buffer
	*buffer = data.all;
}

/* This funtion issues a stop condition and therefore
 * releases the bus
 */
void I2C_Stop(I2C_TypeDef* I2Cx)
{
	// Send I2Cx STOP condition 
	I2C_GenerateSTOP(I2Cx, ENABLE);
}

void registerWrite(I2C_TypeDef* I2Cx, uint8_t reg, uint16_t value)
{
    wordToByte data = {};
    data.all = value;
    I2C_Start(I2Cx, I2C_ADDR_DIRECT_ACCESS, I2C_Direction_Transmitter);
    I2C_Write(I2Cx, reg); // Write address of the reg
    I2C_Write(I2Cx, data.write.high); // Write byte by byte to the slave
    I2C_Write(I2Cx, data.write.low);
    I2C_Stop(I2Cx);
#ifdef SYSTICK_DELAY
    Delay(WRITE_DELAY);
#endif
}

/**
 * @ingroup GA03
 * @brief Gets the register content of a given status register (from 0x0A to 0x0F) 
 * @details Useful when you need just a specific status register content.
 * @details This method update the first element of the shadowStatusRegisters linked to the register
 * @return rdax_reg0a the reference to current value of the 0x0A register. 
 */
void getStatus(I2C_TypeDef* I2Cx, uint8_t reg)
{
    uint16_t temp = 0x0;

    if (reg < 0x0A || reg > 0x0F)
    {
        return; // Maybe not necessary.
    }

    I2C_Start(I2Cx, I2C_ADDR_DIRECT_ACCESS, I2C_Direction_Transmitter);
    I2C_Write(I2Cx, reg); // Request specific register access
    I2C_Stop(I2Cx);

    I2C_Start(I2Cx, I2C_ADDR_DIRECT_ACCESS, I2C_Direction_Receiver);
    I2C_Read(I2Cx, FALSE, &temp); // Read data from the register
    I2C_Stop(I2Cx);

    switch (reg)
    {
    case REG0A:
        handle.reg0A = (RDA_Reg0A)temp;
        break;
    case REG0B:
        handle.reg0B = (RDA_Reg0B)temp;
        break;
    case REG0C:
        handle.reg0C = (RDA_Reg0C)temp;
        break;
    case REG0D:
        handle.reg0D = (RDA_Reg0D)temp;
        break;
    case REG0E:
        handle.reg0E = (RDA_Reg0E)temp;
        break;
    case REG0F:
        handle.reg0F = (RDA_Reg0F)temp;
        break;
    default:
        break;
    }
}

/**
 * @ingroup GA03
 * @brief Waits for Seek or Tune finish
 */
void waitAndFinishTune(I2C_TypeDef* I2Cx)
{
    do
	{
        getStatus(I2Cx, REG0A);
#ifdef SYSTICK_DELAY
        Delay(MIN_DELAY);
#endif
    }
	while (handle.reg0A.refined.STC == 0);
}

/**
 * @ingroup RDA_API
 * @brief Init the RDA chip
 * @param I2Cx I2C Port
 */
void RDA_Init(I2C_TypeDef* I2Cx)
{
#ifdef SYSTICK_DELAY
    Delay_Init();
    Delay(MIN_DELAY);
#endif
    handle.reg02.raw = 0x0;
    handle.reg02.refined.NEW_METHOD = 0;
    handle.reg02.refined.RDS_EN = 0; // RDS disable
    handle.reg02.refined.CLK_MODE = CLOCK_32K;
    handle.reg02.refined.RCLK_DIRECT_IN = OSCILLATOR_TYPE_CRYSTAL;
    handle.reg02.refined.MONO = 1; // Force mono
    handle.reg02.refined.DMUTE = 1; // Normal operation
    handle.reg02.refined.DHIZ = 1; // Normal operation
    handle.reg02.refined.ENABLE = 1;
    handle.reg02.refined.BASS = 1;
    handle.reg02.refined.SEEK = 0;
    registerWrite(I2Cx, REG02, handle.reg02.raw);
    
    handle.reg05.raw = 0x0;
    handle.reg05.refined.INT_MODE = 0;
    handle.reg05.refined.LNA_PORT_SEL = 2;
    handle.reg05.refined.LNA_ICSEL_BIT = 0;
    handle.reg05.refined.SEEKTH = 8; // 0B1000
    handle.reg05.refined.VOLUME = 0;
    registerWrite(I2Cx, REG05, handle.reg05.raw);
}

/**
 * @ingroup RDA_API
 * @brief De-Init the RDA chip
 * @param I2Cx I2C Port
 */
void RDA_DeInit(I2C_TypeDef* I2Cx)
{
    handle.reg02.refined.SEEK = 0;
	handle.reg02.refined.ENABLE = 0;
    registerWrite(I2Cx, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Soft reset the RDA chip
 * @param I2Cx I2C Port
 */
void RDA_SoftReset(I2C_TypeDef* I2Cx)
{
    handle.reg02.refined.SOFT_RESET = 1;
    registerWrite(I2Cx, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API (Internal)
 * @brief Set channel on RDA chip
 * @param I2Cx I2C Port
 * @param channel channel
 */
void RDA_SetChannel(I2C_TypeDef* I2Cx, uint16_t channel)
{
    handle.reg03.refined.CHAN = channel;
    handle.reg03.refined.TUNE = 1;
    handle.reg03.refined.BAND = 0;
    handle.reg03.refined.SPACE = 0;
    handle.reg03.refined.DIRECT_MODE = 0;
    registerWrite(I2Cx, REG03, handle.reg03.raw);
    waitAndFinishTune(I2Cx);
}

/**
 * @ingroup RDA_API
 * @brief Set frequency on RDA chip
 * @param I2Cx I2C Port
 * @param frequency frequency
 */
void RDA_Tune(I2C_TypeDef* I2Cx, uint16_t frequency)
{
    uint16_t channel = (frequency - startBand[handle.currentFMBand] ) / (fmSpace[handle.currentFMSpace] / 10.0);
    RDA_SetChannel(I2Cx, channel);
    handle.currentFrequency = frequency;
}

/**
 * @ingroup RDA_API
 * @brief Call manual seek down on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_ManualDown(I2C_TypeDef* I2Cx)
{
    if (handle.currentFrequency < endBand[handle.currentFMBand])
    {
        handle.currentFrequency += (fmSpace[handle.currentFMSpace] / 10.0);
    }
    else
    {
        handle.currentFrequency = startBand[handle.currentFMBand];
    }
    RDA_Tune(I2Cx, handle.currentFrequency);
}

/**
 * @ingroup RDA_API
 * @brief Call manual seek up on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_ManualUp(I2C_TypeDef* I2Cx)
{
    if (handle.currentFrequency > startBand[handle.currentFMBand])
    {
        handle.currentFrequency -= (fmSpace[handle.currentFMSpace] / 10.0);
    }
    else
    {
        handle.currentFrequency = endBand[handle.currentFMBand];
    }
    RDA_Tune(I2Cx, handle.currentFrequency);
}

/**
 * @ingroup RDA_API (Internal)
 * @brief Get real channel on RDA chip
 * @param I2Cx I2C Port
 */
uint16_t RDA_GetRealChannel(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0A);
    return handle.reg0A.refined.READCHAN;
}

/**
 * @ingroup RDA_API
 * @brief Get real frequency on RDA chip
 * | Band   | Formula |
 * | ------ | ------- | 
 * |    0   | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 87.0 MHz |
 * | 1 or 2 | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 76.0 MHz |
 * |    3   | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 65.0 MHz |
 * @param I2Cx I2C Port
 */
uint16_t RDA_GetRealFrequency(I2C_TypeDef* I2Cx)
{
    return (RDA_GetRealChannel(I2Cx) * (fmSpace[handle.currentFMSpace] / 10.0) + startBand[handle.currentFMBand]);
}

/**
 * @ingroup RDA_API
 * @brief Call seek on RDA chip
 * @param I2Cx I2C Port
 * @param seek_mode if 0, wrap at the upper or lower band limit and continue seeking; 1 = stop seeking at the upper or lower band limit
 * @param direction if 0, seek down; if 1, seek up.
 */
void RDA_Seek(I2C_TypeDef* I2Cx, uint8_t seek_mode, uint8_t direction)
{
    handle.reg02.refined.SEEK = 1;
    handle.reg02.refined.SKMODE = seek_mode;
    handle.reg02.refined.SEEKUP = direction;
    registerWrite(I2Cx, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set seek threshold on RDA chip
 * @param I2Cx I2C Port
 * @param value RSSI threshold
 */
void RDA_SetSeekThreshold(I2C_TypeDef* I2Cx, uint8_t value)
{
    handle.reg05.refined.SEEKTH = value;
    registerWrite(I2Cx, REG05, handle.reg05.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set FM band on RDA chip
 * @param I2Cx I2C Port
 * @param band FM band
 * | Value | Description                 |
 * | ----- | --------------------------- |
 * | 00    | 87–108 MHz (US/Europe)      |
 * | 01    | 76–91 MHz (Japan)           |
 * | 10    | 76–108 MHz (world wide)     |
 * | 11    | 65 –76 MHz (East Europe) or 50-65MHz (see bit 9 of gegister 0x06) |
 */
void RDA_SetBand(I2C_TypeDef* I2Cx, uint8_t band)
{
    handle.reg03.refined.BAND = band;
    registerWrite(I2Cx, REG03, handle.reg03.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set FM space on RDA chip
 * @param I2Cx I2C Port
 * @param space FM space
 * | Value | Description |
 * | ----- | ----------- |
 * | 00    | 100KHz      |
 * | 01    | 200KHz      |
 * | 10    | 50KHz       |
 * | 11    | 25KHz       |
 */
void RDA_SetSpace(I2C_TypeDef* I2Cx, uint8_t space)
{
    handle.reg03.refined.SPACE = space;
    registerWrite(I2Cx, REG03, handle.reg03.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get the current RSSI
 * @param I2Cx I2C Port
 * @details RSSI - 000000(Min) 111111(Max) RSSI scale is logarithmic.
 * @return int32_t
 */
int32_t RDA_GetQuality(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0B);
    return handle.reg0B.refined.RSSI;
}

/**
 * @ingroup RDA_API
 * @brief Set FM soft mute on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetSoftMute(I2C_TypeDef* I2Cx, BOOL value)
{
    handle.reg04.refined.SOFTMUTE_EN = value;
    registerWrite(I2Cx, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set FM mute on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetMute(I2C_TypeDef* I2Cx, BOOL value)
{
    handle.reg02.refined.SEEK = 0;    
    handle.reg02.refined.DHIZ = !value;
    registerWrite(I2Cx, REG02, handle.reg02.raw); 
}

/**
 * @ingroup RDA_API
 * @brief Set mono on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetMono(I2C_TypeDef* I2Cx, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.MONO = value;
    registerWrite(I2Cx, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set bass on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetBass(I2C_TypeDef* I2Cx, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.BASS = value;
    registerWrite(I2Cx, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get mono status on RDA chip
 * @param I2Cx I2C Port
 * @return value TRUE/FALSE
 */
BOOL RDA_GetSterioStatus(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0A);
    return handle.reg0A.refined.ST;
}

/**
 * @ingroup RDA_API
 * @brief Set volume on RDA chip
 * @param I2Cx I2C Port
 * @param value 0-15 levels
 */
void RDA_SetVolume(I2C_TypeDef* I2Cx, uint8_t value)
{
    value > 15 ? value = 15 : value;
    handle.reg05.refined.VOLUME = handle.currentVolume = value;
    registerWrite(I2Cx, REG05, handle.reg05.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get internal volume on RDA chip
 * @param I2Cx I2C Port
 * @return uint8_t 0-15 levels
 */
uint8_t RDA_GetVolume(I2C_TypeDef* I2Cx)
{
    return(handle.currentVolume);
}

/**
 * @ingroup RDA_API
 * @brief Call volume up on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_SetVolumeUp(I2C_TypeDef* I2Cx)
{
    if (handle.currentVolume < 15)
    {
        handle.currentVolume++;
        RDA_SetVolume(I2Cx, handle.currentVolume);
    }
}

/**
 * @ingroup RDA_API
 * @brief Call volume down on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_SetVolumeDown(I2C_TypeDef* I2Cx)
{
    if (handle.currentVolume > 0)
    {
        handle.currentVolume--;
        RDA_SetVolume(I2Cx, handle.currentVolume);
    }
}

/**
 * @ingroup RDA_API
 * @brief Set FM De-Emphasis on RDA chip
 * @param I2Cx I2C Port
 * @param deEmphasis deEmphasis
 */
void RDA_SetFMDeEmphasis(I2C_TypeDef* I2Cx, uint8_t deEmphasis)
{
    handle.reg04.refined.DE = deEmphasis;
    registerWrite(I2Cx, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set RDS on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetRDS(I2C_TypeDef* I2Cx, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.RDS_EN = value;
    registerWrite(I2Cx, REG02, handle.reg02.raw);
}

/**
 * @ingroup RDA_API
 * @brief Set RBDS on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetRBDS(I2C_TypeDef* I2Cx, BOOL value)
{
    handle.reg02.refined.SEEK = 0;
    handle.reg02.refined.RDS_EN = 1;
    registerWrite(I2Cx, REG02, handle.reg02.raw);

    handle.reg04.refined.RBDS = value;
    registerWrite(I2Cx, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Get RDS Ready on RDA chip
 * @param I2Cx I2C Port
 * @return TRUE/FALSE
 */
BOOL RDA_GetRDSReady(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0A);
    return(handle.reg0A.refined.RDSR);
}

/**
 * @ingroup RDA_API
 * @brief Get RDS Sync on RDA chip
 * @param I2Cx I2C Port
 * @return TRUE/FALSE
 */
BOOL RDA_GetRDSSync(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0A);
    return handle.reg0A.refined.RDSS;
}

/**
 * @ingroup RDA_API
 * @brief Get Block ID on RDA chip
 * @param I2Cx I2C Port
 * @return uint8_t
 */
uint8_t RDA_GetBlockId(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0B);
    return handle.reg0B.refined.ABCD_E;
}

/**
 * @ingroup RDA_API
 * @brief Set volume down on RDA chip
 * @param I2Cx I2C Port
 * @return uint8_t
 */
uint8_t RDA_GetErrorBlockB(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0B);
    return handle.reg0B.refined.BLERB;
}

/**
 * @ingroup RDA_API
 * @brief Get RDS info state on RDA chip
 * @param I2Cx I2C Port
 * @return TRUE/FALSE
 */
BOOL RDA_GetRDSInfoState(I2C_TypeDef* I2Cx)
{
    getStatus(I2Cx, REG0B);
    return(handle.reg0A.refined.RDSS && handle.reg0B.refined.ABCD_E == 0 && handle.reg0B.refined.BLERB == 0);
}

/**
 * @ingroup RDA_API
 * @brief Set RDS FIFO on RDA chip
 * @param I2Cx I2C Port
 * @param value TRUE/FALSE
 */
void RDA_SetRDSFifo(I2C_TypeDef* I2Cx, BOOL value)
{
    handle.reg04.refined.RDS_FIFO_EN = value;
    registerWrite(I2Cx, REG04, handle.reg04.raw);
}

/**
 * @ingroup RDA_API
 * @brief Call clear RDS FIFO on RDA chip
 * @param I2Cx I2C Port
 */
void RDA_ClearRDSFifo(I2C_TypeDef* I2Cx)
{
    handle.reg04.refined.RDS_FIFO_CLR = 1;
    registerWrite(I2Cx, REG04, handle.reg04.raw);
}
