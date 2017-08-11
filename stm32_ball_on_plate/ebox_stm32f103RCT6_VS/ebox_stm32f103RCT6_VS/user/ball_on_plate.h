#pragma once
#include "ebox.h"
#include "uart_num.h"
#include "my_math.h"
#include "signal_stream.h"
#include "PID.h"
#include "servo.h"
#include "ws2812.h"

////Ϊľ��ƽ��С����Ƶ�����PID
////������deadzone�У�����ٶ�С��vLim������I����
//
////��Ŀ���ٶ�С��vLim�����С��errLimʱ������I����
////������������ֵ��Сʱ������I����
////������������ֵ����ʱ������I����
//class PIDBallOnPlate:public PIDFeedforward, public PIDIncompleteDiff
//{
//public:
//	PIDBallOnPlate
//};

////Ϊľ��ƽ��С����Ƶ�����PID
////������deadzone�У�����ٶ�С��vLim������I����
class PIDBallOnPlate :public PIDFeforGshifIntIncDiff, public PIDDeadzone
{
	float vLim;
public:
	PIDBallOnPlate(float kp = 0, float ki = 0, float kd = 0,
		float interval = 0.01, float stopFrq = 50, float deadzone = 0, float vLim = 0) :
		PIDPosition(kp, ki, kd, interval),
		PIDFeforGshifIntIncDiff(kp, ki, kd, interval, stopFrq),
		PIDDeadzone(kp, ki, kd, interval, deadzone),
		vLim(vLim)
	{

	}

	virtual float refresh(float feedback)
	{
		float err;
		err = target - feedback;

		//��ʼʱʹ΢��Ϊ0������ͻȻ�ľ޴����΢��
		if (isBegin)
		{
			errOld = err;
			isBegin = false;
		}

		//���������deadzone�У�����ٶ�С��vLim������I���ƣ�����D����
		if ((abs(err) < deadzone) && abs(err - errOld) < vLim)
		{
			feedforward = feedforwardH.call(target);
			output = kp*err + integral + filter.getFilterOut(kd*0.5*(err - errOld))
				+ feedforward;//FunctionPointerδ��ʱĬ�Ϸ���0
		}
		else
		{
			//���������Χֹͣ���ּ�������
			if (((output > outputLimL && output < outputLimH) ||
				(output == outputLimH && err < 0) ||
				(output == outputLimL && err > 0))
				&& err - errOld < 0)
			{
				float ek = (err + errOld) / 2;
				integral += ki*fek(ek)*ek;
			}
			if (err>gearshiftPointL+ gearshiftPointH)
			{
				integral = 0;
			}
			feedforward = feedforwardH.call(target);
			output = kp*err + integral + filter.getFilterOut(kd*(err - errOld))
				+ feedforward;//FunctionPointerδ��ʱĬ�Ϸ���0
		}
		limit<float>(output, outputLimL, outputLimH);

		errOld = err;
		return output;
	}
};

//ƽ��С����࣬���Ϊ������
//�̶�io�����ã������led��
//��������PID���ƽӿڣ���ʹ�ܣ������ָ�ˮƽ��������Ŀ��ֵ��PID�����̶�
//�����������ݻ�ȡ�ӿڣ���ǰ���ꡢPID��ر��������ֵ��ˢ������
//���������жϣ�PID�����жϣ��ص������󶨽ӿ�
//��������״̬��ȡ�ӿڣ�С���Ƿ��ڰ��ϣ���ݮ���Ƿ��ڷ�����Ϣ
//������ݮ�ɻ��������ӿڣ��ػ���ֹͣ����
//���������þ�̬����
class BallOnPlateBase
{
	//��λ
	UartNum<float, 2> uartNum;
	static const float maxPos;
	float posX;
	float posY;
	bool isBallOnPlate;
	uint64_t lastPosReceiveTime;
	//PID
	static const float ratePID;
	static const float intervalPID;
	FpsCounter fpsPID;
	//ǰ��ϵͳ
	static const float gzDenominator;
	static const float feedforwardSysH[3];
	SysWithOnlyZero feedforwardSysX, feedforwardSysY;
	//pid����
	float targetXFiltered, targetYFiltered, targetXraw, targetYraw;
	static const float factorPID;
	PIDBallOnPlate pidX, pidY;
	RcFilter filterOutX, filterOutY, filterTargetX, filterTargetY;
	float outX, outY;
	//pid�ж��лص��ĺ���ָ��
	FunctionPointerArg1<void, void> afterPIDEvent;
	//����
	Servo servoX, servoY;
	//����
	WS2812 ws2812;

