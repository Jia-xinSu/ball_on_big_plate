#pragma once
#include "ebox.h"
#include "uart_num.h"
#include "my_math.h"
#include "signal_stream.h"
#include "PID.h"
#include "servo.h"
#include "ws2812.h"
#include <math.h>

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
protected:
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

protected:
	//�յ���λ������������PID����
	virtual void posReceiveEvent(UartNum<float, 2>* uartNum)
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
		posX(0.0/0.0), posY(0.0/0.0),
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
	virtual void begin()
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


//·�����±�����
class BallOnPlatePathIndex
{
protected:
	int pathLength;//·���ĳ���
public:
	int pathPointIndex[128];//�洢·�����±�

	BallOnPlatePathIndex() 
	{
	}

	//��ȡ��src��dstԲ��������·��
	//pathPointIndex����ָ��ָ��·���ı��
	//0~8����1~9��Բ��9~12����1~4�Ű�ȫ����
	//pathLength����·���ĳ���
	void computePathPoint(int src, int dst)
	{
		pathPointIndex[0] = src;
		
		int srcX = src % 3,srcY=src/3;
		int dstX = dst % 3, dstY = dst / 3;
		int disX = abs(srcX - dstX), disY = abs(srcY - dstY);

		//���src��dst���ڣ������Խ��ߣ�����ͬ
		if (disX <= 1 && disY <= 1)
		{
			pathPointIndex[1] = dst;
			pathLength = 2;
		}
		//��������������Ϊ2
		else if (disX == 2 && disY == 2)
		{
			//�����0��ʼ
			if (src == 0)
			{
				pathPointIndex[1] = 9; 
				pathPointIndex[2] = 11; 
				pathPointIndex[3] = 12;
			}
			//�����6��ʼ
			else if (src == 6)
			{
				pathPointIndex[1] = 11;
				pathPointIndex[2] = 12;
				pathPointIndex[3] = 10;
			}
			//�����8��ʼ
			else if (src == 8)
			{
				pathPointIndex[1] = 12;
				pathPointIndex[2] = 10;
				pathPointIndex[3] = 9;
			}
			//�����2��ʼ
			else if (src == 2)
			{
				pathPointIndex[1] = 10;
				pathPointIndex[2] = 9;
				pathPointIndex[3] = 11;
			}
			pathPointIndex[4] = dst;
			pathLength = 5;
		}
		//���ֻ��һ���������Ϊ2
		else
		{
			//���·������ˮƽ
			if (disY <= 1)
			{
				int safeY = (srcY + dstY) / 2;
				limit(safeY, 0, 1);
				//����������
				if (srcX < dstX)
				{
					pathPointIndex[1] = 9 + safeY * 2;
					pathPointIndex[2] = 9 + safeY * 2 + 1;
				}
				//����������
				else
				{
					pathPointIndex[1] = 9 + safeY * 2 + 1;
					pathPointIndex[2] = 9 + safeY * 2;
				}
			}
			//���·��������ֱ
			else
			{
				int safeX = (srcX + dstX) / 2;
				limit(safeX, 0, 1);
				//����������
				if (srcY < dstY)
				{
					pathPointIndex[1] = 9 + safeX;
					pathPointIndex[2] = 9 + safeX + 2;
				}
				//����������
				else
				{
					pathPointIndex[1] = 9 + safeX + 2;
					pathPointIndex[2] = 9 + safeX;
				}
			}
			pathPointIndex[3] = dst;
			pathLength = 4;
		}
	}


	//��ȡ·������
	int getPathLength()
	{
		return pathLength;
	}

	//��ȡ·�������
	int getSrc()
	{
		return pathPointIndex[0];
	}

	//��ȡ·�����յ�
	int getDst()
	{
		return pathPointIndex[pathLength - 1];
	}

};

//һά·��������
//���ɹ涨ʱ���ڴ�x��y������·��
class SpeedControl
{
	//����ʼĩ��ֵ
	void setBeginEnd(float x, float y)
	{
		this->x = x;
		this->y = y;
	}

	//����ʱ�������ٶ�
	void setSpeed(float v)
	{
		v = abs(v);
		setTime(abs(y - x) / v);
	}

