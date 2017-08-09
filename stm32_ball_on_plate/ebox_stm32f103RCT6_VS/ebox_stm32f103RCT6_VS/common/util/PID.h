#ifndef  __PID
#define  __PID

#include <limits>
#include "my_math.h"
#include "signal_stream.h"
#include <math.h>
#include "FunctionPointer.h"
#include "interpolation.h"


//PID����
class PID
{
protected:
	bool isBegin;
	float kp, ki, kd;
	float interval;
	float outputLimL, outputLimH;
	float target;
	float errOld;
	float output;
public:
	PID(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//���ò������ʱ�䣬��λs
	//����setPIDǰ���������ú�interval
	void setInterval(float interval);
	//��ȡ���ʱ��
	float getInterval();

	//����PID������setPIDǰ���������ú�interval
	void setPID(float kp, float ki, float kd);
	//��ȡPID
	virtual void getPID(float *kp, float *ki, float *kd);

	//ͬʱ����interval��PID
	void setBasic(float kp, float ki, float kd, float interval);
	
	//���������Χ��������Χֹͣ���ּ�������
	void setOutputLim(float limL, float limH);

	//���ÿ���Ŀ��
	void setTarget(float target);
	//��ȡ����Ŀ��
	float getTarget();

	//����ݴ���
	virtual void resetState();

	//��ʵ�ֵ�PID�㷨
	virtual float refresh(float feedback) = 0;

};

//��ͨ���λ���PID����
class PIDPosition : public PID
{
protected:
	float integral;
public:
	//��ͨ���λ���PID�㷨
	PIDPosition(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//��ͨPID���㷨
	virtual float refresh(float feedback);

	//�������������ݴ���
	virtual void resetState();
};

//��ͨ����PID����
class PIDIncrease : public PID
{
protected:
	float deltaErrOld;
public:
	//����PID�㷨
	PIDIncrease(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//����PID���㷨
	virtual float refresh(float feedback);

	//����ݴ���
	virtual void resetState();
};

//���ַ���PID
class PIDIntegralSeperate :virtual public PIDPosition
{
protected:
	float ISepPoint;
public:
	//���ַ���PID�㷨
	PIDIntegralSeperate(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//���û��ַ����
	void setISepPoint(float ISepPoint);

	//���ַ���PID�㷨
	virtual float refresh(float feedback);
};

//����ȫ΢��PID
class PIDIncompleteDiff :virtual public PIDPosition
{
protected:
	RcFilter filter;
public:
	//����ȫ΢��PID�㷨
	PIDIncompleteDiff(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01, float stopFrq = 50);

	//����ȫ΢��PID�㷨
	virtual float refresh(float feedback);
};

//���ַ��벻��ȫ΢��PID
class PIDIntSepIncDiff : public PIDIntegralSeperate, public PIDIncompleteDiff
{
public:
	//���ַ��벻��ȫ΢��PID�㷨
	PIDIntSepIncDiff(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01, float stopFrq = 50);

	//���ַ��벻��ȫ΢��PID�㷨
	virtual float refresh(float feedback);
};

//���ٻ���PID
class PIDGearshiftIntegral :virtual public PIDPosition
{
protected:
	float gearshiftPointL, gearshiftPointH;
	float fek(float ek);
public:
	//���ٻ���PID�㷨
	//ui(k)=ki*{sum 0,k-1 e(i)+f[e(k)]e(k)}*T
	//f[e(k)]= 	{	1						,|e(k)|<=B
	//					[A-|e(k)|+B]/A	,B<|e(k)|<=A+B
	//					0						,|e(k)|>A+B
	PIDGearshiftIntegral(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//���ñ��ٻ��ּ�Ȩ���߲���
	void setGearshiftPoint(float pointL, float pointH);

	//���ٻ���PID�㷨
	virtual float refresh(float feedback);

};

//���ٻ��ֲ���ȫ΢��PID
class PIDGshifIntIncDiff:public PIDGearshiftIntegral,public PIDIncompleteDiff
{
public:
	//���ٻ��ֲ���ȫ΢��PID�㷨
	PIDGshifIntIncDiff(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01, float stopFrq = 50);

