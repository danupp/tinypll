#define anti_backlash 0x0000 // 2.9 ns
#define LD_precision_bit 0x0010


// Remember Ndiv max = P^2-P and f/P = 200 MHz maz (ADF4113)

#if FREQ==1152  // alt 1154

#if XTAL==24
#define R_div 2    // 24 -> 12 MHz
#define A_div 0
#define B_div 12   // (12*8+0)*12 = 1152

#define funct_latch_1 0x0000  // prescaler 8, 1.25mA
#define funct_latch_2 0x8092  // pos pd pol, dig lock-detect

#define R_div_alt 12    // 24 -> 2 MHz
#define A_div_alt 1
#define B_div_alt 72   // (72*8+1)*2 = 1154

#define funct_latch_1_alt 0x0003  // prescaler 8, 5mA
#define funct_latch_2_alt 0x8092  // pos pd pol, dig lock-detect

#elif XTAL==36
#define R_div 2    // 36 -> 18 MHz
#define A_div 0
#define B_div 8   // (8*8+0)*18 = 1152

#define funct_latch_1 0x0000  // prescaler 8, 1.25mA
#define funct_latch_2 0x8092  // pos pd pol, dig lock-detect

#define R_div_alt 18    // 36 -> 2 MHz
#define A_div_alt 1
#define B_div_alt 72   // (72*8+1)*2 = 1154

#define funct_latch_1_alt 0x0003  // prescaler 8, 5mA
#define funct_latch_2_alt 0x8092  // pos pd pol, dig lock-detect
#endif

/////////////////

#elif FREQ==2556  // alt 2586

#if XTAL==24
#define R_div 4    // 24 -> 6 MHz
#define A_div 10
#define B_div 26   // (26*16+10)*6 = 2556

#define funct_latch_1 0x0040  // prescaler 16, 1.25mA
#define funct_latch_2 0x8092  // pos pd pol, dig lock-detect

#define R_div_alt 4    // 24 -> 6 MHz
#define A_div_alt 15
#define B_div_alt 26   // (26*16+15)*6 = 2586

#define funct_latch_1_alt 0x0043  // prescaler 16, 5mA
#define funct_latch_2_alt 0x8092  // pos pd pol, dig lock-detect

#elif XTAL==36
#define R_div 6    // 36 -> 6 MHz
#define A_div 10
#define B_div 26   // (26*16+10)*6 = 2556

#define funct_latch_1 0x0040  // prescaler 16, 1.25mA
#define funct_latch_2 0x8092  // pos pd pol, dig lock-detect

#define R_div_alt 6    // 36 -> 6 MHz
#define A_div_alt 15
#define B_div_alt 26   // (26*16+15)*6 = 2586

#define funct_latch_1_alt 0x0043  // prescaler 16, 5mA
#define funct_latch_2_alt 0x8092  // pos pd pol, dig lock-detect
#endif

////////////////

#elif FREQ==2808  // no alt

#if XTAL==24
#define R_div 3    // 24 -> 8 MHz
#define A_div 15
#define B_div 21   // (21*16+15)*8 = 2808

#define funct_latch_1 0x0043  // prescaler 16, 5mA
#define funct_latch_2 0x8092  // pos pd pol, dig lock-detect

#define R_div_alt 3    // 24 -> 8 MHz
#define A_div_alt 15
#define B_div_alt 21   // (21*16+15)*8 = 2808

#define funct_latch_1_alt 0x0043  // prescaler 16, 5mA
#define funct_latch_2_alt 0x8092  // pos pd pol, dig lock-detect

#elif XTAL==36
#define R_div 6    // 36 -> 6 MHz
#define A_div 4
#define B_div 29   // (29*16+4)*6 = 2808

#define funct_latch_1 0x0044  // prescaler 16, 5mA
#define funct_latch_2 0x8092  // pos pd pol, dig lock-detect

#define R_div_alt 6    // 36 -> 6 MHz
#define A_div_alt 4
#define B_div_alt 29   // (29*16+4)*6 = 2808

#define funct_latch_1_alt 0x0041  // prescaler 8, 1.25mA
#define funct_latch_2_alt 0x8092  // pos pd pol, dig lock-detect
#endif

#endif


/////////////////

#define funct_latch_2_test_neg 0x8042 // neg pd pol, r div out
#define funct_latch_2_test_pos 0x80c2 // pos pd pol, r div out
