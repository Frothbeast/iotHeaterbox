#pragma config OSC = HS
#pragma config WDT = OFF
#pragma config PWRT = ON
#pragma config BOREN = BOHW
#pragma config LVP = OFF
#pragma config PBADEN = OFF


#include <xc.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "iotHeaterbox.h"

 const int16_t heater_temp_lut[1024] = {
    179, 179, 179, 179, 179, 179, 215, 247, 276, 301, 324, 345, 365, 383, 400, 417,
    432, 446, 460, 473, 486, 498, 510, 521, 532, 542, 552, 562, 572, 581, 590, 599,
    607, 616, 624, 632, 639, 647, 654, 662, 669, 676, 683, 689, 696, 703, 709, 715,
    721, 727, 733, 739, 745, 751, 756, 762, 767, 773, 778, 783, 788, 793, 799, 804,
    808, 813, 818, 823, 828, 832, 837, 841, 846, 850, 855, 859, 863, 868, 872, 876,
    880, 884, 889, 893, 897, 901, 905, 908, 912, 916, 920, 924, 928, 931, 935, 939,
    942, 946, 950, 953, 957, 960, 964, 967, 971, 974, 977, 981, 984, 988, 991, 994,
    997, 1001, 1004, 1007, 1010, 1014, 1017, 1020, 1023, 1026, 1029, 1032, 1035, 1038, 1041, 1044,
    1047, 1050, 1053, 1056, 1059, 1062, 1065, 1068, 1071, 1074, 1077, 1079, 1082, 1085, 1088, 1091,
    1093, 1096, 1099, 1102, 1104, 1107, 1110, 1113, 1115, 1118, 1121, 1123, 1126, 1128, 1131, 1134,
    1136, 1139, 1141, 1144, 1147, 1149, 1152, 1154, 1157, 1159, 1162, 1164, 1167, 1169, 1172, 1174,
    1177, 1179, 1182, 1184, 1186, 1189, 1191, 1194, 1196, 1198, 1201, 1203, 1206, 1208, 1210, 1213,
    1215, 1217, 1220, 1222, 1224, 1227, 1229, 1231, 1234, 1236, 1238, 1240, 1243, 1245, 1247, 1249,
    1252, 1254, 1256, 1258, 1261, 1263, 1265, 1267, 1269, 1272, 1274, 1276, 1278, 1280, 1283, 1285,
    1287, 1289, 1291, 1293, 1296, 1298, 1300, 1302, 1304, 1306, 1308, 1310, 1313, 1315, 1317, 1319,
    1321, 1323, 1325, 1327, 1329, 1331, 1333, 1336, 1338, 1340, 1342, 1344, 1346, 1348, 1350, 1352,
    1354, 1356, 1358, 1360, 1362, 1364, 1366, 1368, 1370, 1372, 1374, 1376, 1378, 1380, 1382, 1384,
    1386, 1388, 1390, 1392, 1394, 1396, 1398, 1400, 1402, 1404, 1406, 1408, 1410, 1412, 1414, 1416,
    1418, 1420, 1422, 1424, 1426, 1427, 1429, 1431, 1433, 1435, 1437, 1439, 1441, 1443, 1445, 1447,
    1449, 1451, 1453, 1454, 1456, 1458, 1460, 1462, 1464, 1466, 1468, 1470, 1472, 1473, 1475, 1477,
    1479, 1481, 1483, 1485, 1487, 1489, 1490, 1492, 1494, 1496, 1498, 1500, 1502, 1504, 1505, 1507,
    1509, 1511, 1513, 1515, 1517, 1518, 1520, 1522, 1524, 1526, 1528, 1530, 1531, 1533, 1535, 1537,
    1539, 1541, 1543, 1544, 1546, 1548, 1550, 1552, 1554, 1556, 1557, 1559, 1561, 1563, 1565, 1567,
    1568, 1570, 1572, 1574, 1576, 1578, 1579, 1581, 1583, 1585, 1587, 1589, 1590, 1592, 1594, 1596,
    1598, 1600, 1601, 1603, 1605, 1607, 1609, 1611, 1612, 1614, 1616, 1618, 1620, 1622, 1623, 1625,
    1627, 1629, 1631, 1633, 1634, 1636, 1638, 1640, 1642, 1643, 1645, 1647, 1649, 1651, 1653, 1654,
    1656, 1658, 1660, 1662, 1664, 1665, 1667, 1669, 1671, 1673, 1675, 1676, 1678, 1680, 1682, 1684,
    1686, 1687, 1689, 1691, 1693, 1695, 1697, 1698, 1700, 1702, 1704, 1706, 1708, 1709, 1711, 1713,
    1715, 1717, 1719, 1720, 1722, 1724, 1726, 1728, 1730, 1732, 1733, 1735, 1737, 1739, 1741, 1743,
    1744, 1746, 1748, 1750, 1752, 1754, 1756, 1757, 1759, 1761, 1763, 1765, 1767, 1769, 1770, 1772,
    1774, 1776, 1778, 1780, 1782, 1784, 1785, 1787, 1789, 1791, 1793, 1795, 1797, 1799, 1800, 1802,
    1804, 1806, 1808, 1810, 1812, 1814, 1816, 1817, 1819, 1821, 1823, 1825, 1827, 1829, 1831, 1833,
    1835, 1837, 1838, 1840, 1842, 1844, 1846, 1848, 1850, 1852, 1854, 1856, 1858, 1860, 1862, 1863,
    1865, 1867, 1869, 1871, 1873, 1875, 1877, 1879, 1881, 1883, 1885, 1887, 1889, 1891, 1893, 1895,
    1897, 1899, 1901, 1903, 1905, 1907, 1909, 1911, 1913, 1914, 1916, 1918, 1920, 1922, 1924, 1926,
    1928, 1930, 1933, 1935, 1937, 1939, 1941, 1943, 1945, 1947, 1949, 1951, 1953, 1955, 1957, 1959,
    1961, 1963, 1965, 1967, 1969, 1971, 1973, 1975, 1977, 1980, 1982, 1984, 1986, 1988, 1990, 1992,
    1994, 1996, 1998, 2000, 2003, 2005, 2007, 2009, 2011, 2013, 2015, 2017, 2020, 2022, 2024, 2026,
    2028, 2030, 2032, 2035, 2037, 2039, 2041, 2043, 2045, 2048, 2050, 2052, 2054, 2056, 2059, 2061,
    2063, 2065, 2067, 2070, 2072, 2074, 2076, 2079, 2081, 2083, 2085, 2088, 2090, 2092, 2094, 2097,
    2099, 2101, 2104, 2106, 2108, 2110, 2113, 2115, 2117, 2120, 2122, 2124, 2127, 2129, 2131, 2134,
    2136, 2138, 2141, 2143, 2146, 2148, 2150, 2153, 2155, 2158, 2160, 2162, 2165, 2167, 2170, 2172,
    2175, 2177, 2179, 2182, 2184, 2187, 2189, 2192, 2194, 2197, 2199, 2202, 2204, 2207, 2209, 2212,
    2214, 2217, 2220, 2222, 2225, 2227, 2230, 2232, 2235, 2238, 2240, 2243, 2245, 2248, 2251, 2253,
    2256, 2259, 2261, 2264, 2267, 2269, 2272, 2275, 2278, 2280, 2283, 2286, 2288, 2291, 2294, 2297,
    2300, 2302, 2305, 2308, 2311, 2314, 2316, 2319, 2322, 2325, 2328, 2331, 2334, 2336, 2339, 2342,
    2345, 2348, 2351, 2354, 2357, 2360, 2363, 2366, 2369, 2372, 2375, 2378, 2381, 2384, 2387, 2390,
    2393, 2396, 2399, 2403, 2406, 2409, 2412, 2415, 2418, 2422, 2425, 2428, 2431, 2434, 2438, 2441,
    2444, 2447, 2451, 2454, 2457, 2461, 2464, 2467, 2471, 2474, 2478, 2481, 2484, 2488, 2491, 2495,
    2498, 2502, 2505, 2509, 2512, 2516, 2520, 2523, 2527, 2530, 2534, 2538, 2541, 2545, 2549, 2553,
    2556, 2560, 2564, 2568, 2572, 2575, 2579, 2583, 2587, 2591, 2595, 2599, 2603, 2607, 2611, 2615,
    2619, 2623, 2627, 2631, 2635, 2639, 2644, 2648, 2652, 2656, 2661, 2665, 2669, 2674, 2678, 2682,
    2687, 2691, 2696, 2700, 2705, 2709, 2714, 2718, 2723, 2728, 2732, 2737, 2742, 2747, 2751, 2756,
    2761, 2766, 2771, 2776, 2781, 2786, 2791, 2796, 2801, 2806, 2811, 2817, 2822, 2827, 2832, 2838,
    2843, 2849, 2854, 2860, 2865, 2871, 2876, 2882, 2888, 2894, 2899, 2905, 2911, 2917, 2923, 2929,
    2935, 2941, 2948, 2954, 2960, 2967, 2973, 2979, 2986, 2992, 2999, 3006, 3013, 3019, 3026, 3033,
    3040, 3047, 3054, 3061, 3069, 3076, 3083, 3091, 3098, 3106, 3114, 3121, 3129, 3137, 3145, 3153,
    3162, 3170, 3178, 3187, 3195, 3204, 3213, 3221, 3230, 3239, 3248, 3258, 3267, 3277, 3286, 3296,
    3306, 3316, 3326, 3336, 3346, 3357, 3368, 3378, 3389, 3401, 3412, 3423, 3435, 3447, 3459, 3471,
    3483, 3496, 3508, 3521, 3534, 3548, 3561, 3575, 3589, 3603, 3618, 3633, 3648, 3663, 3679, 3695,
    3711, 3728, 3745, 3762, 3780, 3798, 3816, 3835, 3854, 3874, 3894, 3915, 3936, 3958, 3980, 4003,
    4027, 4051, 4076, 4102, 4128, 4155, 4183, 4212, 4242, 4273, 4305, 4338, 4372, 4408, 4445, 4484,
    4524, 4566, 4610, 4656, 4704, 4754, 4807, 4863, 4923, 4986, 5053, 5124, 5200, 5282, 5371, 5467,
    5571, 5686, 5813, 5953, 6112, 6292, 6500, 6743, 7035, 7394, 7855, 8478, 9398, 9398, 9398, 9398,
};
const int16_t box_temp_lut[1024] = {
    2335, 2335, 2335, 2335, 2335, 2335, 2229, 2142, 2070, 2008, 1954, 1906, 1862, 1823, 1788, 1755,
    1725, 1697, 1671, 1647, 1624, 1602, 1582, 1562, 1544, 1527, 1510, 1494, 1479, 1464, 1450, 1436,
    1423, 1411, 1399, 1387, 1376, 1365, 1354, 1344, 1334, 1324, 1314, 1305, 1296, 1287, 1279, 1271,
    1262, 1254, 1247, 1239, 1232, 1225, 1217, 1211, 1204, 1197, 1191, 1184, 1178, 1172, 1166, 1160,
    1154, 1148, 1143, 1137, 1132, 1126, 1121, 1116, 1111, 1106, 1101, 1096, 1091, 1086, 1082, 1077,
    1072, 1068, 1063, 1059, 1055, 1051, 1046, 1042, 1038, 1034, 1030, 1026, 1022, 1018, 1015, 1011,
    1007, 1003, 1000, 996, 993, 989, 986, 982, 979, 975, 972, 969, 966, 962, 959, 956,
    953, 950, 947, 944, 941, 938, 935, 932, 929, 926, 923, 920, 917, 914, 912, 909,
    906, 904, 901, 898, 896, 893, 890, 888, 885, 883, 880, 878, 875, 873, 870, 868,
    865, 863, 861, 858, 856, 854, 851, 849, 847, 844, 842, 840, 838, 835, 833, 831,
    829, 827, 825, 823, 820, 818, 816, 814, 812, 810, 808, 806, 804, 802, 800, 798,
    796, 794, 792, 790, 788, 786, 784, 783, 781, 779, 777, 775, 773, 771, 770, 768,
    766, 764, 762, 761, 759, 757, 755, 754, 752, 750, 748, 747, 745, 743, 742, 740,
    738, 737, 735, 733, 732, 730, 728, 727, 725, 723, 722, 720, 719, 717, 716, 714,
    712, 711, 709, 708, 706, 705, 703, 702, 700, 699, 697, 696, 694, 693, 691, 690,
    688, 687, 685, 684, 682, 681, 680, 678, 677, 675, 674, 672, 671, 670, 668, 667,
    665, 664, 663, 661, 660, 659, 657, 656, 655, 653, 652, 651, 649, 648, 647, 645,
    644, 643, 641, 640, 639, 637, 636, 635, 634, 632, 631, 630, 628, 627, 626, 625,
    623, 622, 621, 620, 618, 617, 616, 615, 614, 612, 611, 610, 609, 607, 606, 605,
    604, 603, 601, 600, 599, 598, 597, 596, 594, 593, 592, 591, 590, 589, 587, 586,
    585, 584, 583, 582, 581, 579, 578, 577, 576, 575, 574, 573, 572, 570, 569, 568,
    567, 566, 565, 564, 563, 562, 560, 559, 558, 557, 556, 555, 554, 553, 552, 551,
    550, 549, 547, 546, 545, 544, 543, 542, 541, 540, 539, 538, 537, 536, 535, 534,
    533, 532, 531, 530, 529, 528, 527, 526, 524, 523, 522, 521, 520, 519, 518, 517,
    516, 515, 514, 513, 512, 511, 510, 509, 508, 507, 506, 505, 504, 503, 502, 501,
    500, 499, 498, 497, 496, 495, 494, 493, 493, 492, 491, 490, 489, 488, 487, 486,
    485, 484, 483, 482, 481, 480, 479, 478, 477, 476, 475, 474, 473, 472, 471, 470,
    470, 469, 468, 467, 466, 465, 464, 463, 462, 461, 460, 459, 458, 457, 456, 455,
    455, 454, 453, 452, 451, 450, 449, 448, 447, 446, 445, 444, 444, 443, 442, 441,
    440, 439, 438, 437, 436, 435, 434, 433, 433, 432, 431, 430, 429, 428, 427, 426,
    425, 424, 424, 423, 422, 421, 420, 419, 418, 417, 416, 416, 415, 414, 413, 412,
    411, 410, 409, 408, 408, 407, 406, 405, 404, 403, 402, 401, 400, 400, 399, 398,
    397, 396, 395, 394, 393, 393, 392, 391, 390, 389, 388, 387, 386, 386, 385, 384,
    383, 382, 381, 380, 379, 379, 378, 377, 376, 375, 374, 373, 372, 372, 371, 370,
    369, 368, 367, 366, 366, 365, 364, 363, 362, 361, 360, 359, 359, 358, 357, 356,
    355, 354, 353, 353, 352, 351, 350, 349, 348, 347, 347, 346, 345, 344, 343, 342,
    341, 340, 340, 339, 338, 337, 336, 335, 334, 334, 333, 332, 331, 330, 329, 328,
    328, 327, 326, 325, 324, 323, 322, 321, 321, 320, 319, 318, 317, 316, 315, 315,
    314, 313, 312, 311, 310, 309, 309, 308, 307, 306, 305, 304, 303, 302, 302, 301,
    300, 299, 298, 297, 296, 296, 295, 294, 293, 292, 291, 290, 289, 289, 288, 287,
    286, 285, 284, 283, 282, 282, 281, 280, 279, 278, 277, 276, 275, 275, 274, 273,
    272, 271, 270, 269, 268, 267, 267, 266, 265, 264, 263, 262, 261, 260, 260, 259,
    258, 257, 256, 255, 254, 253, 252, 251, 251, 250, 249, 248, 247, 246, 245, 244,
    243, 242, 242, 241, 240, 239, 238, 237, 236, 235, 234, 233, 232, 232, 231, 230,
    229, 228, 227, 226, 225, 224, 223, 222, 221, 221, 220, 219, 218, 217, 216, 215,
    214, 213, 212, 211, 210, 209, 208, 207, 206, 206, 205, 204, 203, 202, 201, 200,
    199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184,
    183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168,
    167, 166, 165, 164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152,
    151, 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 139, 138, 137, 136, 135,
    134, 133, 132, 131, 130, 129, 128, 126, 125, 124, 123, 122, 121, 120, 119, 118,
    116, 115, 114, 113, 112, 111, 110, 108, 107, 106, 105, 104, 103, 101, 100, 99,
    98, 97, 96, 94, 93, 92, 91, 89, 88, 87, 86, 85, 83, 82, 81, 80,
    78, 77, 76, 75, 73, 72, 71, 70, 68, 67, 66, 64, 63, 62, 60, 59,
    58, 56, 55, 54, 52, 51, 50, 48, 47, 46, 44, 43, 41, 40, 39, 37,
    36, 34, 33, 31, 30, 28, 27, 26, 24, 23, 21, 20, 18, 16, 15, 13,
    12, 10, 9, 7, 6, 4, 2, 1, -1, -3, -4, -6, -7, -9, -11, -13,
    -14, -16, -18, -19, -21, -23, -25, -27, -28, -30, -32, -34, -36, -38, -39, -41,
    -43, -45, -47, -49, -51, -53, -55, -57, -59, -61, -63, -66, -68, -70, -72, -74,
    -76, -79, -81, -83, -85, -88, -90, -93, -95, -97, -100, -102, -105, -107, -110, -113,
    -115, -118, -121, -123, -126, -129, -132, -135, -138, -141, -144, -147, -150, -153, -156, -160,
    -163, -166, -170, -173, -177, -181, -184, -188, -192, -196, -200, -204, -209, -213, -218, -222,
    -227, -232, -237, -242, -247, -253, -258, -264, -270, -277, -283, -290, -297, -305, -313, -321,
    -330, -339, -349, -359, -371, -383, -396, -411, -428, -446, -468, -494, -527, -527, -527, -527,
};


