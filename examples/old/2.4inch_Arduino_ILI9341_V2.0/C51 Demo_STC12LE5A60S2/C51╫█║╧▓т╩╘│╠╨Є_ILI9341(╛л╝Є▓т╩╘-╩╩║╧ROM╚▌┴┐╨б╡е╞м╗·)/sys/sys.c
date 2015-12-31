#include "sys.h"
void delay_ms(int count)  // /* X1ms */
{
        int i,j;
        for(i=0;i<count;i++)
                for(j=0;j<1000;j++);
}

void delay_us(int count)  // /* X1us */
{
        int i,j;
        for(i=0;i<count;i++)
                for(j=0;j<1;j++);
}