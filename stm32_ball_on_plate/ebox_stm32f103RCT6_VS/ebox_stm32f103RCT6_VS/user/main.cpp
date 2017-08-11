#include "ebox.h"
#include "ball_on_plate.h"

//����
#include "led.h"
#include "uart_vcan.h"

#include "my_math.h"
#include "signal_stream.h"
#include <math.h>

//����
#include "Button.h"
#include "oled_i2c.h"
//����ϵͳ
#include "freertos.h"
#include "task.h"
#include "queue.h"


using namespace std;

BallOnPlate ballOnplate;

//����
UartVscan uartVscan(&uart1);
FpsCounter fpsUI;
float fpsUItemp;
float outX, outY;

//����
const float rateUI = 10;
const float intervalUI = 1 / rateUI;
const uint32_t uiRefreshDelay = ((1000 * intervalUI + 0.5) / portTICK_RATE_MS);
Button keyL(&PB4, 1);
Button keyR(&PB1, 1);
Button keyU(&PC5, 1);
Button keyD(&PC2, 1);
Led led(&PD2, 1);
OLEDI2C oled(&i2c1);


//��������
int numIndex = 0;
int task = 0;
bool inProgress = false;
bool isReady=false;
bool isEnd = true;
TicToc taskTimer;

float circleR = 0;
float theta = 0;
const float rateCircle = 0.2;
void posReceiveEvent()
{
	//������Ӧ
	keyL.loop();
	keyR.loop();
	keyU.loop();
	keyD.loop();

	if (keyR.click())
	{
		numIndex++;
	}
	if (keyL.click())
	{
		numIndex--;
	}

	//�ܹ�7�����֣�0������������
	limit<int>(numIndex, 0, 5);

	//������Ӧ
	float increase = 0;
	if (keyU.click())
	{
		increase++;
	}
	if (keyD.click())
	{
		increase--;
	}
	if (keyU.pressed_for(200, 0))
	{
		increase += 1;
	}
	if (keyD.pressed_for(200, 0))
	{
		increase -= 1;
	}
	if (keyR.pressed_for(5000, 1))
	{
		ballOnplate.shutdownRasp();
	}

	//float targetXraw = ballOnplate.getTargetXRaw();
	//float targetYraw = ballOnplate.getTargetYRaw();
	switch (numIndex)
	{
	case 0:
		task += increase;
		limit(task, 0, 7);
		if (increase != 0)
		{
			inProgress = false;
			isReady = false;
		}
		break;
	case 1:
		//ѡ�������
		if (increase != 0 && inProgress == false)
		{
			inProgress = true;
			//��ʼ����
			switch (task)
			{
			case 0:
				ballOnplate.setTargetCircle(4);
				break;
			case 1:
				ballOnplate.setTargetCircle(1);
				break;
			case 2:
				ballOnplate.startPath(0, 4, 100);
				break;
			default:
				break;
			}
		}
		else if (increase != 0 && inProgress == true)
		{
			ballOnplate.stopPath();
			inProgress = false;
			isReady = false;
		}
	//case 1:
	//	targetYraw += increase;
	//	limit<float>(targetYraw, 30, ballOnplate.getPosMax() - 30);
	//	break;
	//case 2:
	//	circleR = circleR + increase;
	//	limit<float>(circleR, 0, (ballOnplate.getPosMax() - 100) / 2);
	//	theta += 2 * PI *ballOnplate.getIntervalPID() * rateCircle;//0.5Ȧһ��
	//	targetXraw = ballOnplate.getPosMax() / 2 + circleR*sin(theta);
	//	targetYraw = ballOnplate.getPosMax() / 2 + circleR*cos(theta);
	//	break;
	default:
		break;
	}

	//�������
	switch (task)
	{
	case 2:
		//if (isReady == false)
		//{
		//	if (ballOnplate.isBallInCircle(0))
		//	{
		//		isReady = true;
		//		isEnd = false;
		//		taskTimer.tic();
		//	}
		//}
		//if (isReady == true && taskTimer.toc() > 2 && isEnd == false)
		//{
		//	isEnd = true;
		//	ballOnplate.startPath(0, 4, 50);
		//}
	default:
		break;
	}

	//ballOnplate.setTarget(targetXraw, targetYraw);
	
	float vscan[] = {
		ballOnplate.getPosX(),ballOnplate.getPosY()
		,ballOnplate.getOutX(),ballOnplate.getOutY()
		,ballOnplate.getTargetXRaw(),ballOnplate.getTargetYRaw()
		//,ballOnplate.getFeedforwardX(),ballOnplate.getFeedforwardY()
		,ballOnplate.getTargetXFiltered(),ballOnplate.getTargetYFiltered()
	};
	uartVscan.sendOscilloscope(vscan, 8);

	//����
	outX = ballOnplate.getOutX();
	outY = ballOnplate.getOutY();
}