#define _XTAL_FREQ 20000000

volatile uint16_t esp_timeout_counter = 0;
volatile uint8_t esp_active = 1; // 1 = Attempt communication, 0 = Skip communication

#define HEATER LATCbits.LATC3
#define FAN    LATCbits.LATC1
#define LIGHT  LATCbits.LATC2
#define DISP_TX LATBbits.LATB4

#define LCD_ON_CURSOR_BLINK 25
#define LCD_ON_NO_CURSOR    22
#define LCD_ON_CURSOR       24
#define LCD_BACKLIGHT_OFF   18
#define LCD_BACKLIGHT_ON    17
#define LCD_CLR             12
#define LCD_L1              128
#define LCD_L2              148
#define LCD_L3              168
#define LCD_L4              188

volatile uint8_t cursor_mode = LCD_ON_NO_CURSOR;
uint8_t lcd_line_addrs[] = {LCD_L1, LCD_L2, LCD_L3, LCD_L4};


#define ADDR_INIT  0xFF
#define ADDR_SP    0x00
#define ADDR_FAN   0x10
#define ADDR_KP    0x20
#define ADDR_KI    0x24
#define AD

volatile uint16_t t_h = 0.0;
volatile uint16_t t_b = 0.0;
volatile uint8_t adc_ready = 0;
volatile uint8_t isr_channel = 0;
volatile int16_t box_setpoint = 400;
volatile int16_t heater_limit = 100;
volatile uint16_t adc_val[2];
volatile uint8_t flag_10hz = 0;
volatile uint8_t wifi_ticks = 0;

