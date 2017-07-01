#include <windows.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE (100 * 1024)
#define COM_BUFF_SIZE 1024
#define IDT_TIMER1 1

char acTextBuff[BUFF_SIZE];
long nCharNum = 0, nCurPos = 0;
long nXChar = 0, nYChar = 0;
long nDisRow = 0, nDisLine = 0, nDisStar = 0;

HANDLE hCom;
char acComRxBuff[COM_BUFF_SIZE];
char acComTxBuff[COM_BUFF_SIZE];
BOOL bComIsOpen = FALSE;
char acComPort[8] = {0};
unsigned nComBaud = 0;
const unsigned anComBaudEx[] = {9600, 115200, 76800};

BOOL InitCOM(VOID);
DWORD ReadCOM(char* lpInBuffer, DWORD dwLen);
DWORD WriteCOM(char* lpOutBuffer, DWORD dwLen);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	HFONT hF;
	RECT rc;
	TEXTMETRIC tm;
	PAINTSTRUCT PtStr;
	long i, j, k, n, m;

	switch (iMessage)
	{
	case WM_TIMER:
		if(wParam == IDT_TIMER1)
		{
			i = ReadCOM(acComRxBuff, COM_BUFF_SIZE);
			if (i)
			{
				
				
				for (j = 0; j < i; j++)
				{
					acTextBuff[nCharNum++] = acComRxBuff[j];
					nCurPos++;
					if (nCharNum >= BUFF_SIZE)
					{
						for (k = nCharNum / 3; k < nCharNum; k++)
						{
							if (('\r' == acTextBuff[k]) || ('\n' == acTextBuff[k]))
							{
								k++;
								while (('\r' == acTextBuff[k])
									|| ('\n' == acTextBuff[k]))
								{
									k++;
								}
								break;
							}
						}
						for (n = 0, m = k; k < nCharNum; k++)
						{
							acTextBuff[n++] = acTextBuff[k];
						}
						nCharNum -= m;
						nCurPos -= m;
						nDisStar -= m;
						if (nCurPos < 0) {nCurPos = 0;}
						if (nDisStar < 0) {nDisStar = 0;}
					}
				}
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		break;
	case WM_CHAR:
		if (bComIsOpen) {
			acComTxBuff[0] = (char)wParam;
			WriteCOM(acComTxBuff, 1);
		}
		else {
			acTextBuff[nCharNum++] = (char)wParam;
			nCurPos++;
			if (('\r' == (char)wParam) || ('\n' == (char)wParam))
			{
				acComPort[0] = 0;
				nComBaud = 0;
				for (i = 0; i < nCharNum; i++) {if (' ' != acTextBuff[i]) {break;}}
				for (j = 0; i < nCharNum; i++)
				{
					if (' ' == acTextBuff[i])
					{
						acComPort[j] = 0;
						break;
					}
					else if (j < 7)
					{						
						acComPort[j++] = acTextBuff[i];
					}
				}
				for (; i < nCharNum; i++) {if (' ' != acTextBuff[i]) {break;}}
				for (; i < nCharNum; i++)
				{
					if (('0' <= acTextBuff[i]) && ('9' >= acTextBuff[i]))
					{
						nComBaud *= 10;
						nComBaud += acTextBuff[i] - 48;
					}
					else
					{
						break;
					}
				}
				for (i = 0; i < sizeof(anComBaudEx); i++)
				{
					if (anComBaudEx[i] == nComBaud) {break;}
				}
				if ((sizeof(anComBaudEx) == i) || (j < 4) || (j > 5))
				{					
					MessageBox(NULL, "Parameter Error!", NULL, MB_OK);
					nCharNum = 0;
					nCurPos = 0;
					nDisStar = 0;
				}											
				else {
					if (!InitCOM()) {
						nCharNum = 0;
						nCurPos = 0;
						nDisStar = 0;
					}
					else {
						bComIsOpen = TRUE;
					}
				}
			}
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	case WM_KEYDOWN:
		switch(wParam)
		{
			case VK_UP:
				while (('\r' != acTextBuff[nCurPos]) && ('\n' != acTextBuff[nCurPos]))
				{
					if (nCurPos > 0) {nCurPos--;}
					else {break;}
				}
				while (('\r' == acTextBuff[nCurPos]) || ('\n' == acTextBuff[nCurPos]))
				{
					if (nCurPos > 0) {nCurPos--;}
					else {break;}
				}
				if (nCurPos <= nDisStar)
				{
					nDisStar = 0;
				}
				InvalidateRect(hWnd, NULL, TRUE);
				break;
			case VK_DOWN:
				while (('\r' != acTextBuff[nCurPos]) && ('\n' != acTextBuff[nCurPos]))
				{
					if (nCurPos < nCharNum) {nCurPos++;}
					else {break;}
				}
				while (('\r' == acTextBuff[nCurPos]) || ('\n' == acTextBuff[nCurPos]))
				{
					if (nCurPos < nCharNum) {nCurPos++;}
					else {break;}
				}
				while (('\r' != acTextBuff[nCurPos]) && ('\n' != acTextBuff[nCurPos]))
				{
					if (nCurPos < nCharNum) {nCurPos++;}
					else {break;}
				}
				InvalidateRect(hWnd, NULL, TRUE);
				break;
			case VK_PRIOR:
				nCurPos = nDisStar;
				nDisStar = 0;
				InvalidateRect(hWnd, NULL, TRUE);
				break;
			case VK_LEFT:
				if (nCurPos > 0)
				{
					nCurPos -= 1;
					if (nCurPos <= nDisStar)
					{
						nDisStar = 0;
					}
					InvalidateRect(hWnd, NULL, TRUE);
				}
				break;
			case VK_RIGHT:
				if (nCurPos < nCharNum)
				{
					nCurPos += 1;
					InvalidateRect(hWnd, NULL, TRUE);
				}
				break;
			case VK_HOME:
				nDisStar = 0;
				nCurPos = 0;
				InvalidateRect(hWnd, NULL, TRUE);
				break;
			case VK_END:
				nCurPos = nCharNum;
				InvalidateRect(hWnd, NULL, TRUE);
				break;
			case VK_DELETE:
				break;
		}
		break;
	case WM_PAINT: //重画窗口消息
		hDC = BeginPaint(hWnd, &PtStr); //开始描画
		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, RGB(150, 200, 200));
		hF = CreateFont(16, 0, 0, 0,
			600, FALSE, FALSE,FALSE,
			GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
			"nf_1");
		SelectObject(hDC,hF);
		GetTextMetrics(hDC, &tm);
		GetClientRect(hWnd, &rc);
		//nXChar = ((tm.tmPitchAndFamily & 1 ? 3 : 2) * tm.tmAveCharWidth / 2);
		nYChar = tm.tmHeight + tm.tmExternalLeading;
		//nDisRow = (rc.right - rc.left) / nXChar;
		nDisLine = (rc.bottom - rc.top);
		//nDisStar = 0;
		DrawText(hDC, &(acTextBuff[nDisStar]), nCurPos - nDisStar, &rc, DT_LEFT | DT_EXTERNALLEADING | DT_CALCRECT);
		if ((rc.bottom - rc.top) > nDisLine)
		{
			nDisLine = (rc.bottom - rc.top - nDisLine) / nYChar + 1;
			for (j = 0; j < nDisLine; j++)
			{
				for (i = nDisStar; i < nCurPos; i++)
				{
					if ('\r' == acTextBuff[nDisStar++])
					{
						if ('\n' == acTextBuff[nDisStar])
						{
							nDisStar++;
						}
						break;
					}
					else if ('\n' == acTextBuff[nDisStar])
					{
						if ('\r' == acTextBuff[nDisStar])
						{
							nDisStar++;
						}
						break;
					}
				}
			}
		}
		DrawText(hDC, &(acTextBuff[nDisStar]), nCurPos - nDisStar, &rc, DT_LEFT | DT_EXTERNALLEADING);
		EndPaint(hWnd, &PtStr); //结束描画
		break;
	case WM_RBUTTONDOWN://右键
		break;
	case WM_LBUTTONDOWN://左键	
		if (wParam & MK_CONTROL)//ctr
		{
			HANDLE hGlobalMemory = GlobalAlloc(GHND, nCharNum + 1); // 分配内存
			LPBYTE lpGlobalMemory = (LPBYTE)GlobalLock(hGlobalMemory); // 锁定内存
			for(i = 0; i < nCharNum; i++) // 将"*"复制到全局内存块
				{*lpGlobalMemory++ = acTextBuff[i];}
			*lpGlobalMemory = 0;
			GlobalUnlock(hGlobalMemory); // 锁定内存块解锁
			OpenClipboard(hWnd); // 打开剪贴板
			EmptyClipboard(); // 清空剪贴板
			SetClipboardData(CF_TEXT, hGlobalMemory); // 将内存中的数据放置到剪贴板
			CloseClipboard(); // 关闭剪贴板
		}
		// if (g_unStrIndex < STR_LEN_MAX)
		// {
			// g_szStrBuff[g_unStrIndex++] = g_unStrIndex % 10 + 48;
			// g_szStrBuff[g_unStrIndex] = '\0';
		// }
		break;
	case WM_CREATE:
		SetTimer(hWnd, IDT_TIMER1, 100, NULL);
		// hDC = GetDC(hWnd);
		// GetTextMetrics(hDC, &tm);
		// nXChar = tm.tmAveCharWidth;
		// nYChar = tm.tmHeight + tm.tmExternalLeading;
		// ReleaseDC(hWnd, hDC);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY: //关闭窗口消息
		if (!CloseHandle(hCom)) {
			//MessageBox(NULL, "Com Close Fail!", NULL, MB_OK);
		}
		PostQuitMessage(0); //发送关闭消息
		break;
	default:
		return DefWindowProc(hWnd, iMessage, wParam, lParam); //缺省窗口消息处理函数
	}

	return 0;
}

DWORD ReadCOM(char* lpInBuffer, DWORD dwLen)
{
	DWORD dwBytesRead = dwLen;
	BOOL bReadStatus;
	DWORD dwErrorFlags;
	COMSTAT ComStat;
	OVERLAPPED m_osRead;

	ClearCommError(hCom, &dwErrorFlags, &ComStat);
	if (!ComStat.cbInQue) {return 0;}

	memset(&m_osRead, 0, sizeof(OVERLAPPED));
	m_osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	dwBytesRead = min(dwBytesRead, (DWORD)ComStat.cbInQue);
	bReadStatus = ReadFile(hCom, lpInBuffer, dwBytesRead, &dwBytesRead, &m_osRead);
	if(!bReadStatus) //如果ReadFile函数返回FALSE
	{
		if(GetLastError() == ERROR_IO_PENDING)
		{
			WaitForSingleObject(m_osRead.hEvent, 1000);
			PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
				//使用WaitForSingleObject函数等待，直到读操作完成或延时已达到2秒钟
				//当串口读操作进行完毕后，m_osRead的hEvent事件会变为有信号

			//GetOverlappedResult(hCom, &m_osRead, &dwBytesRead, TRUE);
				// GetOverlappedResult函数的最后一个参数设为TRUE，
				//函数会一直等待，直到读操作完成或由于错误而返回。

			return dwBytesRead;
		}
		return 0;
	}

	return dwBytesRead;
}

DWORD WriteCOM(char* lpOutBuffer, DWORD dwLen)
{
	DWORD dwBytesWritten = dwLen;
	DWORD dwErrorFlags;
	COMSTAT ComStat;
	OVERLAPPED m_osWrite;
	BOOL bWriteStat;

	bWriteStat = WriteFile(hCom, lpOutBuffer, dwBytesWritten, &dwBytesWritten, &m_osWrite);
	if (!bWriteStat)
	{
		if(GetLastError() == ERROR_IO_PENDING)
		{
			WaitForSingleObject(m_osWrite.hEvent, 1000);
			return dwBytesWritten;
		}
		return 0;
	}

	return dwBytesWritten;
}

BOOL InitCOM(VOID)
{
	DCB dcb;
	COMMTIMEOUTS TimeOuts;

	hCom = CreateFile(acComPort, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //重叠方式
		NULL);
	if(hCom == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, "Com Open Fail!", NULL, MB_OK);
		return FALSE;
	}

	SetupComm(hCom, COM_BUFF_SIZE, COM_BUFF_SIZE); //输入缓冲区和输出缓冲区的大小都是1024

	GetCommTimeouts(hCom, &TimeOuts);
	//设定读超时
	TimeOuts.ReadIntervalTimeout = 1000;
	TimeOuts.ReadTotalTimeoutMultiplier = 500;
	TimeOuts.ReadTotalTimeoutConstant = 5000;
	//设定写超时
	TimeOuts.WriteTotalTimeoutMultiplier = 500;
	TimeOuts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(hCom, &TimeOuts); //设置超时

	GetCommState(hCom, &dcb);
	dcb.BaudRate = nComBaud; //波特率
	dcb.ByteSize = 8; //每个字节有8位
	dcb.Parity = NOPARITY; //无奇偶校验位
	dcb.StopBits = TWOSTOPBITS; //停止位
	SetCommState(hCom, &dcb);

	PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);
}