	//���ٻ��ֲ���ȫ΢��PID�㷨
	virtual float refresh(float feedback);
};

//΢������PID
class PIDDifferentialAhead :virtual public PIDPosition
{
	float feedbackOld;
public:
	//΢������PID�㷨
	PIDDifferentialAhead(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//΢������PID�㷨
	virtual float refresh(float feedback);
};

//ǰ������PID
class PIDFeedforward :virtual public PIDPosition
{
protected:
	FunctionPointerArg1<float, float> feedforwardH;
	float feedforward;
public:
	//ǰ������PID�㷨
	PIDFeedforward(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//��ϵͳ����feedforwardH������Ϊʱ����ɢ�źţ�����ϵͳ����ź�
	void attachFeedForwardH(float(*feedforwardH)(float input));

	//��ϵͳ����feedforwardH������Ϊʱ����ɢ�źţ�����ϵͳ����ź�
	template<typename T>
	void attachFeedForwardH(T *pObj, float (T::*feedforwardH)(float input));

	virtual float refresh(float feedback);

	//��ȡ��ǰǰ������ֵ
	float getFeedforward();
};
template<typename T>
void PIDFeedforward::attachFeedForwardH(T *pObj, float (T::*feedforwardH)(float input))
{
	this->feedforwardH.attach(pObj, feedforwardH);
}

//ǰ���������ٻ��ֲ���ȫ΢��PID
class PIDFeforGshifIntIncDiff :public PIDFeedforward, public PIDGearshiftIntegral, public PIDIncompleteDiff
{
public:
	PIDFeforGshifIntIncDiff(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01, float stopFrq = 50);

	virtual float refresh(float feedback);
};

//��������PID
class PIDDeadzone:virtual public PIDPosition
{
protected:
	float deadzone;
public:
	PIDDeadzone(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01, float deadzone = 0);

	virtual float refresh(float feedback);
};

//��������ǰ���������ٻ��ֲ���ȫ΢��PID
class PIDFeforGshifIntIncDiffDezone:public PIDFeforGshifIntIncDiff,public PIDDeadzone
{
public:
	PIDFeforGshifIntIncDiffDezone(float kp = 0, float ki = 0, float kd = 0, 
		float interval = 0.01, float stopFrq = 50, float deadzone = 0);

	virtual float refresh(float feedback);
};

//ר��PID
class PIDExpert :virtual public PIDPosition
{
	float deltaErrOld;
	u8 rule;//ָʾ��ǰ������������
	//��Ӧ���Ƚ�PID���Ƽ���MATLAB���桷�е����������
	float m1, m2, k1, k2, e;
public:
	//ר��PID�㷨
	PIDExpert(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01):
		PIDPosition(kp, ki, kd, interval),
		rule(0)
	{
		resetState();
		setParams(0, 0, 0, 0, 0);
	}

	//����ר��PID���������
	//m1����Ϊ�ܴ��������m1��������
	//m2����Ϊ�ϴ����m2<m1
	//k1������Ŵ�ϵ����k1>1
	//k2������ϵ����0<k2<1
	//e����Ϊ��С�����
	void setParams(float m1, float m2, float k1, float k2, float e)
	{
		this->m1 = m1;
		this->m2 = m2;
		this->k1 = k1;
		this->k2 = k2;
		this->e = e;
	}

	//ר��PID�㷨
	virtual float refresh(float feedback)
	{
		//����err��deltaErr
		//������Ƶı�������errOld��deltaErrOld
		float err;
		err = target - feedback;
		//��ʼʱʹ΢��Ϊ0������ͻȻ�ľ޴����΢��
		if (isBegin)
		{
			errOld = err;
			isBegin = false;
		}
		float deltaErr = err - errOld;

		rule = 0;

		//1. ������ֵ�Ѿ��ܴ�Ӧ���������С��������������ƣ�
		if (abs(err) > m1)
		{
			output = err > 0 ? outputLimH : outputLimL;
			rule = 1;
		}

		//2. ����ڳ�������ֵ����ķ���仯�������Ϊ����
		else if (err*deltaErr > 0 || deltaErr == 0)
		{
			//������ֵ�ϴ󣬿��Կ���ʵʩ��ǿ�Ŀ������ã�
			//��Ťת��������ֵ��С�ķ���
			if (abs(err) >= m2)
			{
				output = k1*kp*err;
			}
			//������ֵ�����󣬿���ʵʩһ��Ŀ������ã�
			//��Ťת��������ֵ��С�ķ���
			else
			{
				output = k2*kp*err;
			}
			rule = 2;
		}

		//3. ����ڳ�������ֵ��С�ķ���仯����ﵽƽ��
		//���Ǳ����������
		else if (
			((err*deltaErr < 0) && (deltaErr*deltaErrOld > 0))
			|| (err == 0)
			)
		{
			//���������Χֹͣ���ּ�������
			if ((output > outputLimL && output < outputLimH) ||
				(output == outputLimH && err < 0) ||
				(output == outputLimL && err > 0))
			{
				integral += ki*(err + errOld) / 2;
			}
			output = kp*err + integral + kd*deltaErr;
			rule = 3;
		}

		//4. ���ڼ�ֵ״̬
		else if ((err*deltaErr < 0) && (deltaErr*deltaErrOld < 0))
		{
			//������ֵ�ϴ󣬿��Կ���ʵʩ��ǿ�Ŀ�������
			if (abs(err) >= m2)
			{
				output = k1*kp*errOld;
			}
			else
			{
				output = k2*kp*errOld;
			}
			rule = 4;
		}

		//5. ������ֵ��С
		if (abs(err) <= e)
		{
			//���������Χֹͣ���ּ�������
			if ((output > outputLimL && output < outputLimH) ||
				(output == outputLimH && err < 0) ||
				(output == outputLimL && err > 0))
			{
				integral += ki*(err + errOld) / 2;
			}
			output = kp*err + integral;
			rule = 5;
		}

		limit<float>(output, outputLimL, outputLimH);
		errOld = err;
		deltaErrOld = deltaErr;
		return output;
	}

