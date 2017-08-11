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

BallOnPlate ballOnPlate;

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
TicToc timerTask,timerUI;
int stage = 0;//�����״̬��0����ֹͣ��1����׼�����

//·������
int abcd[4] = { 0 };
const float speedTask6=150;

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
		ballOnPlate.shutdownRasp();
	}


	switch (numIndex)
	{
	case 0:
		stage = 0;//ֹͣ�����е�����
		task += increase;
		limit(task, 0, 7);
		break;
	case 1:
		//ѡ�������
		if (increase != 0)
		{
			//��ʼ����
			switch (task)
			{
			case 0://����0������
				if (isnan(ballOnPlate.getPosX()))//��ֹ׼��ʱ�ٶ�̫����ƽ��
				{
					ballOnPlate.startPath(4, 4, 100);
				}
				else
				{
					ballOnPlate.startPath(4, 100);
				}
				stage = 1;
				break;
			case 1://����1��2�ȶ�5
				if (isnan(ballOnPlate.getPosX()))
				{
					ballOnPlate.startPath(1, 1, 100);
				}
				else
				{
					ballOnPlate.startPath(1, 100);
				}
				stage = 1;
				break;
			case 2://����2��1~5ͣ��2<15
				if (stage == 0)
				{
					if (isnan(ballOnPlate.getPosX()))
					{
						ballOnPlate.startPath(0, 0, 100);
					}
					else
					{
						ballOnPlate.startPath(0, 100);
					}
					stage = 1;
				}
				else if (stage == 1)
				{
					timerUI.tic();
					ballOnPlate.startPath(0, 4, 100);
					stage = 2;
				}
				break;
			case 3://����3��1~4ͣ��2~5ͣ��2<20
				if (stage == 0)
				{
					if (isnan(ballOnPlate.getPosX()))
					{
						ballOnPlate.startPath(0, 0, 100);
					}
					else
					{
						ballOnPlate.startPath(0, 100);
					}
					stage = 1;
				}
				else if (stage == 1)
				{
					timerUI.tic();
					ballOnPlate.startPath(0, 3, 100);
					timerTask.tic();
					stage = 2;
				}
				break;
			case 4://����4��1~9ͣ��2<30
				if (stage == 0)
				{
					if (isnan(ballOnPlate.getPosX()))
					{
						ballOnPlate.startPath(0, 0, 100);
					}
					else
					{
						ballOnPlate.startPath(0, 100);
					}
					stage = 1;
				}
				else if (stage == 1)
				{
					timerUI.tic();
					ballOnPlate.startPath(0, 8, 100);
					stage = 2;
				}
				
				break;
			case 5://����5��1~2~6~9ͣ��2<40
				if (stage == 0)
				{
					if (isnan(ballOnPlate.getPosX()))
					{
						ballOnPlate.startPath(0, 0, 100);
					}
					else
					{
						ballOnPlate.startPath(0, 100);
					}
					stage = 1;
				}
				else if (stage == 1)
				{
					timerUI.tic();
					ballOnPlate.startPath(0, 1, 100);
					timerTask.tic();
					stage = 2;
				}
				break;
			case 6://����6���ֶ������Ⱥ󾭹�ABCD
				if (stage == 0)
				{
					if (isnan(ballOnPlate.getPosX()))
					{
						ballOnPlate.startPath(abcd[0], abcd[0], speedTask6);
					}
					else
					{
						ballOnPlate.startPath(abcd[0], speedTask6);
					}
					
					stage = 1;
				}
				else if (stage == 1)
				{
					timerUI.tic();
					ballOnPlate.startPath(abcd[0], abcd[1], speedTask6);
					timerTask.tic();
					stage = 2;
				}
				break;
			case 7://����7��

				break;
			default:
				break;
			}
		}
		break;
	case 2://A
		stage = 0;
		abcd[0] += increase;
		limit(abcd[0], 0, 8);
		break;
	case 3://B
		stage = 0;
		abcd[1] += increase;
		limit(abcd[1], 0, 8);
		break;
	case 4://C
		stage = 0;
		abcd[2] += increase;
		limit(abcd[2], 0, 8);
		break;
	case 5://D
		stage = 0;
		abcd[3] += increase;
		limit(abcd[3], 0, 8);
		break;
	default:
		break;
	}

	switch (task)
	{
	case 1:
		if (stage == 1 && !isnan(ballOnPlate.getPosX()))
		{
			timerUI.tic();
			stage = 2;
		}
		else if (stage == 2 && isnan(ballOnPlate.getPosX()))
		{
			stage = 1;
		}
		break;
	case 3:
		if (timerTask.toc() > 5000 && stage == 2)
		{
			ballOnPlate.startPath(3, 4, 100);
			stage = 3;
		}
		break;
	case 5:
		if (timerTask.toc() > 3000 && stage == 2)
		{
			ballOnPlate.startPath(1, 5, 100);
			stage = 3;
			timerTask.tic();
		}
		else if (timerTask.toc() > 4000 && stage == 3)
		{
			ballOnPlate.startPath(5, 8, 100);
			stage = 4;
		}
		break;
	case 6:
		if (timerTask.toc() > 5000 && stage == 2)
		{
			ballOnPlate.startPath(abcd[1], abcd[2], speedTask6);
			stage = 3;
			timerTask.tic();
		}
		else if (timerTask.toc() > 5000 && stage == 3)
		{
			ballOnPlate.startPath(abcd[2], abcd[3], speedTask6);
			stage = 4;
		}
		break;
	default:
		break;
	}

	
	float vscan[] = {
		ballOnPlate.getPosX(),ballOnPlate.getPosY()
		,ballOnPlate.getOutX(),ballOnPlate.getOutY()
		,ballOnPlate.getTargetXRaw(),ballOnPlate.getTargetYRaw()
		//,ballOnplate.getFeedforwardX(),ballOnplate.getFeedforwardY()
		,ballOnPlate.getTargetXFiltered(),ballOnPlate.getTargetYFiltered()
	};
	uartVscan.sendOscilloscope(vscan, 8);

	//����
	outX = ballOnPlate.getOutX();
	outY = ballOnPlate.getOutY();
	//float point[3];
	//bool isEnd = path.getNext(point, point + 1);
	//if (isEnd)
	//{
	//	point[2] = 100;
	//}
	//uartVscan.sendOscilloscope(point, 3);
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
		if (!ballOnPlate.getIsPosReceiving())
		{
			fpsPIDtemp = 0;
		}
		else
		{
			fpsPIDtemp = ballOnPlate.getFps();
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
			if (stage == 0)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16, "start   ");
			}
			else if (stage == 1)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16, "ready   ");
			}
			else if (stage >= 2)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16, "working ");
			}
			
		}
		else
		{
			if (stage == 0)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16_Inv, "start");
			}
			else if (stage == 1)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16_Inv, "ready");
			}
			else if (stage >= 2)
			{
				oled.printf(64, 0, Oledi2c_Font_8x16_Inv, "working");
			}
		}

		//��ʾ4λ��
		for (int i = 0; i < 4; i++)
		{
			if (numIndex - 2 == i)
			{
				oled.printf(i * 8, 2, Oledi2c_Font_8x16_Inv, "%1d",abcd[i]+1);
			}
			else
			{
				oled.printf(i * 8, 2, Oledi2c_Font_8x16, "%1d", abcd[i] + 1);
			}
			
		}
		
		//��ʾ��ʱ
		if (stage >= 2)
		{
			oled.printf(64, 2, Oledi2c_Font_8x16, "%-8.1f", (float)timerUI.toc()/1000);
		}
		oled.printf(0, 5, Oledi2c_Font_6x8, "%-8.1f%-8.1f", ballOnPlate.getTargetXRaw(), ballOnPlate.getTargetYRaw());
		oled.printf(0, 6, Oledi2c_Font_6x8, "%-8.1f%-8.1f", ballOnPlate.getPosX(), ballOnPlate.getPosY());
		fpsUItemp = fpsUI.getFps();
		oled.printf(0, 7, Oledi2c_Font_6x8, "%-8.0f%-8.0f", fpsPIDtemp, fpsUItemp);

		vTaskDelayUntil(&xLastWakeTime, uiRefreshDelay);
	}

}

