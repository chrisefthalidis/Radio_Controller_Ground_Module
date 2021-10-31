#include "NRF24L01.h"

// Important Note: Everything through SPI commands only

void NRF24L01_Read_Register(uint8_t reg_addr, uint8_t *data)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); // CSN pin low to begin the transaction
    HAL_SPI_TransmitReceive(&hspi3, &reg_addr, data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); // CSN pin high to end the transaction
}

void NRF24L01_Write_Register(uint8_t reg_addr, uint8_t *data)
{
    uint8_t reg_plus_data[2] = {NRF24L01_CMD_W_REGISTER | reg_addr, *data}; // Merge register address with the W_REGISTER command

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi3, reg_plus_data, sizeof(reg_plus_data), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
}

void NRF24L01_Read_Payload(uint8_t *data, uint8_t length)
{
    uint8_t cmd = NRF24L01_CMD_R_RX_PAYLOAD;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY); // First, send the R_RX_PAYLOAD command

    for (uint8_t i = 0; i < length; i++)
        HAL_SPI_Receive(&hspi3, &data[i], 1, HAL_MAX_DELAY); // Read multiple bytes from RX FIFO

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
}

void NRF24L01_Write_Payload(uint8_t *data, uint8_t length)
{
    uint8_t cmd = NRF24L01_CMD_W_TX_PAYLOAD;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY); // First, send the W_TX_PAYLOAD command

    for (uint8_t i = 0; i < length; i++)
        HAL_SPI_Transmit(&hspi3, &data[i], 1, HAL_MAX_DELAY); // Write multiple bytes to TX FIFO

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
}

void NRF24L01_Initialise_Tx()
{
    uint8_t data;

    // The first register must be written twice because of SPI quirk
    data = 0x0A; // Power up, TX mode, 1 byte CRC
    NRF24L01_Write_Register(NRF24L01_REG_CONFIG, &data);
    NRF24L01_Write_Register(NRF24L01_REG_CONFIG, &data);

    data = 0x01; // Enable auto-acknowledgement for pipe 0
    NRF24L01_Write_Register(NRF24L01_REG_EN_AA, &data);

    data = 0x01; // Enable pipe 0
    NRF24L01_Write_Register(NRF24L01_REG_EN_RXADDR, &data);

    data = 0x01; // 1 re-transmission after 250us
    NRF24L01_Write_Register(NRF24L01_REG_SETUP_RETR, &data);

    data = 0x06; // 1Mbps data rate, 0dBm output power
    NRF24L01_Write_Register(NRF24L01_REG_RF_SETUP, &data);

    data = 0x01; // Enable dynamic payload length for pipe 0
    NRF24L01_Write_Register(NRF24L01_REG_DYNPD, &data);

    data = 0x04; // Enable dynamic payload length
    NRF24L01_Write_Register(NRF24L01_REG_FEATURE, &data);
}

void NRF24L01_Initialise_Rx()
{
    uint8_t data;

    // The first register must be written twice because of SPI quirk
    data = 0x0B; // Power up, RX mode, 1 byte CRC
    NRF24L01_Write_Register(NRF24L01_REG_CONFIG, &data);
    NRF24L01_Write_Register(NRF24L01_REG_CONFIG, &data);

    data = 0x01; // Enable auto-acknowledgement for pipe 0
    NRF24L01_Write_Register(NRF24L01_REG_EN_AA, &data);

    data = 0x01; // Enable pipe 0
    NRF24L01_Write_Register(NRF24L01_REG_EN_RXADDR, &data);

    data = 0x06; // 1Mbps data rate, 0dBm output power
    NRF24L01_Write_Register(NRF24L01_REG_RF_SETUP, &data);

    data = 0x01; // Enable dynamic payload length for pipe 0
    NRF24L01_Write_Register(NRF24L01_REG_DYNPD, &data);

    data = 0x04; // Enable dynamic payload length
    NRF24L01_Write_Register(NRF24L01_REG_FEATURE, &data);

    HAL_Delay(1);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET); // CE pin high to start receiving
}

void NRF24L01_Transmit(uint8_t *data, uint8_t length)
{
    uint8_t tx_data, rx_data;

    NRF24L01_Write_Payload(data, length); // Write data into TX FIFO

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET); // CE pin high to start transmitting
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // CE pin low to stop transmitting

    NRF24L01_Read_Register(NRF24L01_REG_STATUS, &rx_data); // Read the STATUS register

    if (rx_data & 0b00010000) // If MAX_RT bit in STATUS register is asserted...
    {
        tx_data = 0x10;
        NRF24L01_Write_Register(NRF24L01_REG_STATUS, &tx_data); // ...clear MAX_RT bit
    }
}

void NRF24L01_Receive(uint8_t *data, uint8_t length)
{
    NRF24L01_Read_Payload(data, length); // Read data from RX FIFO
}
