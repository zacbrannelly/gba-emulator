#pragma once

#include <stdint.h>

// =====================
// General Memory Map
// =====================

static constexpr uint32_t BIOS_START = 0x0;
static constexpr uint32_t BIOS_END = 0x3FFFF;

// 256KB of EWRAM (on-board)
static constexpr uint32_t WORKING_RAM_ON_BOARD_START = 0x2000000;
static constexpr uint32_t WORKING_RAM_ON_BOARD_END = 0x203FFFF;

// 32KB of IWRAM (on-chip)
static constexpr uint32_t WORKING_RAM_ON_CHIP_START = 0x3000000;
static constexpr uint32_t WORKING_RAM_ON_CHIP_END = 0x3007FFF;

// I/O Registers (1kb)
static constexpr uint32_t IO_REGISTERS_START = 0x4000000;
static constexpr uint32_t IO_REGISTERS_END = 0x40003FE;

// BG/OBJ Palette RAM (1kb)
static constexpr uint32_t PALETTE_RAM_START = 0x5000000;
static constexpr uint32_t PALETTE_RAM_END = 0x50003FF;

// VRAM (96kb)
static constexpr uint32_t VRAM_START = 0x6000000;
static constexpr uint32_t VRAM_END = 0x6017FFF;

// OAM - Object Attribute Memory (1kb)
static constexpr uint32_t OAM_START = 0x7000000;
static constexpr uint32_t OAM_END = 0x70003FF;

// Game Pak ROM/FlashROM (max 32MB) - Wait State 0
static constexpr uint32_t GAME_PAK_ROM_START = 0x8000000;
static constexpr uint32_t GAME_PAK_ROM_END = 0x9FFFFFF;

// Game Pak ROM/FlashROM (max 32MB) - Wait State 1
static constexpr uint32_t GAME_PAK_ROM_WS1_START = 0xA000000;
static constexpr uint32_t GAME_PAK_ROM_WS1_END = 0xBFFFFFF;

// Game Pak ROM/FalshROM (max 32MB) - Wait State 2
static constexpr uint32_t GAME_PAK_ROM_WS2_START = 0xC000000;
static constexpr uint32_t GAME_PAK_ROM_WS2_END = 0xDFFFFFF;

// Game Pak SRAM (max 64kb)
static constexpr uint32_t GAME_PAK_SRAM_START = 0xE000000;
static constexpr uint32_t GAME_PAK_SRAM_END = 0xE00FFFF;

// =====================
// LCD I/O Registers
// =====================

// LCD Control Registers
static constexpr uint32_t REG_LCD_CONTROL = 0x4000000;    // DISPCNT - LCD Control
static constexpr uint32_t REG_GREEN_SWAP = 0x4000002;     // Undocumented - Green Swap
static constexpr uint32_t REG_LCD_STATUS = 0x4000004;     // DISPSTAT - General LCD Status
static constexpr uint32_t REG_VERTICAL_COUNT = 0x4000006; // VCOUNT - Vertical Counter

// Background Control Registers
static constexpr uint32_t REG_BG0_CONTROL = 0x4000008;  // BG0 Control
static constexpr uint32_t REG_BG1_CONTROL = 0x400000A;  // BG1 Control
static constexpr uint32_t REG_BG2_CONTROL = 0x400000C;  // BG2 Control
static constexpr uint32_t REG_BG3_CONTROL = 0x400000E;  // BG3 Control

// Background Offset Registers
static constexpr uint32_t REG_BG0_X_OFFSET = 0x4000010; // BG0 X-Offset
static constexpr uint32_t REG_BG0_Y_OFFSET = 0x4000012; // BG0 Y-Offset
static constexpr uint32_t REG_BG1_X_OFFSET = 0x4000014; // BG1 X-Offset
static constexpr uint32_t REG_BG1_Y_OFFSET = 0x4000016; // BG1 Y-Offset
static constexpr uint32_t REG_BG2_X_OFFSET = 0x4000018; // BG2 X-Offset
static constexpr uint32_t REG_BG2_Y_OFFSET = 0x400001A; // BG2 Y-Offset
static constexpr uint32_t REG_BG3_X_OFFSET = 0x400001C; // BG3 X-Offset
static constexpr uint32_t REG_BG3_Y_OFFSET = 0x400001E; // BG3 Y-Offset

// Background Rotation/Scaling Parameter Registers
static constexpr uint32_t REG_BG2_PARAM_A = 0x4000020;  // BG2PA - dx
static constexpr uint32_t REG_BG2_PARAM_B = 0x4000022;  // BG2PB - dmx
static constexpr uint32_t REG_BG2_PARAM_C = 0x4000024;  // BG2PC - dy
static constexpr uint32_t REG_BG2_PARAM_D = 0x4000026;  // BG2PD - dmy
static constexpr uint32_t REG_BG2_X_REF = 0x4000028;    // BG2X - X-Coordinate
static constexpr uint32_t REG_BG2_Y_REF = 0x400002C;    // BG2Y - Y-Coordinate
static constexpr uint32_t REG_BG3_PARAM_A = 0x4000030;  // BG3PA - dx
static constexpr uint32_t REG_BG3_PARAM_B = 0x4000032;  // BG3PB - dmx
static constexpr uint32_t REG_BG3_PARAM_C = 0x4000034;  // BG3PC - dy
static constexpr uint32_t REG_BG3_PARAM_D = 0x4000036;  // BG3PD - dmy
static constexpr uint32_t REG_BG3_X_REF = 0x4000038;    // BG3X - X-Coordinate
static constexpr uint32_t REG_BG3_Y_REF = 0x400003C;    // BG3Y - Y-Coordinate