	//�յ���λ������������PID����
	void posReceiveEvent(UartNum<float, 2>* uartNum)
	{
		lastPosReceiveTime = millis();

		//�Զ�λPID��Ŀ����������˲�
		targetXFiltered = filterTargetX.getFilterOut(targetXraw);
		targetYFiltered = filterTargetY.getFilterOut(targetYraw);
		pidX.setTarget(targetXFiltered);
		pidY.setTarget(targetYFiltered);

		if (uartNum->getLength() == 2)
		{
			posX = uartNum->getNum()[0];
			posY = uartNum->getNum()[1];

			if (!isnan(posX) && !isnan(posY))
			{
				if (isBallOnPlate == false)
				{
					pidX.resetState();
					pidY.resetState();

					isBallOnPlate = true;
				}

				outX = 0, outY = 0;
				outX += pidX.refresh(posX);
				outY -= pidY.refresh(posY);

				outX = filterOutX.getFilterOut(outX);
				outY = filterOutY.getFilterOut(outY);
			}
			else
			{
				outX = 0; outY = 0;
				isBallOnPlate = false;
			}
		}
		servoX.setPct(outX);
		servoY.setPct(outY);

		fpsPID.getFps();

		afterPIDEvent.call();
	}

public:
	BallOnPlateBase() :
		//��λ
		uartNum(&uart2),
		posX(-1), posY(-1),
		isBallOnPlate(true),
		lastPosReceiveTime(0),
		//ǰ��ϵͳ
		feedforwardSysX((float*)feedforwardSysH, 3), feedforwardSysY((float*)feedforwardSysH, 3),
		targetXFiltered(maxPos / 2), targetYFiltered(maxPos / 2), targetXraw(targetXFiltered), targetYraw(targetYFiltered),
		//pid����
		pidX(0.3f*factorPID, 0.5f*factorPID, 0.16f*factorPID, 1.f / ratePID, 5, 6, 2),
		pidY(0.3f*factorPID, 0.5f*factorPID, 0.16f*factorPID, 1.f / ratePID, 5, 6, 2),
		filterOutX(ratePID, 15), filterOutY(ratePID, 15), filterTargetX(ratePID, 0.5), filterTargetY(ratePID, 0.5),
		servoX(&PB9, 200, 0.75, 2.05), servoY(&PB8, 200, 0.85, 2.15),
		//����
		ws2812(&PB0)
	{

	}

	//��ʼ��PID����������λ������
	void begin()
	{
		//PID
		fpsPID.begin();
		pidX.setTarget(maxPos / 2);
		pidX.setOutputLim(-100, 100);
		pidX.setGearshiftPoint(20, 100);
		pidX.attachFeedForwardH(&feedforwardSysX, &SysWithOnlyZero::getY);

		pidY.setTarget(maxPos / 2);
		pidY.setOutputLim(-100, 100);
		pidY.setGearshiftPoint(20, 100);
		pidY.attachFeedForwardH(&feedforwardSysY, &SysWithOnlyZero::getY);

		//����
		servoX.begin();
		servoX.setPct(0);
		servoY.begin();
		servoY.setPct(0);

		//��λ
		uartNum.begin(115200);

		//����
		ws2812.begin();
		ws2812.setAllDataHSV(60, 0, 0.7);

		uartNum.attach(this, &BallOnPlateBase::posReceiveEvent);
	}