	//����ʱ������·����ʱ��
	void setTime(float time)
	{
		stepAll = time / interval;
		stepNow = 0;
	}

protected:
	float x, y, interval;
	int stepAll, stepNow;

public:
	SpeedControl(float interval):
		interval(interval),
		stepAll(0),
		stepNow(0)
	{

	}

	//����·�����յ�����ʱ��
	virtual void setPathTime(float x, float y, float time)
	{
		setBeginEnd(x, y);
		setTime(time);
	}

	//����·�����յ������ٶ�
	virtual void setPathSpeed(float x, float y, float speed)
	{
		setBeginEnd(x, y);
		setSpeed(speed);
	}

	//��ȡ·������һ��
	//����true��ʾ���·��
	virtual bool getNext(float *next)
	{
		bool isEnd=false;
		stepNow += 1;
		if (stepNow > stepAll)
		{
			stepNow = stepAll;
			isEnd = true;
		}
		*next = x + (y - x)*stepNow / stepAll;
		return isEnd;
	}

	//�Ƿ��յ�
	virtual bool isEnd()
	{
		if (stepNow >= stepAll)
		{
			return true;
		}
		else 
		{
			return false;
		}
	}
};

class SpeedControlSoft:public SpeedControl
{
protected:
	RcFilter filter;
public:
	SpeedControlSoft(float interval) :
		SpeedControl(interval),
		filter(1 / interval, 1)
	{

	}
	
	//����·�����յ�����ʱ��
	virtual void setPathTime(float x, float y, float time)
	{
		filter.setInit(x);
		SpeedControl::setPathTime(x, y, time);
	}

	//����·�����յ������ٶ�
	virtual void setPathSpeed(float x, float y, float speed)
	{
		filter.setInit(x);
		SpeedControl::setPathSpeed(x, y, speed);
	}

	//��ȡ·������һ��
	//����true��ʾ���·��
	virtual bool getNext(float *output)
	{
		bool isEnd = SpeedControl::getNext(output);
		*output = filter.getFilterOut(*output);
		return isEnd;
	}

	//�����˲�����ֹƵ��
	void setStopFre(float fre)
	{
		filter.setStopFrq(fre);
	}
};

//��������·����ȡ������
class BallOnPlatePath :protected BallOnPlatePathIndex
{
	//�л�����һ��·����
	//����true��ʾ����
	bool moveOn()
	{
		//�����򷵻�
		if (pathIndex >= getPathLength() - 1)
		{
			return true;
		}

		//��ȡʵ������ֵ
		float srcX, srcY, dstX, dstY;
		int srcInd = pathIndex, dstInd = pathIndex + 1;
		int srcPoint = pathPointIndex[srcInd]
			, dstPoint = pathPointIndex[dstInd];
		if (srcPoint < 9)
		{
			srcX = circleX[srcPoint]; srcY = circleY[srcPoint];
		}
		else
		{
			srcX = safeX[srcPoint % 9]; srcY = safeY[srcPoint % 9];
		}
		if (dstPoint < 9)
		{
			dstX = circleX[dstPoint]; dstY = circleY[dstPoint];
		}
		else
		{
			dstX = safeX[dstPoint % 9]; dstY = safeY[dstPoint % 9];
		}
		float disY = abs(dstY - srcY), disX = abs(dstX - srcX);
		float speedRatio = sqrt(disX*disX + disY*disY);
		float speedX = disX / speedRatio*speed;
		float speedY = disY / speedRatio*speed;
		generatorX.setPathSpeed(srcX, dstX, speedX);
		generatorY.setPathSpeed(srcY, dstY, speedY);

		pathIndex++;

		return false;
	}
protected:
	static const float circleX[9], circleY[9];//9��Բ������
	float safeX[4], safeY[4];//4����ȫԲ������
	SpeedControl generatorX, generatorY;//����xy����һάĿ������
	int pathIndex;//��־��ǰ��BallOnPlatePathIndex��·�����±꣬��Χ0 ~ getPathLength()-2
	float speed;
public:
	BallOnPlatePath(float interval) :
		BallOnPlatePathIndex(),
		generatorX(interval), generatorY(interval),
		pathIndex(0),
		speed(1)
	{
		//���㰲ȫԲ������
		for (int i = 0; i < 4; i++)
		{
			int leftTopIndex = i / 2 * 3 + i % 2;
			int rightTopIndex = leftTopIndex + 1;
			int leftBotomIndex = leftTopIndex + 3;
			int rightBotomIndex = leftTopIndex + 4;
			safeX[i] = (circleX[leftTopIndex] + circleX[rightTopIndex]
				+ circleX[leftBotomIndex] + circleX[rightBotomIndex]) / 4;
			safeY[i] = (circleY[leftTopIndex] + circleY[rightTopIndex]
				+ circleY[leftBotomIndex] + circleY[rightBotomIndex]) / 4;
		}
	}

