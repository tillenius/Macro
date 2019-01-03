#include "settingsdlg.h"
#include "resource.h"
#include "main.h"

namespace {

INT_PTR CALLBACK counterSettings(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static char buf[15];

    switch (message) {
        case WM_INITDIALOG: {
            wsprintf(buf, "%d", g_app->m_settings.m_counterDigits);
            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_COUNTERDIGITS), buf);

            if (g_app->m_settings.m_counterHex)
                wsprintf(buf, "%x", g_app->m_settings.m_counter);
            else
                wsprintf(buf, "%d", g_app->m_settings.m_counter);

            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_COUNTERDIGITS), buf, 14);
                    g_app->m_settings.m_counterDigits = atol(buf);

                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf, 14);
                    g_app->m_settings.m_counter = strtoul(buf, NULL, (g_app->m_settings.m_counterHex ? 16 : 10));

                    EndDialog(hWnd, 0);
                    break;

                case IDC_CHECK_HEXCOUNTER: {
                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf, 14);
                    int value = strtoul(buf, NULL, (g_app->m_settings.m_counterHex ? 16 : 10));

                    g_app->m_settings.m_counterHex = (IsDlgButtonChecked(hWnd, IDC_CHECK_HEXCOUNTER) == BST_CHECKED);

                    if (g_app->m_settings.m_counterHex)
                        wsprintf(buf, "%x", value);
                    else
                        wsprintf(buf, "%d", value);

                    SetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf);
                    break;
                }

                default:
                    return FALSE;
            }
            break;
        case WM_CLOSE:
            EndDialog(hWnd, 0);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

} // namespace

void SettingsDlg::show(HINSTANCE hInstance, HWND hWnd) {
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_COUNTER), hWnd, counterSettings);
}
