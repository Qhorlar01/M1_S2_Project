#include "F28x_Project.h"
#include "math.h"

// Define constants
#define EPWM_TIMER_TBPRD 1215 // 10kHz PWM Period register
#define PI 3.14159265358979323846 // Define PI constant
#define SAMPLES 205 // Number of samples for sine wave

// Function prototypes
void InitEPwm1Example(void);
void InitEPwm2Example(void);
void InitEPwm3Example(void);

__interrupt void epwm1_isr(void);
__interrupt void epwm2_isr(void);
__interrupt void epwm3_isr(void);

unsigned int i = 0, j = 0, k = 0; // Sample indices for each ePWM
float sine_wave[SAMPLES]; // Array to hold sine wave samples

// Main function
void main(void)
{
    int m; // Declare variable outside of the loop
    InitSysCtrl(); // Initialize system control

    // Generate sine lookup table for one period of a 50Hz sine wave
    for (m = 0; m < SAMPLES; m++) {
        float angle = 2 * PI * m / SAMPLES; // Calculate angle for each sample
        sine_wave[m] = sin(angle); // Store sine value in array
    }

    EALLOW;
    // Disable pull-up on GPIOs and configure as ePWM outputs
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1; // Disable pull-up for GPIO0 (ePWM1A)
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 1; // Disable pull-up for GPIO2 (ePWM2A)
    GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1; // Disable pull-up for GPIO4 (ePWM3A)

    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1; // Configure GPIO0 as ePWM1A
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1; // Configure GPIO2 as ePWM2A
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1; // Configure GPIO4 as ePWM3A
    // Configure additional GPIOs for phase-shifted sine wave output
    GpioCtrlRegs.GPAPUD.bit.GPIO6 = 1; // Disable pull-up for GPIO6
    GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 0; // Configure GPIO6 as GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO6 = 1; // Set GPIO6 as output

    GpioCtrlRegs.GPAPUD.bit.GPIO7 = 1; // Disable pull-up for GPIO7
    GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0; // Configure GPIO7 as GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO7 = 1; // Set GPIO7 as output

    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 1; // Disable pull-up for GPIO8
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0; // Configure GPIO8 as GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 1; // Set GPIO8 as output

    EDIS;

    DINT; // Disable CPU interrupts
    InitPieCtrl(); // Initialize PIE control registers
    IER = 0x0000; // Clear CPU interrupt register
    IFR = 0x0000; // Clear CPU interrupt flag register
    InitPieVectTable(); // Initialize PIE vector table

    EALLOW;
    // Map ISR functions to PIE vector table
    PieVectTable.EPWM1_INT = &epwm1_isr;
    PieVectTable.EPWM2_INT = &epwm2_isr;
    PieVectTable.EPWM3_INT = &epwm3_isr;
    EDIS;

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0; // Stop all TB clocks
    EDIS;

    // Initialize ePWM modules
    InitEPwm1Example();
    InitEPwm2Example();
    InitEPwm3Example();

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1; // Start all TB clocks
    EDIS;

    // Enable CPU interrupts for ePWM modules
    IER |= M_INT3;
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1; // Enable PIE interrupt for ePWM1
    PieCtrlRegs.PIEIER3.bit.INTx2 = 1; // Enable PIE interrupt for ePWM2
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1; // Enable PIE interrupt for ePWM3

    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

    for (;;)
    {
        asm (" NOP"); // Infinite loop
    }
}

// ISR for ePWM1
__interrupt void epwm1_isr(void)
{
    float value = sine_wave[i]; // Get current sine wave sample
    EPwm1Regs.CMPA.bit.CMPA = (value >= 0 ? value : -value) * EPWM_TIMER_TBPRD; // Set CMPA value based on sine sample
    if (value >= 0) {
        EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR; // Set action qualifier to clear on up-count
        EPwm1Regs.AQCTLA.bit.CAD = AQ_SET; // Set action qualifier to set on down-count
    } else {
        EPwm1Regs.AQCTLA.bit.CAU = AQ_SET; // Set action qualifier to set on up-count
        EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR; // Set action qualifier to clear on down-count
    }

    // Output phase-shifted sine wave sample on GPIO6
    if (value >= 0) {
        GpioDataRegs.GPASET.bit.GPIO6 = 1;
    } else {
        GpioDataRegs.GPACLEAR.bit.GPIO6 = 1;
    }

    i = (i + 1) % SAMPLES; // Increment and wrap sample index
    EPwm1Regs.ETCLR.bit.INT = 1; // Clear interrupt flag
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3; // Acknowledge PIE interrupt
}

