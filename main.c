#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include "res.h"

/*

From PuTTY doc

3.8.1 Starting a session from the command line
        To start a connection to a serial port, e.g. COM1:
        putty.exe -serial com1

3.8.3.22 -sercfg: specify serial port configuration
        Any single digit from 5 to 9 sets the number of data bits.
        ‘1’, ‘1.5’ or ‘2’ sets the number of stop bits.
        Any other numeric string is interpreted as a baud rate.
        A single lower-case letter specifies the parity: ‘n’ for none, ‘o’ for odd, ‘e’ for even, ‘m’ for mark and ‘s’ for space.
        A single upper-case letter specifies the flow control: ‘N’ for none, ‘X’ for XON/XOFF, ‘R’ for RTS/CTS and ‘D’ for DSR/DTR.

        For example, ‘-sercfg 19200,8,n,1,N’ denotes a baud rate of 19200, 8 data bits, no parity, 1 stop bit and no flow control.

*/

#define GET_ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#if defined(UNICODE)
#define SNPRINTF snwprintf
#define SSCANF   swscanf
#define STRCMP   wcscmp
#define STRLEN   wcslen
#else
#define SNPRINTF snprintf
#define SSCANF   sscanf
#define STRCMP   strcmp
#define STRLEN   strlen
#endif

const UINT baud_list[] = {
    CBR_2400,
    CBR_4800,
    CBR_9600,
    CBR_14400,
    CBR_19200,
    CBR_38400,
    CBR_57600,
    CBR_115200,
    230400,
    460800,
    921600
};

const UINT dbit_list[] = {
    5,
    6,
    7,
    8
};

LPCTSTR const sbit_list[] = {
    TEXT("1"),
    TEXT("1.5"),
    TEXT("2")
};

LPCTSTR const parity_list[] = {
    TEXT("None"),
    TEXT("Odd"),
    TEXT("Even"),
    TEXT("Mark"),
    TEXT("Space")
};

LPCTSTR const flow_list[] = {
    TEXT("None"),
    TEXT("XON/XOFF"),
    TEXT("RTS/CTS"),
    TEXT("DSR/DTR")
};

struct {
    HWND dlg;
    HWND port_cbx;
    HWND baud_cbx;
    HWND dbit_cbx;
    HWND sbit_cbx;
    HWND parity_cbx;
    HWND flow_cbx;
    HWND upd_btn;
    HWND try_chb;

    BOOL port_cbx_first_time;
} hwnd_dlg;

BOOL FileExists(LPCTSTR szPath) {
    DWORD dwAttrib = GetFileAttributes(szPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL SerialCheck(LPCTSTR port) {
    TCHAR portName[32];
    SNPRINTF(portName, GET_ARRAY_SIZE(portName), TEXT("\\\\.\\%s"), port);

    HANDLE hwd = CreateFile(portName,
                            GENERIC_READ | GENERIC_WRITE, 0, NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, 0);
    if (hwd != INVALID_HANDLE_VALUE) {
        CloseHandle(hwd);
        return TRUE;
    }

    return FALSE;
}

int SortCompareFn(const void* a, const void* b) {
    int a_val = 0;
    int b_val = 0;

    // extract number
    SSCANF(*(LPCTSTR*) a, TEXT("%*[^0123456789]%d"), &a_val);
    SSCANF(*(LPCTSTR*) b, TEXT("%*[^0123456789]%d"), &b_val);

    return a_val - b_val;
}

void EnumSerial(HWND hcbx, BOOL try_open) {
    const UINT listSize = 64;
    const UINT listItemSize = 16;

    TCHAR list[listSize][listItemSize];
    TCHAR* plist[listSize];
    UINT listIndex = 0;

    HKEY hKey;
    DWORD regIndex = 0;
    TCHAR regName[256];
    TCHAR regValue[listItemSize];
    DWORD regNameSize = GET_ARRAY_SIZE(regName);
    DWORD regValueSize = listItemSize;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return;
    }

    while (RegEnumValue(hKey, regIndex, regName, &regNameSize, NULL, NULL, (LPBYTE)regValue, &regValueSize) == ERROR_SUCCESS) {

        regValue[listItemSize - 1] = TEXT('\0');  // REG_SZ, REG_MULTI_SZ, REG_EXPAND_SZ may not be null terminated

        if (!try_open || SerialCheck(regValue)) {
            SNPRINTF(list[listIndex], listItemSize, TEXT("%s"), regValue);
            plist[listIndex] = list[listIndex];

            listIndex++;

            if (listIndex >= listSize) {
                break;
            }
        }

        regIndex++;
        regNameSize = GET_ARRAY_SIZE(regName);
        regValueSize = listItemSize;
    }

    RegCloseKey(hKey);

    ComboBox_ResetContent(hcbx);

    if (listIndex) {
        qsort(plist, listIndex, sizeof(plist[0]), SortCompareFn);

        for (UINT i = 0; i < listIndex; ++i) {
            ComboBox_AddString(hcbx, plist[i]);
        }

        ComboBox_SetCurSel(hcbx, 0);
    }
}

BOOL ExecPutty(int cmd) {
    TCHAR cmd_line[128] = TEXT("putty.exe");

    if (cmd == IDC_SPUTTY_BTN) {
        TCHAR port[16];
        TCHAR baud[16];
        TCHAR dbit[16];
        TCHAR sbit[16];
        TCHAR parity[16];
        TCHAR flow[16];

        if (ComboBox_GetText(hwnd_dlg.port_cbx, port, GET_ARRAY_SIZE(port)) <= 0)
            return FALSE;

        if (ComboBox_GetText(hwnd_dlg.baud_cbx, baud, GET_ARRAY_SIZE(baud)) <= 0)
            return FALSE;

        if (ComboBox_GetText(hwnd_dlg.dbit_cbx, dbit, GET_ARRAY_SIZE(dbit)) <= 0)
            return FALSE;

        if (ComboBox_GetText(hwnd_dlg.sbit_cbx, sbit, GET_ARRAY_SIZE(sbit)) <= 0)
            return FALSE;

        if (ComboBox_GetText(hwnd_dlg.parity_cbx, parity, GET_ARRAY_SIZE(parity)) <= 0)
            return FALSE;

        if (ComboBox_GetText(hwnd_dlg.flow_cbx, flow, GET_ARRAY_SIZE(flow)) <= 0)
            return FALSE;

        SNPRINTF(cmd_line, GET_ARRAY_SIZE(cmd_line),
                 TEXT("putty.exe -serial %s -sercfg %s,%s,%c,%s,%c"),
                 port,
                 baud,
                 dbit,
                 parity[0] + 32,  // char to lower
                 sbit,
                 flow[0]);
    }

    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        DWORD err = GetLastError();

        if (err) {
            LPTSTR msg = NULL;

            if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, NULL)) {
                msg = TEXT("Undefined error");
            }

            MessageBox(hwnd_dlg.dlg, msg, TEXT("CreateProcess putty.exe failed"), MB_OK | MB_ICONERROR);

            LocalFree(msg);
        }

        return FALSE;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return TRUE;
}

