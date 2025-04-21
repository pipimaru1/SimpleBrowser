// BrowserAppWebView2.cpp : Win32 + WebView2 �T���v��
#include <windows.h>
#include <wrl.h>
#include <wil/com.h>
#include <wrl/wrappers/corewrappers.h>

#include "WebView2.h"          // WebView2 SDK �̃w�b�_�[
#include <WebView2EnvironmentOptions.h>  // �� �����ǉ�

#pragma comment(lib, "ole32.lib")
//////////////////////////////////////////////////////////////////////////
// ���d�v
// WevView2 Nuget�p�b�P�[�W���C���g�[�����邱�ƁB
// Implementation Nuget�p�b�P�[�W���C���X�g�[�����邱�ƁB
// C++17�ȍ~�̃o�[�W�������g�p���邱�ƁB
// �v���W�F�N�g�̃G���g���|�C���g���uWindows �A�v���P�[�V�����v�ɕύX���邱�ƁB [�����J�[] �� [�V�X�e��]
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////
// ����URL
#define STARTURL L"https://chatgpt.com/"

// �v���L�V�ݒ� ���Ɏw�肵�Ȃ��Ă��V�X�e���ݒ�����ɍs��
//#define PROXURL L"--proxy-server=http://hoge:8080"
#define PROXURL L""

// �C���^�[�t�F�[�X�����킩��₷���Z�k���邽�߂̕ʖ���`�iusing�j
// ccHdr �́uController Completed Handler�v�AceHdr �́uEnvironment Completed Handler�v
using namespace Microsoft::WRL;
using ccHdr = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
using ceHdr = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;

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

//#define _USE_LAMDA
#ifdef _USE_LAMDA
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
        Callback<ceHdr>
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
                    Callback<ccHdr>
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
#else

///////////////////////////////////////////////////////////////
// COM�iComponent Object Model�j�́A�قȂ�\�t�g�E�F�A���m��A�g������d�g�݂ł��B
// Windows�ł́AWebView2��Office�ȂǑ����̋@�\��COM�Œ񋟂���Ă��܂��B
// COM�ł́u�C���^�[�t�F�[�X�v�Ƃ����_��ɏ]���ċ@�\���g���܂��B
// �֐��̑���ɁA����̃��\�b�h�����I�u�W�F�N�g�i�n���h���j��n���ď������܂��B
// C++�ł́ACOM�I�u�W�F�N�g�����삷��ꍇ�A�C���^�[�t�F�[�X�����������N���X���K�v�ł��B
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// EnvHdr::Invoke() �́uWebView2 ���i�G���W���j�������ł�����v�Ă΂��B
// CtrlHdr::Invoke() �́uWebView2 �̃R���g���[���[�i���ۂ̉�ʂ𐧌䂷�镔���j�������ł�����v�Ă΂��B
// �����Ƃ� �񓯊��Ŏ����I�ɌĂ΂��B�蓮�� Invoke() ���ĂԂ��Ƃ͂��Ȃ��B
// MakeAndInitialize<T>(&ptr, args...) �� COM �p�̃C���X�^���X�����{�������֗̕��֐��B
// COM �N���X�ł� RuntimeClassInitialize() ���������̓����ɂȂ�B
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// ���������������̃n���h��
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

class CtrlHdr;

///////////////////////////////////////////////////////////////
// WebView2 ���̏����������������Ƃ��ɌĂ΂��N���X
// CreateCoreWebView2EnvironmentWithOptions() �̃R�[���o�b�N��Ƃ��Ďg����
///////////////////////////////////////////////////////////////
class EnvHdr :
    public RuntimeClass<RuntimeClassFlags<ClassicCom>,
    ceHdr>
{
    // ���b�Z�[�W�{�b�N�X�\���ȂǂŎg���E�B���h�E�n���h��
    HWND m_hWnd;

public:
    // �R���X�g���N�^�F�E�B���h�E�n���h�����󂯎��
    EnvHdr(HWND hWnd) : m_hWnd(hWnd) {}
    EnvHdr() = default;

    /////////////////////////////////////////////////////////////
    // MakeAndInitialize �ɂ���ČĂяo����鏉�������\�b�h
    HRESULT RuntimeClassInitialize(HWND hWnd) noexcept
    {
        m_hWnd = hWnd;
        return S_OK;
    }

    /////////////////////////////////////////////////////////////
    // �������������������玩���I�ɌĂяo����郁�\�b�h
    IFACEMETHODIMP Invoke(HRESULT envRes, ICoreWebView2Environment* env) override
    {
        // �������������s�����ꍇ�A�G���[���b�Z�[�W��\��
        if (FAILED(envRes))
        {
            MessageBox(m_hWnd,
                L"WebView2 �����������s",
                L"Error",
                MB_OK);
            return envRes;
        }

        /////////////////////////////////////////////////////////////
        // �R���g���[���[�iWebView �̒��g�̐��䕔�j���쐬���邽�߂̃n���h�����쐬
        ComPtr<ccHdr> ctrlHandler;
        HRESULT hr = MakeAndInitialize<CtrlHdr>(&ctrlHandler, m_hWnd);
        if (FAILED(hr))
        {
            MessageBox(m_hWnd,
                L"CtrlHdr �̏��������s",
                L"Error",
                MB_OK);
            return hr;
        }

        /////////////////////////////////////////////////////////////
        // WebView2 �R���g���[���[��񓯊��ō쐬�i������� CtrlHdr::Invoke ���Ă΂��j
        return env->CreateCoreWebView2Controller(m_hWnd, ctrlHandler.Get());
    }
};

