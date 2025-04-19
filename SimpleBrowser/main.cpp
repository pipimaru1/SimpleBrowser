// BrowserAppWebView2.cpp : Win32 + WebView2 �T���v��
#include <windows.h>
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"          // WebView2 SDK �̃w�b�_�[
#include <WebView2EnvironmentOptions.h>  // �� �����ǉ�

#pragma comment(lib, "ole32.lib")

//////////////////////////////////////////
// ����URL
#define STARTURL L"https://chatgpt.com/"

// �v���L�V�ݒ� ���Ɏw�肵�Ȃ��Ă��V�X�e���ݒ�����ɍs��
//#define PROXURL L"--proxy-server=http://hoge:8080"
#define PROXURL L""

using namespace Microsoft::WRL;
using ccHDL = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
using ceHDL = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;

// ���j���[ ID
#define ID_FILE_OPEN    1001
#define ID_FILE_BACK    1002
#define ID_FILE_FORWARD 1003

// �O���[�o��
//HWND g_hWnd = nullptr;
// �O���[�o���ϐ����Q�ɕ����Ď���
static ComPtr<ICoreWebView2Controller> g_webviewController = nullptr;
static ComPtr<ICoreWebView2>           g_webview = nullptr;

// Forward decl.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

///////////////////////////////////////////////////////////////
// WebView2 ���̏�����
// �R�[���o�b�N�� WebView2 �R���g���[���[�𐶐�
// �R���g���[���[�� Bounds ��ݒ肵�A���� URL �Ƀi�r�Q�[�g
// ����: hWnd - ���C���E�B���h�E�̃n���h��
// �߂�l: �Ȃ�
// ����: WebView2 �̏������͔񓯊��ōs���邽�߁A
//       �R�[���o�b�N�ŃR���g���[���[�𐶐�����K�v������
//       �R�[���o�b�N�̒��� WebView2 �R���g���[���[��ێ�����
//       �ϐ����O���[�o���Ɏ��K�v������
//       �R���g���[���[�� Bounds ��ݒ肷�邽�߁A
//       ���C���E�B���h�E�̃n���h���������ɓn��
///////////////////////////////////////////////////////////////
void InitializeWebView2(HWND hWnd)
{
    // �@ ���I�v�V�������쐬���A�v���L�V������ݒ�
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	options->put_AdditionalBrowserArguments(PROXURL); // �v���L�V�ݒ�

	// �A WebView2 ����񓯊��ŏ�����
    CreateCoreWebView2EnvironmentWithOptions
    (
        nullptr, 
        nullptr, 
        options.Get(),  // �� ������ options.Get() ��n���I
        Callback<ceHDL>
        (
			// �����_���ŃR�[���o�b�N������
            [hWnd](HRESULT envRes, ICoreWebView2Environment* env) -> HRESULT 
            {
                if (FAILED(envRes)) 
                {
                    MessageBox(hWnd, L"WebView2 �����������s", L"Error", MB_OK);
                    return envRes;
                }
                return env->CreateCoreWebView2Controller
                (
                    hWnd,
                    Callback<ccHDL>
                    (
                        [hWnd](HRESULT ctrlRes, ICoreWebView2Controller* controller) -> HRESULT 
                        {
                            if (FAILED(ctrlRes) || !controller) 
                            {
                                MessageBox(hWnd, L"WebView2 �R���g���[���[�������s", L"Error", MB_OK);
                                return ctrlRes;
                            }
                            // �R���g���[���[�� WebView ��ێ�
                            g_webviewController = controller;
                            controller->get_CoreWebView2(&g_webview);

                            // **�@ �����o�E���h��ݒ�**  
                            RECT bounds;
                            GetClientRect(hWnd, &bounds);
                            g_webviewController->put_Bounds(bounds);

                            // ���� URL ���i�r�Q�[�g
                            g_webview->Navigate(STARTURL);

                            return S_OK;
                        }
                    ).Get()
                );
            }
        ).Get()
    );
}

/////////////////////////////////////////////////////////////////
// ���C���֐�
/////////////////////////////////////////////////////////////////
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShowCmd) {
    // STA ���[�h�� OLE ������
    if (FAILED(OleInitialize(NULL))) {
        MessageBox(NULL, L"OleInitialize �Ɏ��s", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // �E�B���h�E�N���X�o�^
    const wchar_t CLASS_NAME[] = L"WebView2BrowserClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // ���j���[�쐬
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_OPEN, L"�J��");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_BACK, L"�߂�");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_FORWARD, L"�i��");
    HMENU hMenu = CreateMenu();
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"�t�@�C��");

    // ���C���E�B���h�E����
    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Simple Browser",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        1024, 768, nullptr, hMenu, hInst, nullptr
    );
    if (!hWnd) {
        MessageBox(NULL, L"���C���E�B���h�E�������s", L"Error", MB_OK | MB_ICONERROR);
        OleUninitialize();
        return 1;
    }

    ShowWindow(hWnd, nShowCmd);
    UpdateWindow(hWnd);

    // WebView2 �̔񓯊�������
    InitializeWebView2(hWnd);

    // ���b�Z�[�W���[�v
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    OleUninitialize();
    return 0;
}

/////////////////////////////////////////////////////////////////
// �E�B���h�E�v���V�[�W��
/////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) 
    {
        //�E�B���h�E�T�C�Y�ύX����
        case WM_SIZE:
        if (g_webviewController) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            // �R���g���[���[�Ƀo�E���h�𒼐ڐݒ�
            g_webviewController->put_Bounds(rc);
        }
        break;

        //���j���[����
        case WM_COMMAND:
        switch (LOWORD(wp)) 
        {
            case ID_FILE_OPEN: 
            {
                // ���[�J�� HTML �t�@�C���I��
                OPENFILENAMEW ofn{ sizeof(ofn) };
                wchar_t szFile[MAX_PATH]{};
                ofn.hwndOwner = hWnd;
                ofn.lpstrFilter = L"HTML Files\0*.htm;*.html\0All Files\0*.*\0";
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) 
                {
                    // file:/// �ɕϊ����� Navigate
                    std::wstring uri = L"file:///" + std::wstring(szFile);
                    g_webview->Navigate(uri.c_str());
                }
                break;
            }

            case ID_FILE_BACK: 
            {
                BOOL canGoBack = FALSE;
                if (g_webview && SUCCEEDED(g_webview->get_CanGoBack(&canGoBack)) && canGoBack) 
                {
                    g_webview->GoBack();
                }
                break;
            }
    
            case ID_FILE_FORWARD: 
            {
                BOOL canGoForward = FALSE;
                if (g_webview && SUCCEEDED(g_webview->get_CanGoForward(&canGoForward)) && canGoForward) 
                {
                    g_webview->GoForward();
                }
                break;
            }
        }
        break;

        //�E�B���h�E������ꂽ�Ƃ�
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, msg, wp, lp);
    }
    return 0;
}
