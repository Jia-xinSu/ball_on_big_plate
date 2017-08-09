#ifndef __SIGNAL_STREAM
#define __SIGNAL_STREAM

#include "ebox.h"
#include "my_math.h"

//��circle bufferʵ�ֵ������źŴ�����
//�����źŴ洢��ͨ���̳л�ʵ����ʵ���źŴ�����
//ע��ģ����ĺ���ʵ�ֱ���д��ͷ�ļ���
template<typename T>
class SignalStream
{
protected:
	int indexHead;
	int length;
public:
	T* buf;

	//��̬�����ڴ棬��ʼ��bufΪ0
	SignalStream(int length);

	//�ͷ��ڴ�
	~SignalStream()
	{
		ebox_free(buf);
	}

	//���bufΪ0
	void clear();

	//����buf�ĳ���
	int getLength();

	//ѹ�����µ����ݲ�������ɵ�����
	void push(T signal);

	//�������ö��г���
	void setLength(int length);

	//��ȡ��ɵ�����
	float getOldest();

	//����[]����������0��ʼΪ���µ��ɵ�����
	T &operator [](int index);
};

template<typename T>
float SignalStream<T>::getOldest()
{
	return operator [](length - 1);
}

template<typename T>
void SignalStream<T>::setLength(int length)
{
	this->length = length;
	ebox_free(buf);
	buf = (T *)ebox_malloc(sizeof(T)*length);
	clear();
}

template<typename T>
SignalStream<T>::SignalStream(int length) :
	indexHead(0),
	length(length)
{
	buf = (T *)ebox_malloc(sizeof(T)*length);
	clear();
}

template<typename T>
void SignalStream<T>::clear()
{
	for (int i = 0; i < length; i++)
	{
		buf[i] = 0;
	}
}

template<typename T>
int SignalStream<T>::getLength()
{
	return length;
}

template<typename T>
void SignalStream<T>::push(T signal)
{
	indexHead--;
	indexHead %= length;
	if (indexHead < 0)
	{
		indexHead += length;
	}
	buf[indexHead] = signal;
}

template<typename T>
T & SignalStream<T>::operator[](int index)
{
	return buf[(index + indexHead) % length];
}

//��SignalStreamʵ�ֵľ�ֵ�˲��࣬����������
class AverageFilter :public SignalStream<float>
{
protected:
	float sumTemp;
public:
	AverageFilter(float sampleFrq,float stopFrq);
	float getFilterOut(float newNum);
};

class RcFilter
{
protected:
	float k;
	float sampleFrq;
	float yOld;

public:
	RcFilter(float sampleFrq, float stopFrq);
	
	//��ȡ�˲������
	float getFilterOut(float x);

	//���ò����ʺͽ�ֹƵ��
	void setParams(float sampleFrq, float stopFrq);

	//���ý�ֹƵ��
	void setStopFrq(float stopFrq);

	//�����һ�����
	void clear();
};

//z�任��ʾ����ɢϵͳ
class SysZ
{
public:
	virtual float getY(float x) = 0;
};

//ֻ������ϵͳ
class SysWithOnlyZero:public SysZ
{
	SignalStream<float> xn;
	float *args;
	int num;
public:
	//ֻ������ϵͳ
	//args��z^0, z^-1,z^-2...��ϵ��
	SysWithOnlyZero(float *args, int num):
		xn(num),
		args(args),
		num(num)
	{
		
	}

	//�������룬��ȡ���
	float getY(float x);

	//���ϵͳ��ʷ����
	void clear();
};

#endif
