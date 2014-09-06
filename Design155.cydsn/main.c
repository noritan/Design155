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

//  Generic Command
//
//  dataOut[0] : Control
//    7 6 5 4 3 2 1 0
//    | | | | | | | +-- R/W
//    | | | | | | +---- START
//    | | | | | +------ RESTART
//    | | | | +-------- STOP
//    | | | +---------- I2CRESTART
//    | | +------------ CONFIG
//
#define     CTRL_RW             (0x01u)
#define     CTRL_START          (0x02u)
#define     CTRL_RESTART        (0x04u)
#define     CTRL_STOP           (0x08u)
#define     CTRL_I2CRESTART     (0x10u)
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

//  Length part
//
//  dataOut[1] : Long
//    7 6 5 4 3 2 1 0
//    | | +-+-+-+-+-+-- Long
//    | +-------------- Burst Repeat
//    +---------------- Burst Multiple
//

//  Command part
//
//  dataOut[2] : Command
//    7 6 5 4 3 2 1 0
//    | +-+-+-+-+-+-+-- Command code
//    +---------------- Internal command
//
#define     COM_VERSION         (0x81u)

//  Status code
//
//  dataIn[0] : Status
//    7 6 5 4 3 2 1 0
//    | | | | | | | +-- ACK
//    | | | | | | +---- N/A
//    | | | | | +------ VTARG

// Buffer for endpoints
uint8 dataIn[64] = {0};
uint8 dataOut[64] = {0};

// Return value for command
CYCODE const uint8 version[] = {0, 0, 0, 1, 0x01, 0x23, 0x00, 0xA5};

uint8 parseCommand(uint16 len) {
    uint16 i;
    uint8 dataInIndex = 0;
    
    LCD_Position(0, 0);
    for (i = 0; i < 8; i++) {
        LCD_PrintInt8(dataOut[i]);
    }
    LCD_PutChar(' ');
    
    if ((dataOut[0] == 0x02) && (dataOut[1] == 0x00) && (dataOut[2] == COM_VERSION)) {
        // Get version
        dataIn[dataInIndex++] = 5;
        for (i = 0; i < sizeof version; i++) {
            dataIn[dataInIndex++] = version[i];
        }
        while (!(USBFS_GetEPState(ENDPOINT_IN) & USBFS_IN_BUFFER_EMPTY));
        USBFS_LoadInEP(ENDPOINT_IN, dataIn, DATA_LEN);
    }
    return 0;
}

int main()
{
    uint8 counter = 0u;
    
    /* Enable Global Interrupts */
    CyGlobalIntEnable;
    
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
                if (parseCommand(len)) break;
            }
        }
    }
}

/* [] END OF FILE */