	void resetState()
	{
		deltaErrOld = 0;
		PIDPosition::resetState();
	}

	u8 getCurrentRule()
	{
		return rule;
	}
};

//ģ������Ӧ����PID
class PIDFuzzy :virtual public PIDPosition
{
protected:
	float kpF, kiF, kdF;
	Interpolation2D *fuzzyTableKp, *fuzzyTableKi, *fuzzyTableKd;
	void refreshPID(float err,float deltaErr)
	{
		///ͨ��ģ�����Ʋ�ѯ���ֵ�ó�kp��ki��kd�ĵ�����
		float x, y;
		float limL, limH;

		//��ȡdeltaKp
		x = err; y = deltaErr;
		fuzzyTableKp->getXLim(&limL, &limH);
		limit(x, limL, limH);
		fuzzyTableKp->getYLim(&limL, &limH);
		limit(y, limL, limH);
		float deltaKp = fuzzyTableKp->getZ(x, y);

		//��ȡdeltaKi
		x = err; y = deltaErr;
		fuzzyTableKi->getXLim(&limL, &limH);
		limit(x, limL, limH);
		fuzzyTableKi->getYLim(&limL, &limH);
		limit(y, limL, limH);
		float deltaKi = fuzzyTableKi->getZ(x, y);

		//��ȡdeltaKd
		x = err; y = deltaErr;
		fuzzyTableKd->getXLim(&limL, &limH);
		limit(x, limL, limH);
		fuzzyTableKd->getYLim(&limL, &limH);
		limit(y, limL, limH);
		float deltaKd = fuzzyTableKd->getZ(x, y);

		//������PID
		kpF = kp + deltaKp;
		kiF = ki + deltaKi * interval;
		kdF = kd + deltaKd / interval;
		limitLow<float>(kpF, 0);
		limitLow<float>(kiF, 0);
		limitLow<float>(kdF, 0);
	}
public:
	PIDFuzzy(float kp = 0, float ki = 0, float kd = 0, float interval = 0.01);

	//����ģ�����Ʋ�ѯ��
	void setFuzzyTable(
		Interpolation2D *fuzzyTableKp
		, Interpolation2D *fuzzyTableKi
		, Interpolation2D *fuzzyTableKd)
	{
		this->fuzzyTableKp = fuzzyTableKp;
		this->fuzzyTableKi = fuzzyTableKi;
		this->fuzzyTableKd = fuzzyTableKd;
	}

	float refresh(float feedback)
	{
		float err;
		err = target - feedback;
		float deltaErr = err - errOld;


		//��ʼʱʹ΢��Ϊ0������ͻȻ�ľ޴����΢��
		if (isBegin)
		{
			errOld = err;
			isBegin = false;
		}

		refreshPID(err, deltaErr);

		//���������Χֹͣ���ּ�������
		if ((output > outputLimL && output < outputLimH) ||
			(output == outputLimH && err < 0) ||
			(output == outputLimL && err > 0))
		{
			integral += kiF*(err + errOld) / 2;
		}
		output = kpF*err + integral + kdF*(err - errOld);
		limit<float>(output, outputLimL, outputLimH);

		errOld = err;
		return output;
	}

	//��ȡģ������Ӧ���PIDֵ
	void getPID(float *kp, float *ki, float *kd)
	{
		*kp = this->kpF;
		*ki = this->kiF / interval;
		*kd = this->kdF*interval;
	}
};

//�ظ�����������PID����ʹ��
//������PID֮ǰ����ʵ�ʲ���ֵ������뵽PID��
class RepetitiveController
{
protected:
	SignalStream<float> err;
	RcFilter filter;
	float k;
	int forwardCorrect;
public:
	//�ظ�����������PID����ʹ��
	//������PID֮ǰ����ʵ�ʲ���ֵ������뵽PID��
	//length���Ŷ�������źŵ�����
	//k���������
	//frq��fh�������ͨ�˲����Ĳ���
	//Sz���ظ��������Ĳ���ϵͳ��Ϊ�ܿض������
	RepetitiveController(int length,int forwardCorrect, float k, float frq, float fh);

	//�����ʷ���
	void clear();

	//��������ź�
	float refresh(float feedback);

	//��������
	void setLength(int length);

};


#endif