BOOL InitWindows(HINSTANCE hInstance, LPCSTR szClassName, int nCmdShow)
{
	HWND hWnd;

	hWnd = CreateWindow(szClassName,
		"Com Console ----Luyuexin",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		600, 480,
		NULL, NULL,
		hInstance, NULL);

	if (hWnd == NULL) {return FALSE;}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

BOOL InitWindowsClass(HINSTANCE hInstance, LPCSTR szClassName)
{
	WNDCLASS WndClass;

	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = (LPCSTR)szClassName;

	return (RegisterClass(&WndClass));
}

int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst,
	LPSTR lpsCmdLine, int nCmdShow)
{
	MSG Message;
	char szClassName[] ="test_wc"; //窗口名

	//注册窗口类
	if (!InitWindowsClass(hCurInst, szClassName)) {
		return FALSE;
	}

	//初始化窗
	if (!InitWindows(hCurInst, szClassName, nCmdShow)) {
		return FALSE;
	}

	// if (!InitCOM()) {
		// return FALSE;
	// }

	//消息循环
	while (GetMessage(&Message, NULL, 0, 0) > 0) {
		TranslateMessage(&Message); //消息解释
		DispatchMessage(&Message); //消息发送
	}

	// if (!CloseHandle(hCom)) {
		// return FALSE;
	// }

	return (int)Message.wParam;
}