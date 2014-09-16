/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>

#define ENDPOINT_OUT        (0x01u)
#define ENDPOINT_IN         (0x02u)
#define DATA_LEN            (0x40u)
#define MAX_I2C_SIZE        (DATA_LEN-3)

//  Generic Command
//
//  dataOut[0] : Control
//    7 6 5 4 3 2 1 0
//    | | | | | | | +-- R/W
//    | | | | | | +---- START
//    | | | | +-------- STOP
//    | | +------------ CONFIG
//
#define     CTRL_RW             (0x01u)
#define     CTRL_START          (0x02u)
#define     CTRL_STOP           (0x08u)
#define     CTRL_CONFIG         (0x20u)

//  CONFIGURATION COMMAND
//
//  dataOut[0] : Config code
//    7 6 5 4 3 2 1 0
//    0 0 1   | |
//            +-+------ SPEED
//
//  SPEED
//    00 : 100kHz
//    01 : 400kHz
//    10 : 50kHz
//
#define     CONFIG_SPEED        (0x0cu)
#define     CONFIG_100K         (0x00u)
#define     CONFIG_400K         (0x04u)
#define     CONFIG_50K          (0x08u)

//  Length part
//
//  dataOut[1] : Long

//  Command part
//
//  dataOut[2] : Command
//    7 6 5 4 3 2 1 0
//    | +-+-+-+-+-+-+-- Command code
//    +---------------- Internal command
//
#define     COM_INTERNAL        (0x80u)
#define     COM_STATUS          (0x80u)
#define     COM_VERSION         (0x81u)

//  Status code
//
//  dataIn[0] : Status
//    7 6 5 4 3 2 1 0
//    | | | | | | | +-- ACK
//    | | | | | +------ VTARG
//
#define     STAT_ACK            (0x01u)
#define     STAT_VTARG          (0x04u)

//  Power code
#define     POWER_NONE          (0x00u)
#define     POWER_5p0V          (0x01u)
#define     POWER_3p3V          (0x02u)

// Buffer for endpoints
uint8 dataIn[64] = {0};
uint8 dataOut[64] = {0};

uint8 dataLength;
uint8 i2cSpeed = CONFIG_400K;
uint8 power = POWER_NONE;

// Return value for command
CYCODE const uint8 version[] = {0, 0, 0, 1, 0x01, 0x23, 0x00, 0xA5};

void stall(uint8 errorCode) {
    LCD_Position(1, 0);
    LCD_PrintString("ERR=");
    LCD_PrintHexUint8(errorCode);
    LCD_PrintString(" STA=");
    LCD_PrintHexUint8(I2CM_MasterStatus());
    for (;;) ;
}