// ISR for ePWM2
__interrupt void epwm2_isr(void)
{
    float value = sine_wave[(j + SAMPLES / 3) % SAMPLES]; // Get 120-degree phase-shifted sine wave sample
    EPwm2Regs.CMPA.bit.CMPA = (value >= 0 ? value : -value) * EPWM_TIMER_TBPRD; // Set CMPA value based on sine sample
    if (value >= 0) {
        EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR; // Set action qualifier to clear on up-count
        EPwm2Regs.AQCTLA.bit.CAD = AQ_SET; // Set action qualifier to set on down-count
    } else {
        EPwm2Regs.AQCTLA.bit.CAU = AQ_SET; // Set action qualifier to set on up-count
        EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR; // Set action qualifier to clear on down-count
    }

    // Output phase-shifted sine wave sample on GPIO7
    if (value >= 0) {
        GpioDataRegs.GPASET.bit.GPIO7 = 1;
    } else {
        GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;
    }

    j = (j + 1) % SAMPLES; // Increment and wrap sample index
    EPwm2Regs.ETCLR.bit.INT = 1; // Clear interrupt flag
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3; // Acknowledge PIE interrupt
}

// ISR for ePWM3
__interrupt void epwm3_isr(void)
{
    float value = sine_wave[(k + 2 * SAMPLES / 3) % SAMPLES]; // Get 240-degree phase-shifted sine wave sample
    EPwm3Regs.CMPA.bit.CMPA = (value >= 0 ? value : -value) * EPWM_TIMER_TBPRD; // Set CMPA value based on sine sample
    if (value >= 0) {
        EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR; // Set action qualifier to clear on up-count
        EPwm3Regs.AQCTLA.bit.CAD = AQ_SET; // Set action qualifier to set on down-count
    } else {
        EPwm3Regs.AQCTLA.bit.CAU = AQ_SET; // Set action qualifier to set on up-count
        EPwm3Regs.AQCTLA.bit.CAD = AQ_CLEAR; // Set action qualifier to clear on down-count
    }

    // Output phase-shifted sine wave sample on GPIO8
    if (value >= 0) {
        GpioDataRegs.GPASET.bit.GPIO8 = 1;
    } else {
        GpioDataRegs.GPACLEAR.bit.GPIO8 = 1;
    }

    k = (k + 1) % SAMPLES; // Increment and wrap sample index
    EPwm3Regs.ETCLR.bit.INT = 1; // Clear interrupt flag
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3; // Acknowledge PIE interrupt
}

// Initialize ePWM1
void InitEPwm1Example()
{
    ClkCfgRegs.PERCLKDIVSEL.bit.EPWMCLKDIV=0;
    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO; // Select interrupt event on counter zero
    EPwm1Regs.ETSEL.bit.INTEN = 1; // Enable interrupt
    EPwm1Regs.ETPS.bit.INTPRD = ET_1ST; // Generate interrupt on first event
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN; // Set up-down count mode
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Disable phase loading
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1; // Set high-speed time-base clock prescaler
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV2; // Set time-base clock prescaler
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_IMMEDIATE; // Set shadow mode for CMPA
    EPwm1Regs.CMPA.bit.CMPA = 0; // Initialize CMPA register
    EPwm1Regs.TBPRD = EPWM_TIMER_TBPRD; // Set timer period
    EPwm1Regs.TBPHS.bit.TBPHS = 0x0000; // Initialize phase register
    EPwm1Regs.TBCTR = 0x0000; // Clear counter
}

// Initialize ePWM2
void InitEPwm2Example()
{
    EPwm2Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO; // Select interrupt event on counter zero
    EPwm2Regs.ETSEL.bit.INTEN = 1; // Enable interrupt
    EPwm2Regs.ETPS.bit.INTPRD = ET_1ST; // Generate interrupt on first event
    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN; // Set up-down count mode
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Disable phase loading
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1; // Set high-speed time-base clock prescaler
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV2; // Set time-base clock prescaler
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_IMMEDIATE; // Set shadow mode for CMPA
    EPwm2Regs.CMPA.bit.CMPA = 0; // Initialize CMPA register
    EPwm2Regs.TBPRD = EPWM_TIMER_TBPRD; // Set timer period
    EPwm2Regs.TBPHS.bit.TBPHS = 0x0000; // Initialize phase register
    EPwm2Regs.TBCTR = 0x0000; // Clear counter
}

// Initialize ePWM3
void InitEPwm3Example()
{
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO; // Select interrupt event on counter zero
    EPwm3Regs.ETSEL.bit.INTEN = 1; // Enable interrupt
    EPwm3Regs.ETPS.bit.INTPRD = ET_1ST; // Generate interrupt on first event
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN; // Set up-down count mode
    EPwm3Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Disable phase loading
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1; // Set high-speed time-base clock prescaler
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV2; // Set time-base clock prescaler
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_IMMEDIATE; // Set shadow mode for CMPA
    EPwm3Regs.CMPA.bit.CMPA = 0; // Initialize CMPA register
    EPwm3Regs.TBPRD = EPWM_TIMER_TBPRD; // Set timer period
    EPwm3Regs.TBPHS.bit.TBPHS = 0x0000; // Initialize phase register
    EPwm3Regs.TBCTR = 0x0000; // Clear counter
}