//UI����
void uiRefresh(void *pvParameters)
{
	portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		float fpsPIDtemp;
		//�ж��Ƿ���ݮ���ѹػ�
		if (!ballOnplate.getIsPosReceiving())
		{
			fpsPIDtemp = 0;
		}
		else
		{
			fpsPIDtemp = ballOnplate.getFps();
		}

		//��ʾ������
		oled.printf(0, 0, Oledi2c_Font_8x16, "task:");
		if (numIndex != 0)
		{
			oled.printf(40, 0, Oledi2c_Font_8x16, "%d",task);
		}
		else
		{
			oled.printf(40, 0, Oledi2c_Font_8x16_Inv, "%d", task);
		}

		//��ʾ��ʼ����
		if (numIndex != 1)
		{
			if (inProgress == false)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16, "start");
			}
			else 
			{
				oled.printf(64, 0, Oledi2c_Font_8x16, "stop ");
			}
		}
		else
		{
			if (inProgress == false)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16_Inv, "start");
			}
			else
			{
				oled.printf(64, 0, Oledi2c_Font_8x16_Inv, "stop ");
			}
		}
		

		//switch (numIndex)
		//{
		//case 0:
		//	oled.printf(0, 0, 2, "*%.1f %.1f %.0f  ", ballOnplate.getTargetXraw(), ballOnplate.getTargetYraw(), circleR);
		//	break;
		//case 1:
		//	oled.printf(0, 0, 2, "%.1f *%.1f %.0f  ", ballOnplate.getTargetXraw(), ballOnplate.getTargetYraw(), circleR);
		//	break;
		//case 2:
		//	oled.printf(0, 0, 2, "%.1f %.1f *%.0f  ", ballOnplate.getTargetXraw(), ballOnplate.getTargetYraw(), circleR);
		//	break;
		//default:
		//	break;
		//}

		oled.printf(0, 5, Oledi2c_Font_6x8, "%-8.1f%-8.1f", ballOnplate.getTargetXRaw(), ballOnplate.getTargetYRaw());
		oled.printf(0, 6, Oledi2c_Font_6x8, "%-8.1f%-8.1f", ballOnplate.getPosX(), ballOnplate.getPosY());
		fpsUItemp = fpsUI.getFps();
		oled.printf(0, 7, Oledi2c_Font_6x8, "%-8.0f%-8.0f", fpsPIDtemp, fpsUItemp);

		vTaskDelayUntil(&xLastWakeTime, uiRefreshDelay);
	}

}

//����·�����������
void showPathIndex(int* pathPointIndex, int pathLength)
{
	for (int i = 0; i < pathLength; i++)
	{
		uart1.printf("%d\t", pathPointIndex[i]);
	}
	uart1.printf("\r\n");
}


void setup()
{
	ebox_init();
	ballOnplate.attachAfterPIDEvent(posReceiveEvent);
	ballOnplate.begin();


	//����
	uart1.begin(115200);
	fpsUI.begin();


	//����
	keyD.begin();
	keyL.begin();
	keyR.begin();
	keyU.begin();
	led.begin();
	oled.begin();

	set_systick_user_event_per_sec(configTICK_RATE_HZ);
	attach_systick_user_event(xPortSysTickHandler);
	xTaskCreate(uiRefresh, "uiRefresh", 512, NULL, 0, NULL);
	vTaskStartScheduler();
}


int main(void)
{
	setup();


	while (1)
	{

	}

}