/////////////////////////////////////////////////////////////
// WebView2 �R���g���[���[�i�\�����䕔�j���������ꂽ�Ƃ��ɌĂ΂��N���X
// CreateCoreWebView2Controller() �̃R�[���o�b�N��Ƃ��Ďg����
/////////////////////////////////////////////////////////////
class CtrlHdr :
    public RuntimeClass<RuntimeClassFlags<ClassicCom>,
    ccHdr>
{
    HWND m_hWnd;

public:
    // �R���X�g���N�^�i�e�E�B���h�E�̃n���h����ێ��j
    CtrlHdr(HWND hWnd) : m_hWnd(hWnd) {}
    CtrlHdr() : m_hWnd(nullptr) {}

    /////////////////////////////////////////////////////////
    // MakeAndInitialize �ŌĂ΂�鏉�������\�b�h
    HRESULT RuntimeClassInitialize(HWND hWnd) noexcept
    {
        m_hWnd = hWnd;
        return S_OK;
    }

    /////////////////////////////////////////////////////////
    // �R���g���[���[���������������玩���I�ɌĂяo����郁�\�b�h
    IFACEMETHODIMP Invoke(HRESULT ctrlRes, ICoreWebView2Controller* controller) override
    {
        // �R���g���[���[�������s��
        if (FAILED(ctrlRes) || !controller)
        {
            MessageBox(m_hWnd,
                L"WebView2 �R���g���[���[�������s",
                L"Error",
                MB_OK);
            return ctrlRes;
        }

        /////////////////////////////////////////////////////////
        // �O���[�o���ϐ��ɃR���g���[���[��ۑ��i��Ŏg�����߁j
        // �R���g���[���^WebView ���O���[�o���ɕێ�
        g_webviewController = controller;
        controller->get_CoreWebView2(&g_webview);

        /////////////////////////////////////////////////////////
        // �E�B���h�E�̃T�C�Y���擾���āAWebView ��S�̂ɍL����
        // �E�B���h�E�����ς��ɃT�C�Y�ݒ�
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        g_webviewController->put_Bounds(bounds);

        // �����y�[�W�փi�r�Q�[�g
        g_webview->Navigate(STARTURL);

        return S_OK;
    }
};



/////////////////////////////////////////////////////////////////
// WebView2�i�g�ݍ��݃u���E�U�j������������֐�
// hWnd�FWebView2 ��\������e�E�B���h�E�̃n���h��
/////////////////////////////////////////////////////////////////
void InitializeWebView2(HWND hWnd)
{
    /////////////////////////////////////////////////////////////
    // �@ ���I�v�V�������쐬���AWebView2 �̓���ݒ���s��
    // �����ł́u�v���L�V���g���v�Ȃǂ̋N���I�v�V������ݒ肷�邽�߂̃I�u�W�F�N�g���쐬����
    auto options = Make<CoreWebView2EnvironmentOptions>();

    /////////////////////////////////////////////////////////////
    // WebView2 �̋N���I�v�V�����Ɂu�ǉ��̃u���E�U�����v��ݒ�
    // PROXURL �̓v���L�V�ݒ�Ȃǂ̕�����i��: "--proxy-server=..."�j
    options->put_AdditionalBrowserArguments(PROXURL);

    /////////////////////////////////////////////////////////////
    // �A WebView2 �̊���񓯊��ŏ��������鏀��
    // ���������ł����Ƃ��ɌĂяo�����R�[���o�b�N�i�C�x���g�����N���X�j��p�ӂ���

    /////////////////////////////////////////////////////////////
    // �R�[���o�b�N�p�̃n���h���C���^�[�t�F�[�X���i�[����ϐ�
    ComPtr<ceHdr> envHandler;

    /////////////////////////////////////////////////////////////
    // WebView2 ���̏������������󂯎��N���X�iEnvHdr�j�𐶐����A
    // ���̃C���X�^���X�� envHandler �Ɋi�[����
    // hWnd �̓R�[���o�b�N���Ŏg�����߂ɓn���Ă���i�Ⴆ�΃G���[�\���p�j
    HRESULT hr = MakeAndInitialize<EnvHdr>(&envHandler, hWnd);
    if (FAILED(hr)) {
        // �C���X�^���X�쐬�����s������G���[���b�Z�[�W��\�����ď����I��
        MessageBox(hWnd,
            L"EnvHdr �̏��������s",
            L"Error",
            MB_OK);
        return;
    }

    /////////////////////////////////////////////////////////////
    // WebView2 �̊����쐬����֐��i�񓯊��j
    // options �͋N���ݒ�AenvHandler �͊������̏���������R�[���o�b�N
    // �����ł���� envHandler::Invoke() �������I�ɌĂ΂��
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,        // �u���E�U�̎��s�t�H���_�inullptr �Ŋ���l�j
        nullptr,        // ���[�U�[�f�[�^�t�H���_�inullptr �Ŋ���l�j
        options.Get(),  // �쐬�����I�v�V������n��
        envHandler.Get() // �������������ɌĂ΂��n���h����o�^
    );
}

#endif // _USE_LAMDA
/////////////////////////////////////////////////////////////////
// ���C���֐�
/////////////////////////////////////////////////////////////////
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShowCmd) 
{
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