volatile uint8_t menu_state = 0; 
volatile uint8_t main_index = 4;

#define main_F              170
#define main_L              174
#define main_H              178
#define main_S              184

volatile uint8_t cursor_position[] = {main_F, main_L, main_H, main_S};
uint8_t last_menu_state = 0xFF;

volatile uint8_t menu_press = 0, up_press = 0, down_press = 0, select_press = 0;
volatile uint8_t btn_menu = 0, btn_up = 0, btn_down = 0, btn_select = 0;
volatile uint8_t btn_menu_state = 0, btn_up_state = 0, btn_down_state = 0, btn_select_state = 0;

#define not_pressed 0
#define was_pressed 1
#define being_held  3

volatile uint8_t select_mode = 0;

#define main_menu 0
#define wifi_menu 1
#define pid_menu 2

volatile uint8_t *btn_pins[] = {&btn_menu, &btn_up, &btn_down, &btn_select};
volatile uint8_t *btn_states[] = {&btn_menu_state, &btn_up_state, &btn_down_state, &btn_select_state};
volatile uint8_t *btn_flags[] = {&menu_press, &up_press, &down_press, &select_press};
volatile uint8_t time_to_send = 0;

uint8_t lcd_lines[] = {LCD_L1, LCD_L2, LCD_L3, LCD_L4};
uint8_t current_line_idx = 0;
char display_buffer[4][24]; // 4 lines, 20 chars + null terminator