	//����·��������յ���ٶ�
	virtual void setPathTime(int src, int dst, float speed)
	{
		this->speed = speed;
		pathIndex = 0;
		computePathPoint(src, dst);
	}

	//��ȡ��һ������꣬�����Ƿ����
	virtual bool getNext(float *x, float *y)
	{
		//���xy���򶼵����յ㣬������һ��·��
		if (generatorX.isEnd() && generatorY.isEnd())
		{
			if (moveOn())
			{
				return true;
			}
		}
		generatorX.getNext(x);
		generatorY.getNext(y);
	}

	bool isEnd()
	{
		if (pathIndex >= getPathLength() - 1)
		{
			return true;
		}
		else {
			return false;
		}
	}

};
class BallOnPlatePathSoft :public BallOnPlatePath
{
protected:
	RcFilter filterX, filterY;
public:
	BallOnPlatePathSoft(float interval, float stopFre = 0.5) :
		BallOnPlatePath(interval),
		filterX(1/ interval, stopFre), filterY(1 / interval, stopFre)
	{

	}

	//�����˲����Ľ�ֹƵ��
	void setStopFre(float stopFre)
	{
		filterX.setStopFrq(stopFre);
		filterY.setStopFrq(stopFre);
	}

	//����·��������յ���ٶ�
	virtual void setPathSpeed(int src, int dst, float speed)
	{
		BallOnPlatePath::setPathTime(src, dst, speed);
		
		//��ȡ���ʵ������ֵ
		float srcX, srcY;
		srcX = circleX[src]; srcY = circleY[src];

		//�����˲�����ʼֵ
		filterX.setInit(srcX);
		filterY.setInit(srcY);
	}

	//��ȡ��һ������꣬�����Ƿ����
	virtual bool getNext(float *x, float *y)
	{
		float xRaw, yRaw;
		bool isEnd = BallOnPlatePath::getNext(&xRaw, &yRaw);
		if (isEnd==false)
		{
			*x = filterX.getFilterOut(xRaw);
			*y = filterY.getFilterOut(yRaw);
		}
		else
		{
			//��ȡ�յ�ʵ������ֵ
			float dstX, dstY;
			dstX = circleX[getDst()]; dstY = circleY[getDst()];

			*x = filterX.getFilterOut(dstX);
			*y = filterY.getFilterOut(dstY);
		}

		return isEnd;
	}
};

const float BallOnPlatePath::circleX[9] = {
	94.8847,298.129,498.131,
	93.1718,299.301,507.667,
	97.8807,300.759,504.591
};
const float BallOnPlatePath::circleY[9] = {
	106.717,98.0838,98.9344,
	303.629,301.297,298.753,
	504.300,505.724,495.129
};


class BallOnPlate:public BallOnPlateBase
{
protected:
	BallOnPlatePath path;
public:
	BallOnPlate() :
		path(getIntervalPID())
	{

	}


	//����·��
	void startPath(int src, int dst, float speed)
	{
		path.setPathTime(src, dst, speed);
	}

	//ˢ��·��
	bool refreshPath()
	{
		float x, y;
		bool isEnd = path.getNext(&x, &y);
		setTarget(x, y);
		return isEnd;
	}

	//PID�жϺ��������·��ˢ�¶�Ŀ��������
	virtual void posReceiveEvent(UartNum<float, 2>* uartNum)
	{
		if (!path.isEnd())
		{
			refreshPath();
		}
		BallOnPlateBase::posReceiveEvent(uartNum);
	}
};
