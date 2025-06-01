#include <STC8G.H>

unsigned int pwm = 0;
unsigned int Key_Slow_Down = 0;
unsigned char Key_Val, Key_Down, Key_Up, Key_Old;

void Delay1x_us(unsigned int m)	//@12.000MHz Delay1x_ms(1);
{
    if (m == 0) return;
    while (m--)
    {
		 unsigned char data i, j;

	i = 2;
	j = 39;
	do
	{
		while (--j);
	} while (--i);
//        unsigned char i, j;
//        i = 32;
//        j = 40;
//        do
//        {
//            while (--j);
//        } while (--i);
    }
}
void Delay1x_mms(unsigned int m)	//@12.000MHz Delay1x_ms(1);
{
    if (m == 0) return;
    while (m--)
    {

        unsigned char i, j;
        i = 32;
        j = 40;
        do
        {
            while (--j);
        } while (--i);
    }
}
void Timer0_Isr(void) interrupt 1
{
    if (++Key_Slow_Down == pwm)
    {
        Key_Slow_Down = 0;
    
    }
}

void Timer0_Init(void)		//1??@24.000MHz
{
    AUXR |= 0x80;			//?????1T???
    TMOD &= 0xF0;			//????????
    TL0 = 0x40;				//????????
    TH0 = 0xA2;				//????????
    TF0 = 0;				//??TF0????
    TR0 = 1;				//???0????
    ET0 = 1;				//?????0???
}

void main()
{
    P3M0 = 0x00;
    P3M1 = 0x00;
    P5M0 = 0x00;
    P5M1 = 0x00;
	P55 = 1;Delay1x_mms(1000);P55 = 0;
    P54 = 0;

//    Timer0_Init();
pwm =98;
    while (1)
    {
//        if (P54==1)
//        {
//            Delay1x_mms(10); // ????
//            if (P54==1) // ????
//					{  pwm++;P54 = 0;P55=0;Delay1x_mms(10);
//                if(pwm==3){pwm =3;} 
//					 if(pwm==4){pwm =6;}
//					 if(pwm==7){pwm =14;}
//					 if(pwm==15){pwm =50;}
//					 if(pwm==51){pwm =100;}
//					 if(pwm==101){pwm=2;}
//			
//			}
//        }
		  if (P54==1)//·çÉÈµ÷ËÙ
			  {pwm=100-pwm;            Delay1x_mms(10); // ????
            if (P54==1) // ????
					{  pwm++;P54 = 0;P55=0;
                if(pwm==3){pwm =10;} 
					 if(pwm==11){pwm =30;}
					 if(pwm==31){pwm =50;}
					 if(pwm==51){pwm =75;}
					 if(pwm==76){pwm =98;}
					 if(pwm==99){pwm=2;}
				 }
					pwm=100-pwm;
			}
//		  if(pwm==0)P55 = 1;
		   P55=1;Delay1x_us(pwm);
		   P55=0;Delay1x_us(100-pwm);
    }
}
	 