// Window Dimension Registers
static constexpr uint32_t REG_WINDOW0_HORIZONTAL = 0x4000040; // WIN0H - Window 0 Horizontal Dimensions
static constexpr uint32_t REG_WINDOW1_HORIZONTAL = 0x4000042; // WIN1H - Window 1 Horizontal Dimensions
static constexpr uint32_t REG_WINDOW0_VERTICAL = 0x4000044;   // WIN0V - Window 0 Vertical Dimensions
static constexpr uint32_t REG_WINDOW1_VERTICAL = 0x4000046;   // WIN1V - Window 1 Vertical Dimensions

// Window Inside/Outside Registers
static constexpr uint32_t REG_WINDOW_INSIDE = 0x4000048;  // WININ - Inside of Window 0 and 1
static constexpr uint32_t REG_WINDOW_OUTSIDE = 0x400004A; // WINOUT - Inside of OBJ Window & Outside of Windows

// Mosaic Size Register
static constexpr uint32_t REG_MOSAIC_SIZE = 0x400004C; // MOSAIC - Mosaic Size

// =====================
// Direct Memory Access (DMA) Registers
// =====================

// DMA 0 Registers
static constexpr uint32_t REG_DMA0_SOURCE_ADDRESS = 0x40000B0;    // DMA0SAD - DMA 0 Source Address
static constexpr uint32_t REG_DMA0_DESTINATION_ADDRESS = 0x40000B4; // DMA0DAD - DMA 0 Destination Address
static constexpr uint32_t REG_DMA0_WORD_COUNT = 0x40000B8;        // DMA0CNT_L - DMA 0 Word Count
static constexpr uint32_t REG_DMA0_CONTROL = 0x40000BA;           // DMA0CNT_H - DMA 0 Control

// DMA 1 Registers
static constexpr uint32_t REG_DMA1_SOURCE_ADDRESS = 0x40000BC;    // DMA1SAD - DMA 1 Source Address
static constexpr uint32_t REG_DMA1_DESTINATION_ADDRESS = 0x40000C0; // DMA1DAD - DMA 1 Destination Address
static constexpr uint32_t REG_DMA1_WORD_COUNT = 0x40000C4;        // DMA1CNT_L - DMA 1 Word Count
static constexpr uint32_t REG_DMA1_CONTROL = 0x40000C6;           // DMA1CNT_H - DMA 1 Control

// DMA 2 Registers
static constexpr uint32_t REG_DMA2_SOURCE_ADDRESS = 0x40000C8;    // DMA2SAD - DMA 2 Source Address
static constexpr uint32_t REG_DMA2_DESTINATION_ADDRESS = 0x40000CC; // DMA2DAD - DMA 2 Destination Address
static constexpr uint32_t REG_DMA2_WORD_COUNT = 0x40000D0;        // DMA2CNT_L - DMA 2 Word Count
static constexpr uint32_t REG_DMA2_CONTROL = 0x40000D2;           // DMA2CNT_H - DMA 2 Control

// DMA 3 Registers
static constexpr uint32_t REG_DMA3_SOURCE_ADDRESS = 0x40000D4;    // DMA3SAD - DMA 3 Source Address
static constexpr uint32_t REG_DMA3_DESTINATION_ADDRESS = 0x40000D8; // DMA3DAD - DMA 3 Destination Address
static constexpr uint32_t REG_DMA3_WORD_COUNT = 0x40000DC;        // DMA3CNT_L - DMA 3 Word Count
static constexpr uint32_t REG_DMA3_CONTROL = 0x40000DE;           // DMA3CNT_H - DMA 3 Control

// =====================
// Timer Registers
// =====================

// Timer 0 Registers
static constexpr uint32_t REG_TIMER0_COUNTER_RELOAD = 0x4000100; // TM0CNT_L - Timer 0 Counter/Reload
static constexpr uint32_t REG_TIMER0_CONTROL = 0x4000102;        // TM0CNT_H - Timer 0 Control

// Timer 1 Registers
static constexpr uint32_t REG_TIMER1_COUNTER_RELOAD = 0x4000104; // TM1CNT_L - Timer 1 Counter/Reload
static constexpr uint32_t REG_TIMER1_CONTROL = 0x4000106;        // TM1CNT_H - Timer 1 Control

// Timer 2 Registers
static constexpr uint32_t REG_TIMER2_COUNTER_RELOAD = 0x4000108; // TM2CNT_L - Timer 2 Counter/Reload
static constexpr uint32_t REG_TIMER2_CONTROL = 0x400010A;        // TM2CNT_H - Timer 2 Control

// Timer 3 Registers
static constexpr uint32_t REG_TIMER3_COUNTER_RELOAD = 0x400010C; // TM3CNT_L - Timer 3 Counter/Reload
static constexpr uint32_t REG_TIMER3_CONTROL = 0x400010E;        // TM3CNT_H - Timer 3 Control

// =====================
// Keypad Registers
// =====================

static constexpr uint32_t REG_KEY_STATUS = 0x4000130;            // KEYINPUT - Key Status
static constexpr uint32_t REG_KEY_INTERRUPT_CONTROL = 0x4000132; // KEYCNT - Key Interrupt Control

// =====================
// Interrupt Registers
// =====================

// Interrupt and Wait State Control Registers
static constexpr uint32_t REG_INTERRUPT_ENABLE = 0x4000200;          // IE - Interrupt Enable Register
static constexpr uint32_t REG_INTERRUPT_REQUEST_FLAGS = 0x4000202;   // IF - Interrupt Request Flags / IRQ Acknowledge
static constexpr uint32_t REG_WAIT_STATE_CONTROL = 0x4000204;        // WAITCNT - Game Pak Waitstate Control
static constexpr uint32_t REG_INTERRUPT_MASTER_ENABLE = 0x4000208;   // IME - Interrupt Master Enable Register
