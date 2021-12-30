#define M_SEC 1000

/* Gonna mess around with FPS
 * - kouts */
#define DEFAULT_FPS 60

typedef unsigned long msec_t;
void cap_fps(int fps);
void delay(msec_t i);
msec_t gettime(void);

