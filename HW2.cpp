

#include "stdafx.h"
#include <stdint.h>
#include <conio.h>
#include "FileIoHelperClass.h"
#include "StopWatch.h"
#include "mmio.h"
#include <time.h>

UCHAR *BUF = (UCHAR *)malloc(1024*1024*1024*10);
int _tmain(int argc, _TCHAR* argv[])
{
	
	printf("<< Small File Create, Copy! (1GB) >>\n\n");
	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 1024*1024*1024;
	_ASSERTE(create_very_big_file(L"small.txt", 512));

	StopWatch sw1;
	sw1.Start();
	_ASSERTE(file_copy_using_read_write(L"small.txt", L"small1.txt"));
	sw1.Stop();
	print("[*] read & write time elapsed = %f\n", sw1.GetDurationSecond());

	StopWatch sw2;
	sw2.Start();
	_ASSERTE(file_copy_using_memory_map(L"small.txt", L"small2.txt"));
	sw2.Stop();
	print("[*] mmio time elapsed = %f\n", sw2.GetDurationSecond());

	


	printf("\nBig File Create & Copy Start!\n\n");
	_sleep(2);
	
	printf("<< Big File Create, Copy! (10GB) >>\n");
	
	StopWatch sw3;
	sw3.Start();
	_ASSERTE(file_copy_using_read_write(L"big.txt", L"big1.txt"));
	sw3.Stop();
	print("[*] read & write time elapsed = %f\n", sw3.GetDurationSecond());
	/*
	LARGE_INTEGER offset;
	offset.QuadPart=512;
	_ASSERTE(create_very_big_file(L"big.txt", 10*1024));
	FileIoHelper *a = new FileIoHelper;
	LARGE_INTEGER msize;
	msize.QuadPart = 1024*1024*1024*10;
	StopWatch sw4;
	sw4.Start();
	_ASSERTE(file_copy_using_memory_map(L"big.txt", L"big1.txt"));
	a->FIOpenForRead(L"big.txt");
	a->FIOCreateFile(L"big2.txt", msize);
	sw4.Stop();
	*/
	return 0;
}