uint8 parseCommand(void) {
    uint16 i;
    uint8 dataInIndex = 0;
    uint8 dataOutIndex = 0;
    uint8 control;
    uint8 command;
    uint8 slaveAddress;
    uint8 status;
    uint8 errorCode;
    
    dataInIndex = 0;
    dataIn[dataInIndex++] = 0;        // default status code
    
    LCD_Position(0, 0);
    for (i = 0; i < 8; i++) {
        LCD_PrintInt8(dataOut[i]);
    }
    LCD_PutChar(' ');
    
    control = dataOut[0];
    dataLength = dataOut[1];
    if (dataLength > MAX_I2C_SIZE) {
        LCD_Position(1, 0);
        LCD_PrintString("TOO LONG PACKET");
        for (;;) ;
    }
    
    if (control & CTRL_CONFIG) {
        // Configuration command
        i2cSpeed = control & CONFIG_SPEED;
        dataIn[0] |= STAT_ACK;
    } else if (control & CTRL_START) {
        command = dataOut[2];
        if (command & COM_INTERNAL) {
            dataIn[0] |= STAT_VTARG;
            if (command == COM_STATUS) {
                if (control & CTRL_RW) {
                    // Return status code
                    dataIn[1] = power;          // Power
                    dataIn[2] = i2cSpeed;       // I2C speed
                    dataIn[0] |= STAT_ACK;
                } else {
                    // Write control code
                    power = dataOut[3];
                    dataIn[0] = STAT_ACK;
                }
            } else if (command == COM_VERSION) {
                // Get version
                for (i = 0; i < sizeof version; i++) {
                    dataIn[dataInIndex++] = version[i];
                }
                dataIn[0] |= STAT_ACK;
            } else {
                LCD_Position(1, 0);
                LCD_PrintString("UNK INT COM");
                for (;;) ;
            }
        } else {
            // I2C bus access
            slaveAddress = dataOut[2];
            if (control & CTRL_RW) {
                // READ operation
                I2CM_MasterClearStatus();
                // Start transfer
                errorCode = I2CM_MasterReadBuf(
                    slaveAddress,
                    &dataIn[dataInIndex], dataLength,
                    I2CM_MODE_COMPLETE_XFER
                );
                // Wait for transfer ends
                while (!((status = I2CM_MasterStatus()) & I2CM_MSTAT_RD_CMPLT)) ;
                if (status & (~I2CM_MSTAT_RD_CMPLT)) {
                    // Some error reported
                    if (status & I2CM_MSTAT_ERR_ADDR_NAK) {
                        // Return NAK for address NAK
                    } else {
                        stall(errorCode);
                    }
                } else {
                    dataIn[0] |= STAT_ACK;
                }
            } else {
                // WRITE operation
                dataOutIndex = 3;
                I2CM_MasterClearStatus();
                // Start transfer
                errorCode = I2CM_MasterWriteBuf(
                    slaveAddress,
                    &dataOut[dataOutIndex], dataLength,
                    I2CM_MODE_COMPLETE_XFER
                );
                // Wait for transfer ends
                while (!((status = I2CM_MasterStatus()) & I2CM_MSTAT_WR_CMPLT)) ;
                if (status & (~I2CM_MSTAT_WR_CMPLT)) {
                    // Some error reported
                    if (status & I2CM_MSTAT_ERR_ADDR_NAK) {
                        // Return NAK for address NAK
                    } else {
                        stall(errorCode);
                    }
                } else {
                    // Return status for all bytes
                    for (i = 0; i < dataLength; i++) {
                        dataIn[dataInIndex++] = 1;
                    }
                    dataIn[0] |= STAT_ACK;
                }
            }
            if (!(control & CTRL_STOP)) {
                LCD_Position(1, 0);
                LCD_PrintString("NO STOP");
                for (;;) ;
            }        
        }
    } else {
        LCD_Position(1, 0);
        LCD_PrintString("NO START");
        for (;;) ;
    }
    while (!(USBFS_GetEPState(ENDPOINT_IN) & USBFS_IN_BUFFER_EMPTY));
    USBFS_LoadInEP(ENDPOINT_IN, dataIn, DATA_LEN);
    return 0;
}

int main()
{
    uint8 counter = 0u;
    
    /* Enable Global Interrupts */
    CyGlobalIntEnable;
    
    I2CM_Start();
    
    /* Start USBFS device 0 with 3V operation */
    USBFS_Start(0u, USBFS_3V_OPERATION); 
    
    LCD_Start();                                // LCDの初期化

    for (;;) {
        // 初期化終了まで待機
        while (USBFS_GetConfiguration() == 0);
        
        USBFS_IsConfigurationChanged();         // CHANGEフラグを確実にクリアする
        
        USBFS_EnableOutEP(ENDPOINT_OUT);        // OUTエンドポイントを起動する

        for (;;) {
            // 設定が変更されたら、再初期化を行う
            if (USBFS_IsConfigurationChanged()) {
                break;
            }
            
            // OUTパケットの到着待ち
            if (USBFS_GetEPState(ENDPOINT_OUT) & USBFS_OUT_BUFFER_FULL) {
                uint16 len = USBFS_GetEPCount(ENDPOINT_OUT);
                USBFS_ReadOutEP(ENDPOINT_OUT, dataOut, len);
                USBFS_EnableOutEP(ENDPOINT_OUT);        // OUTエンドポイントを起動する
                if (parseCommand()) break;
            }
        }
    }
}

/* [] END OF FILE */