void InitDialogUI(void) {
    TCHAR str[16];
    UINT i;

    for (i = 0; i < GET_ARRAY_SIZE(baud_list); ++i) {
        SNPRINTF(str, GET_ARRAY_SIZE(str), TEXT("%u"), baud_list[i]);
        ComboBox_AddString(hwnd_dlg.baud_cbx, str);
    }

    for (i = 0; i < GET_ARRAY_SIZE(dbit_list); ++i) {
        SNPRINTF(str, GET_ARRAY_SIZE(str), TEXT("%u"), dbit_list[i]);
        ComboBox_AddString(hwnd_dlg.dbit_cbx, str);
    }

    for (i = 0; i < GET_ARRAY_SIZE(sbit_list); ++i) {
        ComboBox_AddString(hwnd_dlg.sbit_cbx, sbit_list[i]);
    }

    for (i = 0; i < GET_ARRAY_SIZE(parity_list); ++i) {
        ComboBox_AddString(hwnd_dlg.parity_cbx, parity_list[i]);
    }

    for (i = 0; i < GET_ARRAY_SIZE(flow_list); ++i) {
        ComboBox_AddString(hwnd_dlg.flow_cbx, flow_list[i]);
    }

    ComboBox_SetCurSel(hwnd_dlg.baud_cbx, 0);
    ComboBox_SetCurSel(hwnd_dlg.dbit_cbx, GET_ARRAY_SIZE(dbit_list) - 1);
    ComboBox_SetCurSel(hwnd_dlg.sbit_cbx, 0);
    ComboBox_SetCurSel(hwnd_dlg.parity_cbx, 0);
    ComboBox_SetCurSel(hwnd_dlg.flow_cbx, 0);

    // load PuTTY default settings
    HKEY hKey;
    DWORD value;
    DWORD valueType;
    DWORD valueSize;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     TEXT("SOFTWARE\\SimonTatham\\PuTTY\\Sessions\\Default%20Settings"),
                     0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return;
    }

    valueType = REG_SZ;
    valueSize = GET_ARRAY_SIZE(str) - 1;
    if (RegQueryValueEx(hKey, TEXT("SerialLine"), NULL, &valueType, (LPBYTE)str, &valueSize) == ERROR_SUCCESS) {
        str[GET_ARRAY_SIZE(str) - 1] = TEXT('\0');  // REG_SZ, REG_MULTI_SZ, REG_EXPAND_SZ may not be null terminated

        UINT cnt = ComboBox_GetCount(hwnd_dlg.port_cbx);
        for (i = 0; i < cnt; ++i) {
            TCHAR port[16];
            if (ComboBox_GetLBTextLen(hwnd_dlg.port_cbx, i) < (GET_ARRAY_SIZE(port) - 1)) {
                ComboBox_GetLBText(hwnd_dlg.port_cbx, i, port);
                if (STRCMP(str, port) == 0) {
                    ComboBox_SetText(hwnd_dlg.port_cbx, str);
                    break;
                }
            }
        }
    }

    valueType = REG_DWORD;
    valueSize = sizeof(value);
    if (RegQueryValueEx(hKey, TEXT("SerialSpeed"), NULL, &valueType, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS) {
        SNPRINTF(str, GET_ARRAY_SIZE(str), TEXT("%lu"), value);
        ComboBox_SetText(hwnd_dlg.baud_cbx, str);
    }

    valueSize = sizeof(value);
    if (RegQueryValueEx(hKey, TEXT("SerialDataBits"), NULL, &valueType, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS) {
        SNPRINTF(str, GET_ARRAY_SIZE(str), TEXT("%lu"), value);
        ComboBox_SetText(hwnd_dlg.dbit_cbx, str);
    }

    valueSize = sizeof(value);
    if (RegQueryValueEx(hKey, TEXT("SerialStopHalfbits"), NULL, &valueType, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS) {
        if (value == 3) {
            ComboBox_SetCurSel(hwnd_dlg.sbit_cbx, 1);
        } else if (value == 4) {
            ComboBox_SetCurSel(hwnd_dlg.sbit_cbx, 2);
        }
    }

    valueSize = sizeof(value);
    if (RegQueryValueEx(hKey, TEXT("SerialParity"), NULL, &valueType, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS) {
        if (value < GET_ARRAY_SIZE(parity_list)) {
            ComboBox_SetCurSel(hwnd_dlg.parity_cbx, value);
        }
    }

    valueSize = sizeof(value);
    if (RegQueryValueEx(hKey, TEXT("SerialFlowControl"), NULL, &valueType, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS) {
        if (value < GET_ARRAY_SIZE(flow_list)) {
            ComboBox_SetCurSel(hwnd_dlg.flow_cbx, value);
        }
    }

    RegCloseKey(hKey);
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
    hwnd_dlg.dlg = hwnd;
    hwnd_dlg.port_cbx = GetDlgItem(hwnd, IDC_PORT_CBX);
    hwnd_dlg.baud_cbx = GetDlgItem(hwnd, IDC_BAUD_CBX);
    hwnd_dlg.dbit_cbx = GetDlgItem(hwnd, IDC_DBIT_CBX);
    hwnd_dlg.sbit_cbx = GetDlgItem(hwnd, IDC_SBIT_CBX);
    hwnd_dlg.parity_cbx = GetDlgItem(hwnd, IDC_PARITY_CBX);
    hwnd_dlg.flow_cbx = GetDlgItem(hwnd, IDC_FLOW_CBX);
    hwnd_dlg.upd_btn = GetDlgItem(hwnd, IDC_UPD_BTN);
    hwnd_dlg.try_chb = GetDlgItem(hwnd, IDC_TRY_CHB);

    hwnd_dlg.port_cbx_first_time = TRUE;  // by default the combo box is created with text highlighting - this is a fix

    SendMessage(hwnd_dlg.upd_btn, BM_SETIMAGE,
                (WPARAM)IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_UPDICON), IMAGE_ICON, 16, 16, 0));

    Button_SetCheck(hwnd_dlg.try_chb, 1);

    EnumSerial(hwnd_dlg.port_cbx, Button_GetCheck(hwnd_dlg.try_chb));

    InitDialogUI();

    return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
    switch (id) {
        case IDC_UPD_BTN:
            EnumSerial(hwnd_dlg.port_cbx, Button_GetCheck(hwnd_dlg.try_chb));
            ComboBox_ShowDropdown(hwnd_dlg.port_cbx, 1);
            break;
        case IDC_SPUTTY_BTN:
        case IDC_PUTTY_BTN:
            if (ExecPutty(id)) {
                EndDialog(hwnd, id);
            }
            break;
        case IDCANCEL:
            EndDialog(hwnd, id);
            break;
        case IDC_PORT_CBX:
            if (hwnd_dlg.port_cbx_first_time && codeNotify == CBN_SETFOCUS) {
                SendMessage(hwnd_dlg.port_cbx, CB_SETEDITSEL, 0, MAKELPARAM(-1, -1));
                hwnd_dlg.port_cbx_first_time = FALSE;
            }
            break;
        default:
            break;
    }
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        INT nCmdShow) {
    if (!FileExists(TEXT("putty.exe"))) {
        MessageBox(NULL, TEXT("PuTTY not found!"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    InitCommonControls();
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
    return EXIT_SUCCESS;
}
