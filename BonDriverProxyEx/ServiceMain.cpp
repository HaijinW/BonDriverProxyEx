#include <locale.h>
#include "CWinService.cpp"

static void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	CWinService *lpCWinService = CWinService::getInstance();
	if (lpCWinService->RegisterService() != TRUE)
		return;

	BOOL bWinsockInit = FALSE;
	do
	{
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
			break;
		bWinsockInit = TRUE;

		if (Init(NULL) != 0)
			break;

		HostInfo *phi = new HostInfo;
		phi->host = g_Host;
		phi->port = g_Port;
		g_hListenThread = CreateThread(NULL, 0, Listen, phi, 0, NULL);
		if (g_hListenThread)
		{
			lpCWinService->ServiceRunning();
			ShutdownInstances();	// g_hListenThread�͂��̒���CloseHandle()�����
		}
		else
			delete phi;
		CleanUp();	// ShutdownInstances()��DriversMap�ɃA�N�Z�X����X���b�h�͖����Ȃ��Ă���͂�
	} while (0);

	if (bWinsockInit)
		WSACleanup();

	lpCWinService->ServiceStopped();

	return;
}

static int RunOnCmd(HINSTANCE hInstance)
{
	if (Init(hInstance) != 0)
		return -1;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -2;

	HostInfo *phi = new HostInfo;
	phi->host = g_Host;
	phi->port = g_Port;
	int ret = (int)Listen(phi);

	ShutdownInstances();
	CleanUp();

	WSACleanup();
	return ret;
}

static BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		g_ShutdownEvent.Set();
		return TRUE;
	default:
		return FALSE;
	}
}

int _tmain(int argc, _TCHAR *argv[], _TCHAR *envp[])
{
#if _DEBUG
	_CrtMemState ostate, nstate, dstate;
	_CrtMemCheckpoint(&ostate);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	HANDLE hLogFile = NULL;
#endif

	int ret = 0;
	CWinService *lpCWinService = CWinService::getInstance();

	_tsetlocale(LC_ALL, _T(""));
	do
	{
		if (argc == 1)
		{
			// �����Ȃ��ŋN�����ꂽ
#if _DEBUG
			TCHAR szDrive[4];
			TCHAR szPath[MAX_PATH];
			TCHAR szLogFile[MAX_PATH + 16];
			GetModuleFileName(NULL, szPath, MAX_PATH);
			_tsplitpath_s(szPath, szDrive, 4, szPath, MAX_PATH, NULL, 0, NULL, 0);
			_tmakepath_s(szLogFile, MAX_PATH + 16, szDrive, szPath, _T("dbglog"), _T(".txt"));
			hLogFile = CreateFile(szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			SetFilePointer(hLogFile, 0, NULL, FILE_END);
			_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
			_CrtSetReportFile(_CRT_WARN, hLogFile);
			_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
			_CrtSetReportFile(_CRT_ERROR, hLogFile);
			_RPT0(_CRT_WARN, "--- PROCESS_START ---\n");
#endif
			if (lpCWinService->Run(ServiceMain))
			{
				// �T�[�r�X������
				break;
			}
			else
			{
				// �T�[�r�X�ł͂Ȃ�
				_tprintf(_T("�R���\�[�����[�h�ŊJ�n���܂�...Ctrl+C�ŏI��\n"));
				SetConsoleCtrlHandler(HandlerRoutine, TRUE);
				ret = RunOnCmd(GetModuleHandle(NULL));
				switch (ret)
				{
				case -1:
					_tprintf(_T("ini�t�@�C���̓Ǎ��Ɏ��s���܂���\n"));
					break;
				case -2:
					_tprintf(_T("winsock�̏������Ɏ��s���܂���\n"));
					break;
				case 1:
					_tprintf(_T("Host�A�h���X�̉����Ɏ��s���܂���\n"));
					break;
				case 2:
					_tprintf(_T("bind()�Ɏ��s���܂���\n"));
					break;
				case 3:
					_tprintf(_T("listen()�Ɏ��s���܂���\n"));
					break;
				case 4:
					_tprintf(_T("accept()���ɃG���[���������܂���\n"));
					break;
				case 0:
					_tprintf(_T("�I�����܂�\n"));
					break;
				}
				break;
			}
		}
		else
		{
			// ��������
			BOOL done = FALSE;
			for (int i = 1; i < argc; i++)
			{
				if (_tcscmp(argv[i], _T("install")) == 0)
				{
					if (lpCWinService->Install())
						_tprintf(_T("Windows�T�[�r�X�Ƃ��ēo�^���܂���\n"));
					else
						_tprintf(_T("Windows�T�[�r�X�Ƃ��Ă̓o�^�Ɏ��s���܂���\n"));
					done = TRUE;
					break;
				}
				else if (_tcscmp(argv[i], _T("remove")) == 0)
				{
					if (lpCWinService->Remove())
						_tprintf(_T("Windows�T�[�r�X����폜���܂���\n"));
					else
						_tprintf(_T("Windows�T�[�r�X����̍폜�Ɏ��s���܂���\n"));
					done = TRUE;
					break;
				}
				else if (_tcscmp(argv[i], _T("start")) == 0)
				{
					if (lpCWinService->Start())
						_tprintf(_T("Windows�T�[�r�X���N�����܂���\n"));
					else
						_tprintf(_T("Windows�T�[�r�X�̋N���Ɏ��s���܂���\n"));
					done = TRUE;
					break;
				}
				else if (_tcscmp(argv[i], _T("stop")) == 0)
				{
					if (lpCWinService->Stop())
						_tprintf(_T("Windows�T�[�r�X���~���܂���\n"));
					else
						_tprintf(_T("Windows�T�[�r�X�̒�~�Ɏ��s���܂���\n"));
					done = TRUE;
					break;
				}
				else if (_tcscmp(argv[i], _T("restart")) == 0)
				{
					if (lpCWinService->Restart())
						_tprintf(_T("Windows�T�[�r�X���ċN�����܂���\n"));
					else
						_tprintf(_T("Windows�T�[�r�X�̍ċN���Ɏ��s���܂���\n"));
					done = TRUE;
					break;
				}
			}
			if (done)
				break;
		}
		// Usage�\��
		_tprintf(_T("Usage: %s <command>\n")
			_T("�R�}���h\n")
			_T("  install    Windows�T�[�r�X�Ƃ��ēo�^���܂�\n")
			_T("  remove     Windows�T�[�r�X����폜���܂�\n")
			_T("  start      Windows�T�[�r�X���N�����܂�\n")
			_T("  stop       Windows�T�[�r�X���~���܂�\n")
			_T("  restart    Windows�T�[�r�X���ċN�����܂�\n")
			_T("\n")
			_T("�����Ȃ��ŋN�����ꂽ�ꍇ�A�R���\�[�����[�h�œ��삵�܂�\n"),
			argv[0]);
	} while (0);

#if _DEBUG
	_CrtMemCheckpoint(&nstate);
	if (_CrtMemDifference(&dstate, &ostate, &nstate))
	{
		_CrtMemDumpStatistics(&dstate);
		_CrtMemDumpAllObjectsSince(&ostate);
	}
	if (hLogFile)
	{
		_RPT0(_CRT_WARN, "--- PROCESS_END ---\n");
		CloseHandle(hLogFile);
	}
#endif

	return ret;
}