	//��ȡλ�õ����ֵ���������������εĳ���
	float getPosMax()
	{
		return maxPos;
	}

	//��ȡPIDˢ�¼��
	float getIntervalPID()
	{
		return intervalPID;
	}

	//����Ŀ��ֵ
	void setTarget(float x, float y)
	{
		targetXraw = x;
		targetYraw = y;
	}
	//��ȡ�˲���Ŀ��ֵ
	void getTargetFiltered(float *x, float *y)
	{
		*x = targetXFiltered;
		*y = targetYFiltered;
	}
	//��ȡԭʼĿ��ֵ
	void getTargetRaw(float *x, float *y)
	{
		*x = targetXraw;
		*y = targetYraw;
	}

	//����Ŀ��X
	void setTargetX(float x)
	{
		targetXraw = x;
	}
	//����Ŀ��Y
	void setTargetY(float y)
	{
		targetYraw = y;
	}
	//��ȡԭʼĿ��X
	float getTargetXraw()
	{
		return targetXraw;
	}
	//��ȡԭʼĿ��Y
	float getTargetYraw()
	{
		return targetYraw;
	}
	//��ȡ�˲���Ŀ��X
	float getTargetXFiltered()
	{
		return targetXFiltered;
	}
	//��ȡ�˲���Ŀ��Y
	float getTargetYFiltered()
	{
		return targetYFiltered;
	}

	//��ȡ��ǰ����
	void getPos(float* x, float *y)
	{
		*x = posX;
		*y = posY;
	}
	//��ȡ��ǰX����
	float getPosX()
	{
		return posX;
	}
	//��ȡ��ǰY����
	float getPosY()
	{
		return posY;
	}

	//��ȡ������
	void getOut(float* x, float* y)
	{
		*x = outX;
		*y = outY;
	}
	//��ȡ��ǰX����
	float getOutX()
	{
		return outX;
	}
	//��ȡ��ǰY����
	float getOutY()
	{
		return outY;
	}

	//��ȡǰ��������
	void getFeedforward(float* x, float* y)
	{
		*x = pidX.getFeedforward();
		*y = pidY.getFeedforward();
	}
	//��ȡXǰ��������
	float getFeedforwardX()
	{
		return pidX.getFeedforward();
	}
	//��ȡYǰ��������
	float getFeedforwardY()
	{
		return pidY.getFeedforward();
	}

	//С���Ƿ��ڰ���
	bool getIsBallOn()
	{
		return isBallOnPlate;
	}
	
	//��ݮ���Ƿ��ڷ�����Ϣ
	bool getIsPosReceiving()
	{
		if (millis() - lastPosReceiveTime < 1000)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	//��pid�ж��лص��ĺ���
	void attachAfterPIDEvent(void(*afterPIDEvent)(void))
	{
		this->afterPIDEvent.attach(afterPIDEvent);
	}

	//��pid�ж��лص��ĺ���
	template<typename T>
	void attachAfterPIDEvent(T *pObj, void (T::*afterPIDEvent)(void))
	{
		this->afterPIDEvent.attach(pObj, afterPIDEvent);
	}

	//��ȡˢ����
	float getFps()
	{
		return fpsPID.getOldFps();
	}

	//�ر���ݮ��
	void shutdownRasp()
	{
		uartNum.printf("s");
	}

};

//��λ
const float BallOnPlateBase::maxPos = 600;
//PID
const float BallOnPlateBase::ratePID = 32;
const float BallOnPlateBase::intervalPID = 1 / ratePID;
//ǰ��ϵͳ
const float BallOnPlateBase::gzDenominator = 
0.6125* intervalPID *intervalPID / (4 / 3 * M_PI / 200);//����һ������ٷֱȵ�������ȵĵ�λϵ��
const float BallOnPlateBase::feedforwardSysH[3] = 
{ 1 / gzDenominator,-2 / gzDenominator,1 / gzDenominator };
//pid����
const float BallOnPlateBase::factorPID = 3.7;
