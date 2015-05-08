#ifndef __BONDRIVER_PROXYEX_H__
#define __BONDRIVER_PROXYEX_H__
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <process.h>
#include <list>
#include <queue>
#include <map>
#include "Common.h"
#include "IBonDriver3.h"

#define HAVE_UI
#ifdef BUILD_AS_SERVICE
#undef HAVE_UI
#endif

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define WAIT_TIME	10	// GetTsStream()�̌�ŁAdwRemain��0�������ꍇ�ɑ҂���(ms)

////////////////////////////////////////////////////////////////////////////////

static char g_Host[256];
static char g_Port[8];
static size_t g_PacketFifoSize;
static DWORD g_TsPacketBufSize;
static DWORD g_OpenTunerRetDelay;
static BOOL g_SandBoxedRelease;
static BOOL g_DisableUnloadBonDriver;
static DWORD g_ProcessPriority;		// �s�v���Ǝv�����Ǖێ����Ă���
static int g_ThreadPriorityTsReader;
static int g_ThreadPrioritySender;

#include "BdpPacket.h"

#define MAX_DRIVERS	64		// �h���C�o�̃O���[�v���ƃO���[�v���̐��̗���
static char **g_ppDriver[MAX_DRIVERS];
struct stDriver {
	char *strBonDriver;
	HMODULE hModule;
	BOOL bUsed;
	FILETIME ftLoad;
};
static std::map<char *, std::vector<stDriver> > DriversMap;

////////////////////////////////////////////////////////////////////////////////

struct stTsReaderArg {
	IBonDriver *pIBon;
	volatile BOOL StopTsRead;
	volatile BOOL ChannelChanged;
	DWORD pos;
	std::list<cProxyServerEx *> TsReceiversList;
	cCriticalSection TsLock;
	stTsReaderArg()
	{
		StopTsRead = FALSE;
		ChannelChanged = TRUE;
		pos = 0;
	}
};

class cProxyServerEx {
#ifdef HAVE_UI
public:
#endif
	SOCKET m_s;
	DWORD m_dwSpace;
	DWORD m_dwChannel;
	char *m_pDriversMapKey;
	int m_iDriverNo;
#ifdef HAVE_UI
private:
#endif
	int m_iDriverUseOrder;
	IBonDriver *m_pIBon;
	IBonDriver2 *m_pIBon2;
	IBonDriver3 *m_pIBon3;
	HMODULE m_hModule;
	cEvent m_Error;
	BOOL m_bTunerOpen;
	HANDLE m_hTsRead;
	BOOL m_bChannelLock;
	stTsReaderArg *m_pTsReaderArg;
	cPacketFifo m_fifoSend;
	cPacketFifo m_fifoRecv;

	DWORD Process();
	int ReceiverHelper(char *pDst, DWORD left);
	static DWORD WINAPI Receiver(LPVOID pv);
	void makePacket(enumCommand eCmd, BOOL b);
	void makePacket(enumCommand eCmd, DWORD dw);
	void makePacket(enumCommand eCmd, LPCTSTR str);
	void makePacket(enumCommand eCmd, BYTE *pSrc, DWORD dwSize, float fSignalLevel);
	static DWORD WINAPI Sender(LPVOID pv);
	static DWORD WINAPI TsReader(LPVOID pv);
	void StopTsReceive();

	BOOL SelectBonDriver(LPCSTR p);
	IBonDriver *CreateBonDriver();

	// IBonDriver
	const BOOL OpenTuner(void);
	void CloseTuner(void);
	void PurgeTsStream(void);
	void Release(void);

	// IBonDriver2
	LPCTSTR EnumTuningSpace(const DWORD dwSpace);
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);
	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);

	// IBonDriver3
	const DWORD GetTotalDeviceNum(void);
	const DWORD GetActiveDeviceNum(void);
	const BOOL SetLnbPower(const BOOL bEnable);

public:
	cProxyServerEx();
	~cProxyServerEx();
	void setSocket(SOCKET s){ m_s = s; }
	static DWORD WINAPI Reception(LPVOID pv);
};

static std::list<cProxyServerEx *> g_InstanceList;
static cCriticalSection g_Lock;
static cEvent g_ShutdownEvent(TRUE, FALSE);
#if defined(HAVE_UI) || defined(BUILD_AS_SERVICE)
static HANDLE g_hListenThread;
#endif

#endif	// __BONDRIVER_PROXYEX_H__