float Kp = 10.0, Ki = 0.5, Kd = 0.1;
uint8_t fan_mode = 0;
uint8_t control_active = 1;

#define ESP_TX_MODE     2
#define ESP_RX_MODE     1
#define ESP_IDLE_MODE   0
volatile uint8_t esp_mode = 0;
volatile uint8_t rx_idx = 0;
volatile uint8_t data_ready_flag = 0;
volatile uint8_t rx_buf[13];
volatile uint8_t ack_received = 0;

void soft_putch(char data) {
    uint8_t gie_backup = INTCONbits.GIE;
    INTCONbits.GIE = 0;
    DISP_TX = 0; __delay_us(100);
    for(int i=0; i<8; i++) { DISP_TX = (data >> i) & 0x01; __delay_us(100); }
    DISP_TX = 1; __delay_us(100);
    INTCONbits.GIE = gie_backup;
}

void lcd_cmd_direct(uint8_t cmd) {
    soft_putch(cmd);
    __delay_ms(2);
}

void lcd_cmd_with_prefix(uint8_t cmd) {
    soft_putch(255);
    soft_putch(cmd);
    __delay_ms(2);
}

void lcd_move_cursor(uint8_t address) {
    // Just send the address command directly to the display
    soft_putch(address); 
    __delay_ms(2);
}

