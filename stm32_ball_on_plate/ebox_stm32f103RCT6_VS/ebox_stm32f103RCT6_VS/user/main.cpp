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

BallOnPlateBase ballOnplate;
BallOnPlatePath path;

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


//�յ���λ������������PID����
//���������outXY�����ˢ�³������
//UI����
int index = 0;
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
		index++;
	}
	if (keyL.click())
	{
		index--;
	}
	limit<int>(index, 0, 2);

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
		increase += 2;
	}
	if (keyD.pressed_for(200, 0))
	{
		increase -= 2;
	}
	if (keyR.pressed_for(5000, 1))
	{
		ballOnplate.shutdownRasp();
	}

	float targetXraw = ballOnplate.getTargetXraw();
	float targetYraw = ballOnplate.getTargetYraw();
	switch (index)
	{
	case 0:
		targetXraw += increase;
		limit<float>(targetXraw, 30, ballOnplate.getPosMax() - 30);
		break;
	case 1:
		targetYraw += increase;
		limit<float>(targetYraw, 30, ballOnplate.getPosMax() - 30);
		break;
	case 2:
		circleR = circleR + increase;
		limit<float>(circleR, 0, (ballOnplate.getPosMax() - 100) / 2);
		theta += 2 * PI *ballOnplate.getIntervalPID() * rateCircle;//0.5Ȧһ��
		targetXraw = ballOnplate.getPosMax() / 2 + circleR*sin(theta);
		targetYraw = ballOnplate.getPosMax() / 2 + circleR*cos(theta);
		break;
	default:
		break;
	}

	ballOnplate.setTarget(targetXraw, targetYraw);

	//float vscan[] = { 
	//	ballOnplate.getPosX(),ballOnplate.getPosY()
	//	,ballOnplate.getOutX(),ballOnplate.getOutY()
	//	,ballOnplate.getFeedforwardX(),ballOnplate.getFeedforwardY()
	//	,ballOnplate.getTargetXFiltered(),ballOnplate.getTargetYFiltered()
	//};
	//uartVscan.sendOscilloscope(vscan, 8);
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

		switch (index)
		{
		case 0:
			oled.printf(0, 0, 2, "*%.1f %.1f %.0f  ", ballOnplate.getTargetXraw(), ballOnplate.getTargetYraw(), circleR);
			break;
		case 1:
			oled.printf(0, 0, 2, "%.1f *%.1f %.0f  ", ballOnplate.getTargetXraw(), ballOnplate.getTargetYraw(), circleR);
			break;
		case 2:
			oled.printf(0, 0, 2, "%.1f %.1f *%.0f  ", ballOnplate.getTargetXraw(), ballOnplate.getTargetYraw(), circleR);
			break;
		default:
			break;
		}
		oled.printf(0, 2, 2, "%-8.1f%-8.1f", ballOnplate.getPosX(), ballOnplate.getPosY());
		fpsUItemp = fpsUI.getFps();
		oled.printf(0, 4, 2, "%-8.0f%-8.0f", fpsPIDtemp, fpsUItemp);

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

	//����·���滮��
	for (int src = 0; src < 9; src++)
	{
		for (int dst = 0; dst < 9; dst++)
		{
			uart1.printf("%d��%d��\t", src, dst);
			path.computePathPoint(src, dst);
			showPathIndex(path.getPathPointIndex(), path.getPathLength());
		}
	}

	//��������
	//	0��0��	0	0
	//	0��1��	0	1
	//	0��2��	0	9	10	2
	//	0��3��	0	3
	//	0��4��	0	4
	//	0��5��	0	9	10	5
	//	0��6��	0	9	11	6
	//	0��7��	0	9	11	7
	//	0��8��	0	9	11	12	8
	//	1��0��	1	0
	//	1��1��	1	1
	//	1��2��	1	2
	//	1��3��	1	3
	//	1��4��	1	4
	//	1��5��	1	5
	//	1��6��	1	9	11	6
	//	1��7��	1	10	12	7
	//	1��8��	1	10	12	8
	//	2��0��	2	10	9	0
	//	2��1��	2	1
	//	2��2��	2	2
	//	2��3��	2	10	9	3
	//	2��4��	2	4
	//	2��5��	2	5
	//	2��6��	2	10	9	11	6
	//	2��7��	2	10	12	7
	//	2��8��	2	10	12	8
	//	3��0��	3	0
	//	3��1��	3	1
	//	3��2��	3	9	10	2
	//	3��3��	3	3
	//	3��4��	3	4
	//	3��5��	3	11	12	5
	//	3��6��	3	6
	//	3��7��	3	7
	//	3��8��	3	11	12	8
	//	4��0��	4	0
	//	4��1��	4	1
	//	4��2��	4	2
	//	4��3��	4	3
	//	4��4��	4	4
	//	4��5��	4	5
	//	4��6��	4	6
	//	4��7��	4	7
	//	4��8��	4	8
	//	5��0��	5	10	9	0
	//	5��1��	5	1
	//	5��2��	5	2
	//	5��3��	5	12	11	3
	//	5��4��	5	4
	//	5��5��	5	5
	//	5��6��	5	12	11	6
	//	5��7��	5	7
	//	5��8��	5	8
	//	6��0��	6	11	9	0
	//	6��1��	6	11	9	1
	//	6��2��	6	11	12	10	2
	//	6��3��	6	3
	//	6��4��	6	4
	//	6��5��	6	11	12	5
	//	6��6��	6	6
	//	6��7��	6	7
	//	6��8��	6	11	12	8
	//	7��0��	7	11	9	0
	//	7��1��	7	12	10	1
	//	7��2��	7	12	10	2
	//	7��3��	7	3
	//	7��4��	7	4
	//	7��5��	7	5
	//	7��6��	7	6
	//	7��7��	7	7
	//	7��8��	7	8
	//	8��0��	8	12	10	9	0
	//	8��1��	8	12	10	1
	//	8��2��	8	12	10	2
	//	8��3��	8	12	11	3
	//	8��4��	8	4
	//	8��5��	8	5
	//	8��6��	8	12	11	6
	//	8��7��	8	7
	//	8��8��	8	8





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


