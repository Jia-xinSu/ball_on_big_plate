#include <ctime>
#include <iostream>
#include <stdio.h>
#include <raspicam_cv.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>

#include "SocketMatTransmissionClient.h"  

#include "my_opencv.h"
#include "my_math.h"
#include <algorithm>
#include "uart_num.h"

#include <string>


using namespace cv;
using namespace std;

#define STDIO_DEBUG
#define SOCKET_SEND_IMAGE


int main(int argc, char **argv)
{
	///��������
	//����ͷ
	raspicam::RaspiCam_Cv cam;
	//��������
	cam.set(CV_CAP_PROP_FORMAT, CV_8UC1);
	cam.set(CV_CAP_PROP_FRAME_WIDTH, cam.get(CV_CAP_PROP_FRAME_WIDTH) * 1);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, cam.get(CV_CAP_PROP_FRAME_HEIGHT) * 1);
	int imRawH = cam.get(CV_CAP_PROP_FRAME_HEIGHT),
		imRawW = cam.get(CV_CAP_PROP_FRAME_WIDTH);
	Mat imRaw;
	
#ifdef SOCKET_SEND_IMAGE
	//����ͼ�����ڲ���
	Mat imSend;
#endif // SOCKET_SEND_IMAGE
	
	
	
	///��ʼ��
	//��ʼ������
	if (!cam.open())
		return 1;

#ifdef SOCKET_SEND_IMAGE
	cout << "socket connecting..." << endl;
	SocketMatTransmissionClient socketMat;
	socketMat.begin("192.168.2.100");
#endif // SOCKET_SEND_IMAGE

	double timeStart = 0, timeEnd = 0;
	
	int f = 1, all = 0;
	while (1)
	{
		cam.grab();
		cam.retrieve(imRaw);

		if (imRaw.empty())
		{
			cout << "imRawΪ�գ�����" << endl;
			return 1;
		}
		
		all++;
		char key = waitKey(1);
		string imgname;
		if (key == 27)break;

		if (all % 30 == 0)
		{
			stringstream stream;
			stream << f++;
			imgname = stream.str() + ".jpg";
			imwrite(imgname, imRaw);
			cout << "shoot!!!" << endl;
		}
		
#ifdef STDIO_DEBUG
		//�����㷨֡��
		cout << "fps: " << 1.0 / (timeEnd - timeStart)*(double)getTickFrequency() << endl;
		timeStart = timeEnd;
		timeEnd = (double)getTickCount();
#endif // STDIO_DEBUG
		
		///С��λ�㷨����
		
		
		
#ifdef SOCKET_SEND_IMAGE
		//����ͼ�����ڲ���
		resize(imRaw, imSend, Size(0, 0), 0.5, 0.5, INTER_NEAREST);
//		threshold(imTrans, imSend, threshBinary, 255, CV_THRESH_BINARY);
		socketMat.transmit(imSend, 80);
#endif // SOCKET_SEND_IMAGE
		
		
	}
	
#ifdef STDIO_DEBUG
	cout << "���ս���" << endl;
#endif // STDIO_DEBUG
	
#ifdef SOCKET_SEND_IMAGE
	socketMat.disconnect();
#endif // SOCKET_SEND_IMAGE
	cam.release();
	return 0;
}