void lcd_write(const char *str) { while(*str) soft_putch(*str++); }

void poll_buttons(void) {
    btn_menu = (PORTBbits.RB3 == 0);
    btn_up = (PORTBbits.RB1 == 0);
    btn_down = (PORTBbits.RB2 == 0);
    btn_select = (PORTBbits.RB0 == 0);
    
    // Loop through all 4 buttons
    for (uint8_t i = 0; i < 4; i++) {
        if (*btn_pins[i]) {
            switch (*btn_states[i]) {
            case not_pressed: // button was not pressed previously
                *btn_states[i] = was_pressed;
                break;
            case was_pressed: // button was pressed previously
                *btn_states[i] = being_held;
                break;
            case being_held:
                break;
            default:
                break;
            }
        } else {
            switch (*btn_states[i]) {
            case not_pressed: // button was not pressed previously
                break;
            case being_held:
                *btn_flags[i] = 1; // flag to deal with button pressed
                *btn_states[i] = not_pressed;
                break;
            }
        }
    }
}

void DATA_EE_Write(uint8_t addr, uint8_t data) {
    EEADR = addr; EEDATA = data;
    EECON1bits.EEPGD = 0; EECON1bits.CFGS = 0; EECON1bits.WREN = 1;
    INTCONbits.GIE = 0; EECON2 = 0x55; EECON2 = 0xAA; EECON1bits.WR = 1;
    INTCONbits.GIE = 1; while(EECON1bits.WR) { ; } EECON1bits.WREN = 0;
}

