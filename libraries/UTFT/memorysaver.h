// UTFT Memory Saver
// -----------------
//
// Since most people have only one or possibly two different display modules a lot
// of memory has been wasted to keep support for many unneeded controller chips.
// You now have the option to remove this unneeded code from the library with
// this file.
// By disabling the controllers you don't need you can reduce the memory footprint
// of the library by several Kb.
//
// Uncomment the lines for the displaycontrollers that you don't use to save
// some flash memory by not including the init code for that particular
// controller.

//#define DISABLE_CPLD		 		1	// EHOUSE50CPLD

//#define DISABLE_HX8340B_8 		1	// ITDB22 8bit mode / GEEE22
//#define DISABLE_HX8340B_S 		1	// ITDB22 Serial mode
//#define DISABLE_HX8347A			1	// ITDB32
//#define DISABLE_HX8352A			1	// ITDB32WD / TFT01_32WD / CTE32W
//#define DISABLE_HX8353C           1	// DMTFT18101

//#define DISABLE_ILI9320			1	// GEEE24 / GEEE28	- This single define will disable both 8bit and 16bit mode for this controller
//#define DISABLE_ILI9325C  		1	// ITDB24
//#define DISABLE_ILI9325D  		1	// ITDB24D / ITDB24DWOT / ITDB28 / TFT01_24_8 / TFT01_24_16 / DMTFT24104 / DMTFT28103	- This single define will disable both 8bit and 16bit mode for this controller
//#define DISABLE_ILI9325D_ALT 		1	// CTE28
//#define DISABLE_ILI9327			1	// ITDB32WC / TFT01_32W
//#define DISABLE_ILI9341_S4P		1	// MI0283QT9
//#define DISABLE_ILI9341_S5P		1	// TFT01_22SP / DMTFT28105
//#define DISABLE_ILI9481			1	// CTE32HR
//#define DISABLE_ILI9486			1	// CTE40

//#define DISABLE_PCF8833			1	// LPH9135

//#define DISABLE_R61581			1	// CTE35IPS

//#define DISABLE_S1D19122  		1	// ITDB25H
//#define DISABLE_S6D0164			1	// CTE22 / DMTFT22102
//#define DISABLE_S6D1121			1	// ITDB24E	- This single define will disable both 8bit and 16bit mode for this controller
//#define DISABLE_SSD1289			1	// ITDB32S / TFT01_32 / GEEE32 / ELEE32_REVA / ELEE32_REVB / CTE32	- This single define will disable both 8bit, 16bit and latched mode for this controller
//#define DISABLE_SSD1963_480		1	// ITDB43 / TFT01_43
//#define DISABLE_SSD1963_800		1	// ITDB50 / TFT01_50 / CTE50 / EHOUSE50
//#define DISABLE_SSD1963_800_ALT	1	// TFT01_70 / CTE70 / EHOUSE70
//#define DISABLE_ST7735			1	// ITDB18SP
//#define DISABLE_ST7735S			1	// TFT01_18SP
