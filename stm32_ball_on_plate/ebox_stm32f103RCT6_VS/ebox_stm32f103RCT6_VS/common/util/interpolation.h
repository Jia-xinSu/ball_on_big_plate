
#ifndef  INTERPOLATION_H
#define  INTERPOLATION_H
#include"ebox.h"
class Interpolation
{
public:
	//���캯����������������������������׵�ַ�����鳤��
	Interpolation(float *x, float *y, int n);
	//������������������������׵�ַ�����鳤��
	void setSamplePoint(float *x, float *y, int n);
	//�ö��ֲ��Ҳ�������x����������,��������
	int search(float x);
	//���麯������ȡ��ֵ�����������µ�ֵ
	virtual float getY(float x) = 0;
	//��ȡmeasureY
private:
	float *xaxis;
	float *yaxis;
	int length;

};


class LinearInterpolation :public Interpolation
{
public:
	LinearInterpolation(float *x, float *y, int n);
	float getY(float x);

private:
	float *xaxis;
	float *yaxis;
	int length;
};



class QuadraticInterpolation :public Interpolation
{
public:
	//���캯��
	QuadraticInterpolation(float *x, float *y, int n);
	//������ി�麯��
	float getY(float x);

private:
	float *xaxis;
	float *yaxis;
	int length;

};


class Interpolation2D
{
protected:
	float *x, *y, *z;
	int lengthX, lengthY;
public:
	Interpolation2D(float *x, float *y, float *z, int lengthX, int lengthY);

	void setSamplePoint(float *x, float *y, float *z, int lengthX, int lengthY);

	//Ѱ�Ҳ�ֵ���X���Y���ƫ����
	int search1D(float *xaxis, float x, int length);

	//Ѱ�Ҳ�ֵ��Zֵ
	float getZ(float x, float y);

	//��ȡx��ȡֵ��Χ
	void getXLim(float *limL, float *limH)
	{
		*limL = x[0];
		*limH = x[lengthX - 1];
	}

	//��ȡy��ȡֵ��Χ
	void getYLim(float *limL, float *limH)
	{
		*limL = y[0];
		*limH = y[lengthY - 1];
	}
};

#endif