uint8_t DATA_EE_Read(uint8_t addr) {
    EEADR = addr; EECON1bits.EEPGD = 0; EECON1bits.CFGS = 0;
    EECON1bits.RD = 1; return EEDATA;
}

void eeprom_write_f(uint8_t addr, float val) {
    uint8_t *p = (uint8_t *)&val;
    for(uint8_t i=0; i<4; i++) DATA_EE_Write(addr+i, p[i]);
}

float eeprom_read_f(uint8_t addr) {
    float val; uint8_t *p = (uint8_t *)&val;
    for(uint8_t i=0; i<4; i++) p[i] = DATA_EE_Read(addr+i);
    return val;
}

void __interrupt() ISR(void) { 
    // ADC handling
    if (PIR1bits.ADIF) {
        adc_val[isr_channel] = (uint16_t)((uint16_t)ADRESH << 8) | ADRESL;
        PIR1bits.ADIF = 0;
        adc_ready = 1; 
        isr_channel = (isr_channel == 0) ? 1 : 0;
        ADCON0bits.CHS = (isr_channel == 0) ? 0 : 2;
        ADCON0bits.GO = 1;
    }
    
    // Timer 0 handling
    if (INTCONbits.TMR0IF) {
        TMR0H = 0x3C; TMR0L = 0xAF;
        flag_10hz = 1;
        INTCONbits.TMR0IF = 0;
    }
    
    // Timer 1 handling
    if (PIR1bits.TMR1IF) {
        static uint16_t timer1_counter = 0;
        timer1_counter++;
        if (timer1_counter >= 572) {
            time_to_send = 1;
            timer1_counter = 0;
        }
        PIR1bits.TMR1IF = 0;
    }

    if (PIR1bits.RCIF) {
        if (RCSTAbits.OERR) { RCSTAbits.CREN = 0; RCSTAbits.CREN = 1; }
        
        char c = RCREG;
        
        if (c == 0x06) {
            ack_received = 1; // Set flag when ACK arrives
        }
        else if (c == 0x02) {
            esp_mode = ESP_RX_MODE;
            rx_idx = 0;
        }
        else if (esp_mode == ESP_RX_MODE) {
            if (rx_idx < 12) {
                rx_buf[rx_idx++] = c;
                if (rx_idx >= 12) {
                    esp_mode = ESP_IDLE_MODE; 
                }
            }
        }
    }
}

void handle_buttons(void){
    if (menu_press){
        menu_press=0;
        menu_state++;
        
        cursor_mode = LCD_ON_NO_CURSOR;
        
        if (menu_state > pid_menu) {
            menu_state = main_menu;
        }
        lcd_cmd_direct(LCD_CLR);
        
        switch (menu_state){
            case main_menu:
                sprintf(display_buffer[0], "Main Menu");
                sprintf(display_buffer[1], "H:%3d.%1d B:%3d.%1d", t_h / 10, t_h % 10, t_b / 10, t_b % 10);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                main_index =4;
                break;
            case wifi_menu:
                sprintf(display_buffer[0], "WIFI Menu");
                sprintf(display_buffer[1], "H:%3d.%1d B:%3d.%1d", t_h / 10, t_h % 10, t_b / 10, t_b % 10);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
            case pid_menu:
                sprintf(display_buffer[0], "PID Menu");
                sprintf(display_buffer[1], "H:%3d.%1d B:%3d.%1d", t_h / 10, t_h % 10, t_b / 10, t_b % 10);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
            default:
                sprintf(display_buffer[0], "default Menu");
                sprintf(display_buffer[1], "H:%3d.%1d B:%3d.%1d", t_h / 10, t_h % 10, t_b / 10, t_b % 10);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
        }
    }
    
    if (up_press){
        up_press=0;
        switch (menu_state){
            case main_menu:
                if(select_mode){
                    switch(main_index){
                        case 0://fan
                            FAN=1;
                            break;
                        case 1://light
                            LIGHT=1;
                            break;
                        case 2://heater
                            HEATER=1;
                            break;
                        case 3://setpoint
                            box_setpoint = box_setpoint+10;
                            break;
                            
                    }

                }
                else {
                    main_index++;
                    if (main_index >3) {
                        main_index = 0;
                    }
                    cursor_mode = LCD_ON_CURSOR;        
                  
                }
            break;
        }

    }
    if (down_press){
        down_press=0;
        switch (menu_state){
            case main_menu:
                if(select_mode){
                    switch(main_index){
                        case 0://fan
                            FAN=0;
                            break;
                        case 1://light
                            LIGHT=0;
                            break;
                        case 2://heater
                            HEATER=0;
                            break;
                        case 3://setpoint
                            box_setpoint = box_setpoint-10;
                            break;
                            
                    }

                }
                else {
                    if (main_index < 1) {
                        main_index = 4;
                    }
                    main_index--;

                    cursor_mode = LCD_ON_CURSOR;        
                }
            break;
                
        }

    }
    if (select_press){
        select_press=0;
        if (select_mode){
            select_mode=0;
            cursor_mode = LCD_ON_CURSOR;
        }
        else {
            switch (menu_state){
                case main_menu:
                    cursor_mode = LCD_ON_CURSOR_BLINK;
                    lcd_cmd_direct(cursor_mode);
                    select_mode = 1;
                    break;
                default:
                    break;
            }
        }
    }
}

