

#include <Windows.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#include <malloc.h>

#include <atomic>

typedef unsigned long long log_type;

log_type *threadcounts;
unsigned int threadcount;
HANDLE *handles;

const unsigned int CHARS = 10;

typedef unsigned __int32 uint32_t;

uint32_t wanted = 0x980FAE1E;

extern uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32);

__declspec(thread) char buffer[CHARS + 1];

void dothecrc(log_type &logger, char universe, unsigned int done, uint32_t crc)
{
	if (done == CHARS) return;
	buffer[done + 1] = 0;

	uint32_t temp;

	for (char c = '0'; c <= '9'; c++)
	{
		buffer[done] = c;
		temp = crc32_1byte(&c, 1, crc);

		if (temp == wanted)
			printf("Matched crc with %08X! (STEAM_0:%c:%s)\n", ~wanted, universe, buffer);

		dothecrc(logger, universe, done + 1, temp);
	}

	logger += 10;

	buffer[done] = 0;
}
static std::atomic<unsigned char> nextcharid;

uint32_t start_crc;

DWORD WINAPI Finder(LPVOID threadid)
{
	for (int i = 1; i <= CHARS; i++)
		buffer[i] = 0;

	unsigned char temp = nextcharid.fetch_add(1);

	while (temp < 20 && temp >= 0)
	{
		char universe = temp > 9 ? '1' : '0';
		char start = (temp % 10) + '0';


		uint32_t current_crc = crc32_1byte(&universe, 1, start_crc);
		current_crc = crc32_1byte(":", 1, current_crc);
		current_crc = crc32_1byte(&start, 1, current_crc);

		buffer[0] = start;

		if(current_crc == wanted)
			printf("Matched crc with %08X! (STEAM_0:%c:%s)\n", ~wanted, universe, buffer);

		threadcounts[(unsigned long long)threadid]++;

		dothecrc(threadcounts[(unsigned long long)threadid], universe, 1, current_crc);

		temp = nextcharid.fetch_add(1);
	}

	return 0;
}

extern void InitCRC32(void);

log_type gettargetamount(void)
{
	static log_type ret = -1;
	if (ret != -1) return ret;

	ret = 0;

	for (int i = 1; i <= CHARS; i++)
		ret += (log_type)pow(10, i);

	ret *= 2;

	return ret;
}

int __cdecl main(char *argv[], int argc)
{
	nextcharid = 0;

	start_crc = crc32_1byte("gm_STEAM_0:", 11, 0xFFFFFFFF);

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	threadcount = sysinfo.dwNumberOfProcessors;

	printf("Using automatic thread number: %i%s\n", threadcount, threadcount >= 8 ? " (nice cpu! :o)" : "");

	threadcounts = new log_type[threadcount];
	handles = new HANDLE[threadcount];

	clock_t start;
	for (unsigned int i = 0; i < threadcount; i++)
	{
		threadcounts[i] = 0;
		if (i == 0) start = clock();
		handles[i] = CreateThread(NULL, NULL, &Finder, (LPVOID)i, NULL, NULL);
	}

	
	while (WAIT_TIMEOUT == WaitForMultipleObjects(threadcount, handles, TRUE, 500))
	{
		log_type total = 0;
		for (unsigned int i = 0; i < threadcount; i++)
			total += threadcounts[i];

		static char temp[512];

		sprintf_s(temp, "%" PRIu64 " complete (%.2f%%), time: %.2f seconds, avg: %.2f/second", total, double(total) / double(gettargetamount()) * 100, double(clock() - start) / double(CLOCKS_PER_SEC), total / (double(clock() - start) / double(CLOCKS_PER_SEC)));
		SetConsoleTitleA(temp);
	}

	log_type total = 0;
	for (unsigned int i = 0; i < threadcount; i++)
		total += threadcounts[i];

	static char temp[512];

	sprintf_s(temp, "%" PRIu64 " complete (%.2f%%), time: %.2f seconds, avg: %.2f/second", total, double(total) / double(gettargetamount()) * 100, double(clock() - start) / double(CLOCKS_PER_SEC), total / (double(clock() - start) / double(CLOCKS_PER_SEC)));
	SetConsoleTitleA(temp);


	printf("I'm done!\n");
	while (1) Sleep(1000);

	return 0;
}