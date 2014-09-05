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

#define CURSOR_STEP         (0x05u)     /* Step size */

uint8 dataIn[64] = {0};
uint8 dataOut[64] = {0};
uint8 bSNstring[16]={0x0Eu, 0x03u, '0', 0u, '0', 0u, '0', 0u, '0', 0u, '0', 0u, '0', 0u};

int main()
{
    uint8 counter = 0u;
    
    /* Enable Global Interrupts */
    CyGlobalIntEnable;
    
    /* Start USBFS device 0 with 3V operation */
    USBFS_Start(0u, USBFS_3V_OPERATION); 

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
            
            // INパケットの送出待ち
            if (USBFS_GetEPState(ENDPOINT_IN) & USBFS_IN_BUFFER_EMPTY) {        
                // INパケットを送り出す
                USBFS_LoadInEP(ENDPOINT_IN, dataIn, DATA_LEN);
            }
            
            // OUTパケットの到着待ち
            if (USBFS_GetEPState(ENDPOINT_OUT) & USBFS_OUT_BUFFER_FULL) {
                uint16 len = USBFS_GetEPCount(ENDPOINT_OUT);
                USBFS_ReadOutEP(ENDPOINT_OUT, dataOut, len);
            }
        }
    }
}

/* [] END OF FILE */
