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

    /* Wait for Device to enumerate */
    while(!USBFS_GetConfiguration());

    /* Enumeration is done, load endpoint 1. Do not toggle the first time. */
    USBFS_LoadInEP(ENDPOINT_IN, dataIn, DATA_LEN);
    
    while(1)
    {
        /* Wait for ACK before loading data */
        while(!USBFS_GetEPAckState(ENDPOINT_IN));
        
        /* ACK has occurred, load the endpoint and toggle the data bit */
        USBFS_LoadInEP(ENDPOINT_IN, dataIn, DATA_LEN);
        
        counter++;
    }
}

/* [] END OF FILE */
