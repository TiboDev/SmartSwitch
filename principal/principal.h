/* TIME configuration */

#define UART_HOUR			    1
#define UART_MINUTE			  2
#define UART_SECOND 		  3

/* WIFI configuration */

#define UART_WIFI_STATUS	4
#define UART_WIFI_RANGE		5

/* MENU configuration */
#define UART_MENU_TYPE    6
#define MENU_TYPE_START   7
#define MENU_TYPE_BEST_HOUR   8
#define MENU_TYPE_PROCESS  9
#define MENU_TYPE_END     10
#define MENU_TYPE_ERROR   11


/* Internals variables */
#define UART_AVERAGE_PRICE    13
#define UART_CHARGING_DURATION    14
#define UART_REMAINING_TIME    15
#define UART_END_TIME    16
#define UART_START_TIME    17

/* CURSOR configuration */
#define UART_CURSOR_SCREEN  18
#define CURSOR_OK           19
#define CURSOR_RETURN       20
#define UART_CONFIG_DURATION    21


/* potentiometer */
#define DECREASE           50
#define INCREASE           51

/* stat of the program */
#define STAT_START        52
#define STAT_CONFIG       53
#define STAT_INFORMATION  54
#define STAT_PROCESS      55
#define STAT_END          56

/* LED */
#define RED               60
#define GREEN             61
#define BLUE              62
#define ORANGE            63
#define WHITE             64
#define BLACK             65