//��ʾ���ɵ�·��
BallOnPlatePathIndex path;
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
	ballOnPlate.attachAfterPIDEvent(posReceiveEvent);
	ballOnPlate.begin();
	ballOnPlate.startPath(4, 4, 100);//����


	//����
	uart1.begin(115200);
	fpsUI.begin();

	for (int src = 0; src < 13; src++)
	{
		for (int dst = 0; dst < 13; dst++)
		{
			uart1.printf("%d��%d��     \t", src, dst);
			path.computePathPoint(src, dst);
			showPathIndex(path.pathPointIndex, path.getPathLength());
		}
	}
	//���Խ��
	//	0��0��     	0	0
	//	0��1��     	0	1
	//	0��2��     	0	9	10	2
	//	0��3��     	0	3
	//	0��4��     	0	4
	//	0��5��     	0	9	10	5
	//	0��6��     	0	9	11	6
	//	0��7��     	0	9	11	7
	//	0��8��     	0	9	11	12	8
	//	0��9��     	0	9	11	12	8
	//	0��10��     	0	9	11	12	8
	//	0��11��     	0	9	11	12	8
	//	0��12��     	0	9	11	12	8
	//	1��0��     	1	0
	//	1��1��     	1	1
	//	1��2��     	1	2
	//	1��3��     	1	3
	//	1��4��     	1	4
	//	1��5��     	1	5
	//	1��6��     	1	9	11	6
	//	1��7��     	1	10	12	7
	//	1��8��     	1	10	12	8
	//	1��9��     	1	10	12	8
	//	1��10��     	1	10	12	8
	//	1��11��     	1	10	12	8
	//	1��12��     	1	10	12	8
	//	2��0��     	2	10	9	0
	//	2��1��     	2	1
	//	2��2��     	2	2
	//	2��3��     	2	10	9	3
	//	2��4��     	2	4
	//	2��5��     	2	5
	//	2��6��     	2	10	9	11	6
	//	2��7��     	2	10	12	7
	//	2��8��     	2	10	12	8
	//	2��9��     	2	10	12	8
	//	2��10��     	2	10	12	8
	//	2��11��     	2	10	12	8
	//	2��12��     	2	10	12	8
	//	3��0��     	3	0
	//	3��1��     	3	1
	//	3��2��     	3	9	10	2
	//	3��3��     	3	3
	//	3��4��     	3	4
	//	3��5��     	3	11	12	5
	//	3��6��     	3	6
	//	3��7��     	3	7
	//	3��8��     	3	11	12	8
	//	3��9��     	3	11	12	8
	//	3��10��     	3	11	12	8
	//	3��11��     	3	11	12	8
	//	3��12��     	3	11	12	8
	//	4��0��     	4	0
	//	4��1��     	4	1
	//	4��2��     	4	2
	//	4��3��     	4	3
	//	4��4��     	4	4
	//	4��5��     	4	5
	//	4��6��     	4	6
	//	4��7��     	4	7
	//	4��8��     	4	8
	//	4��9��     	4	8
	//	4��10��     	4	8
	//	4��11��     	4	8
	//	4��12��     	4	8
	//	5��0��     	5	10	9	0
	//	5��1��     	5	1
	//	5��2��     	5	2
	//	5��3��     	5	12	11	3
	//	5��4��     	5	4
	//	5��5��     	5	5
	//	5��6��     	5	12	11	6
	//	5��7��     	5	7
	//	5��8��     	5	8
	//	5��9��     	5	8
	//	5��10��     	5	8
	//	5��11��     	5	8
	//	5��12��     	5	8
	//	6��0��     	6	11	9	0
	//	6��1��     	6	11	9	1
	//	6��2��     	6	11	12	10	2
	//	6��3��     	6	3
	//	6��4��     	6	4
	//	6��5��     	6	11	12	5
	//	6��6��     	6	6
	//	6��7��     	6	7
	//	6��8��     	6	11	12	8
	//	6��9��     	6	11	12	8
	//	6��10��     	6	11	12	8
	//	6��11��     	6	11	12	8
	//	6��12��     	6	11	12	8
	//	7��0��     	7	11	9	0
	//	7��1��     	7	12	10	1
	//	7��2��     	7	12	10	2
	//	7��3��     	7	3
	//	7��4��     	7	4
	//	7��5��     	7	5
	//	7��6��     	7	6
	//	7��7��     	7	7
	//	7��8��     	7	8
	//	7��9��     	7	8
	//	7��10��     	7	8
	//	7��11��     	7	8
	//	7��12��     	7	8
	//	8��0��     	8	12	10	9	0
	//	8��1��     	8	12	10	1
	//	8��2��     	8	12	10	2
	//	8��3��     	8	12	11	3
	//	8��4��     	8	4
	//	8��5��     	8	5
	//	8��6��     	8	12	11	6
	//	8��7��     	8	7
	//	8��8��     	8	8
	//	8��9��     	8	8
	//	8��10��     	8	8
	//	8��11��     	8	8
	//	8��12��     	8	8
	//	9��0��     	9	8
	//	9��1��     	9	8
	//	9��2��     	9	8
	//	9��3��     	9	8
	//	9��4��     	9	8
	//	9��5��     	9	8
	//	9��6��     	9	8
	//	9��7��     	9	8
	//	9��8��     	9	8
	//	9��9��     	9	9
	//	9��10��     	9	10
	//	9��11��     	9	11
	//	9��12��     	9	11
	//	10��0��     	10	11
	//	10��1��     	10	11
	//	10��2��     	10	11
	//	10��3��     	10	11
	//	10��4��     	10	11
	//	10��5��     	10	11
	//	10��6��     	10	11
	//	10��7��     	10	11
	//	10��8��     	10	11
	//	10��9��     	10	9
	//	10��10��     	10	10
	//	10��11��     	10	12
	//	10��12��     	10	12
	//	11��0��     	11	12
	//	11��1��     	11	12
	//	11��2��     	11	12
	//	11��3��     	11	12
	//	11��4��     	11	12
	//	11��5��     	11	12
	//	11��6��     	11	12
	//	11��7��     	11	12
	//	11��8��     	11	12
	//	11��9��     	11	9
	//	11��10��     	11	10
	//	11��11��     	11	11
	//	11��12��     	11	12
	//	12��0��     	12	12
	//	12��1��     	12	12
	//	12��2��     	12	12
	//	12��3��     	12	12
	//	12��4��     	12	12
	//	12��5��     	12	12
	//	12��6��     	12	12
	//	12��7��     	12	12
	//	12��8��     	12	12
	//	12��9��     	12	9
	//	12��10��     	12	10
	//	12��11��     	12	11
	//	12��12��     	12	12


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