void control_output(void){
    if (select_mode && main_index == 2 && menu_state==main_menu){
        // do nothing
    }
    else{
        if (t_b <= box_setpoint && t_h/10 <= heater_limit){
            HEATER = 1;
        }
        else {
            HEATER = 0;
        }
    }
}

void send_to_display(void) {
    
    uint8_t old_gie = INTCONbits.GIE; //save state of interrupts
    INTCONbits.GIE = 0;// stop all interrupts

    lcd_move_cursor(lcd_line_addrs[current_line_idx]);
    lcd_write(display_buffer[current_line_idx]); 

    current_line_idx++;
    if (current_line_idx >= 4) {
        current_line_idx = 0;
    } 
    lcd_move_cursor(cursor_position[main_index]);
    lcd_cmd_direct(cursor_mode);
    INTCONbits.GIE = old_gie;// restore interrupts
}

uint8_t send_to_esp(char *data, uint8_t length) {
    
    ack_received = 0;//reset
    
    // Check for UART error (Overrun or Framing)
    if (RCSTAbits.OERR) { 
        RCSTAbits.CREN = 0; 
        RCSTAbits.CREN = 1; 
    }
    
    // send start byte
    TXREG = 0x02;
    while(!PIR1bits.TXIF);

    // Transmit bytes without resetting TXEN
    for(uint8_t i = 0; i < length; i++) {
        uint16_t timeout = 50000; // Increased for reliable detection
        TXREG = data[i];
        while(!PIR1bits.TXIF);
        __delay_ms(2);
    }
    
    uint32_t wait_counter = 100000; 
    while(!ack_received && --wait_counter > 0);

    return (ack_received == 1);
}

uint8_t wait_for_handshake(uint8_t send_byte) {
    // Flush out garbage from rx
    if(RCSTAbits.OERR) { 
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }
    while(PIR1bits.RCIF) {
        volatile char dummy = RCREG;
    }
    // Send the byte only if TX is ready
    if(PIR1bits.TXIF) {
        TXREG = send_byte;
    } else {
        return 0; // TX busy, fail immediately to prevent lockup
    }
    
    // 2. Poll with a significantly smaller timeout
    uint16_t timeout = 5000; 
    while(--timeout > 0) {
        if(RCSTAbits.OERR) { // Clear overrun errors
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
        }
        if(PIR1bits.RCIF) {
            return (RCREG);
        }
    }
    return 0;
}

void main(void) {
    // 20MHz Clock, 9600 Baud @ BRGH=1
    TXSTAbits.BRGH = 1;       // High Speed mode
    TXSTAbits.SYNC = 0;       // Asynchronous
    BAUDCONbits.BRG16 = 0;    // 8-bit mode
    SPBRG = 129;              // (20,000,000 / (16 * 9600)) - 1 = 129
    RCSTAbits.SPEN = 1;       // Enable Serial Port
    TXSTAbits.TXEN = 1;       // Enable Transmitter
    RCSTAbits.CREN = 1;       // Enable Receiver
    __delay_ms(500);//for display to power up
  // Configuration
    ECANCON = 0x00;
    CANCON = 0x20;
    LATC = 0x00; TRISC = 0x00; TRISA = 0x05; TRISB = 0x0F;
    ADCON1 = 0x0D; ADCON2 = 0x92; ADCON0bits.ADON = 1;
    
    // A/D channel 0 (Heater Temp)
    ADCON0bits.CHS = 0;
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO); // Wait for conversion to complete
    adc_val[0] = (uint16_t)((uint16_t)ADRESH << 8) | ADRESL;
    
    // A/D channel 2 (Box Temp)
    ADCON0bits.CHS = 2;
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO); // Wait for conversion to complete
    adc_val[1] = (uint16_t)((uint16_t)ADRESH << 8) | ADRESL;
        
    lcd_cmd_direct(LCD_ON_NO_CURSOR);
    lcd_cmd_direct(LCD_BACKLIGHT_ON);
    lcd_cmd_direct(LCD_CLR);
    
    // Initial buffer setup
    sprintf(display_buffer[0], "*Heater Box Control*");
    sprintf(display_buffer[1], "   by Dan Jubenville");
    sprintf(display_buffer[2], "June 2026 recovering");
    sprintf(display_buffer[3], "from big toe surgery");
    
    lcd_move_cursor(lcd_line_addrs[0]);
    lcd_write(display_buffer[0]);
    lcd_move_cursor(lcd_line_addrs[1]);
    lcd_write(display_buffer[1]);
    lcd_move_cursor(lcd_line_addrs[2]);
    lcd_write(display_buffer[2]);
    lcd_move_cursor(lcd_line_addrs[3]);
    lcd_write(display_buffer[3]);
    
          // Splash screen (standard delay allowed here as no other tasks are running)
    __delay_ms(2000);
    lcd_cmd_direct(LCD_CLR);
    uint8_t rx_byte = wait_for_handshake(0xAA);
    if (rx_byte == 0x55) {
        sprintf(display_buffer[3], "ESP OK 0x%02X", rx_byte);
    } else {
        sprintf(display_buffer[3], "ESP FAIL 0x%02X", rx_byte);
    }
    esp_mode = ESP_IDLE_MODE;
    lcd_move_cursor(lcd_line_addrs[2]);
    sprintf(display_buffer[2], "RX:0x%02X", RCREG);
    lcd_write(display_buffer[2]);
    __delay_ms(2000);
    
    lcd_move_cursor(lcd_line_addrs[3]);
    lcd_write(display_buffer[3]);
    __delay_ms(1000);
    
    PIE1bits.RCIE = 1; // Enable UART Receive Interrupt
    // Start interrupts for main loop
    T0CON = 0x84; INTCONbits.TMR0IE = 1; PIE1bits.ADIE = 1; INTCONbits.GIE = 1; INTCONbits.PEIE = 1;
    T1CON = 0x30; TMR1H = 0; TMR1L = 0; PIE1bits.TMR1IE = 1; T1CONbits.TMR1ON = 1;
    
    ADCON0bits.CHS = 0; 
    ADCON0bits.GO = 1;
    
    sprintf(display_buffer[0], "Main Menu");
    sprintf(display_buffer[1], "H:%3d.%1d B:%3d.%1d", t_h / 10, t_h % 10, t_b / 10, t_b % 10);
    sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
    sprintf(display_buffer[3], "");

    while(1) {
        
        //watchdog
        if (esp_mode != ESP_IDLE_MODE) {
            esp_timeout_counter++;
            if (esp_timeout_counter > 50000) { // Adjust based on your loop speed
                esp_mode = ESP_IDLE_MODE;
                rx_idx = 0;
                esp_timeout_counter = 0;
            }
        } else {
            esp_timeout_counter = 0;
        }
        
        poll_buttons();
        
        handle_buttons();
        
        control_output();
        
        if (esp_mode == ESP_IDLE_MODE) {
            static uint8_t retry_count = 0;
            if (time_to_send) {
                time_to_send = 0;

                if (esp_active) {
                    char packet_buffer[24];
                    sprintf(packet_buffer, "%04X%04X%02X%02X%02X%04X", t_h, t_b, FAN, LIGHT, HEATER, box_setpoint);
                    
                    if (send_to_esp(packet_buffer, 18)) {
                        sprintf(display_buffer[3], "ESP SUCCESS        ");
                        retry_count = 0;
                    } else {
                        retry_count++;
                        sprintf(display_buffer[3], "ESP FAIL (%d)      ", retry_count);
                    }
                } 
                else {
                    // This is what you were missing: 
                    // Logic to handle when the ESP is not active
                    sprintf(display_buffer[3], "NO ESP DETECTED    ");
                }

                lcd_move_cursor(lcd_line_addrs[3]);
                lcd_write(display_buffer[3]);
}

            if (flag_10hz) {
                flag_10hz = 0;

                if (adc_ready) {
                    adc_ready = 0;

                    if (!isr_channel) {
                        t_h = heater_temp_lut[adc_val[0]];    
                    }
                    else {
                        t_b = box_temp_lut[adc_val[1]];
                    }
                }

                if (menu_state == main_menu){
                    //sprintf(display_buffer[0], "%04X%04X%02X%02X%02X%04X", t_h, t_b, FAN, LIGHT, HEATER, box_setpoint);
                    sprintf(display_buffer[1], "H:%3d.%1d B:%3d.%1d", t_h / 10, t_h % 10, t_b / 10, t_b % 10);
                    sprintf(display_buffer[2], "F:%d L:%d H:%d S:%3d.%1d", FAN, LIGHT, HEATER, box_setpoint / 10, box_setpoint % 10);

                }

                send_to_display();
            
            }
        }
        else {
            // ESP is active, skip display updates to prevent timing contention
            // Optional: clear the display or show a "BUSY" message once
        }
    }
}