// BrowserAppWebView2.cpp : Win32 + WebView2 サンプル
#include <windows.h>
#include <wrl.h>
#include <wil/com.h>
#include <wrl/wrappers/corewrappers.h>

#include "WebView2.h"          // WebView2 SDK のヘッダー
#include <WebView2EnvironmentOptions.h>  // ← これを追加

#pragma comment(lib, "ole32.lib")
//////////////////////////////////////////////////////////////////////////
// ★重要
// WevView2 Nugetパッケージをイントールすること。
// Implementation Nugetパッケージをインストールすること。
// C++17以降のバージョンを使用すること。
// プロジェクトのエントリポイントを「Windows アプリケーション」に変更すること。 [リンカー] → [システム]
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////
// 初期URL
#define STARTURL L"https://chatgpt.com/"

// プロキシ設定 特に指定しなくてもシステム設定を見に行く
//#define PROXURL L"--proxy-server=http://hoge:8080"
#define PROXURL L""

// インターフェース名をわかりやすく短縮するための別名定義（using）
// ccHdr は「Controller Completed Handler」、ceHdr は「Environment Completed Handler」
using namespace Microsoft::WRL;
using ccHdr = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
using ceHdr = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;

// メニュー ID
#define ID_FILE_OPEN    1001
#define ID_FILE_BACK    1002
#define ID_FILE_FORWARD 1003

// グローバル
//HWND g_hWnd = nullptr;
// グローバル変数を２つに分けて持つ
static ComPtr<ICoreWebView2Controller> g_webviewController = nullptr;
static ComPtr<ICoreWebView2>           g_webview = nullptr;

// Forward decl.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//#define _USE_LAMDA
#ifdef _USE_LAMDA
///////////////////////////////////////////////////////////////
// WebView2 環境の初期化
// コールバックで WebView2 コントローラーを生成
// コントローラーの Bounds を設定し、初期 URL にナビゲート
// 引数: hWnd - メインウィンドウのハンドル
// 戻り値: なし
// 注意: WebView2 の初期化は非同期で行われるため、
//       コールバックでコントローラーを生成する必要がある
//       コールバックの中で WebView2 コントローラーを保持する
//       変数をグローバルに持つ必要がある
//       コントローラーの Bounds を設定するため、
//       メインウィンドウのハンドルを引数に渡す
///////////////////////////////////////////////////////////////
void InitializeWebView2(HWND hWnd)
{
    // ① 環境オプションを作成し、プロキシ引数を設定
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	options->put_AdditionalBrowserArguments(PROXURL); // プロキシ設定

	// ② WebView2 環境を非同期で初期化
    CreateCoreWebView2EnvironmentWithOptions
    (
        nullptr, 
        nullptr, 
        options.Get(),  // ← ここに options.Get() を渡す！
        Callback<ceHdr>
        (
			// ラムダ式でコールバックを実装
            [hWnd](HRESULT envRes, ICoreWebView2Environment* env) -> HRESULT 
            {
                if (FAILED(envRes)) 
                {
                    MessageBox(hWnd, L"WebView2 環境初期化失敗", L"Error", MB_OK);
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
                                MessageBox(hWnd, L"WebView2 コントローラー生成失敗", L"Error", MB_OK);
                                return ctrlRes;
                            }
                            // コントローラーと WebView を保持
                            g_webviewController = controller;
                            controller->get_CoreWebView2(&g_webview);

                            // **① 初期バウンドを設定**  
                            RECT bounds;
                            GetClientRect(hWnd, &bounds);
                            g_webviewController->put_Bounds(bounds);

                            // 初期 URL をナビゲート
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
// COM（Component Object Model）は、異なるソフトウェア同士を連携させる仕組みです。
// Windowsでは、WebView2やOfficeなど多くの機能がCOMで提供されています。
// COMでは「インターフェース」という契約に従って機能を使います。
// 関数の代わりに、特定のメソッドを持つオブジェクト（ハンドラ）を渡して処理します。
// C++では、COMオブジェクトを自作する場合、インターフェースを実装したクラスが必要です。
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// EnvHdr::Invoke() は「WebView2 環境（エンジン）が準備できたら」呼ばれる。
// CtrlHdr::Invoke() は「WebView2 のコントローラー（実際の画面を制御する部分）が準備できたら」呼ばれる。
// 両方とも 非同期で自動的に呼ばれる。手動で Invoke() を呼ぶことはしない。
// MakeAndInitialize<T>(&ptr, args...) は COM 用のインスタンス生成＋初期化の便利関数。
// COM クラスでは RuntimeClassInitialize() が初期化の入口になる。
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// 環境初期化完了時のハンドラ
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

class CtrlHdr;

///////////////////////////////////////////////////////////////
// WebView2 環境の初期化が完了したときに呼ばれるクラス
// CreateCoreWebView2EnvironmentWithOptions() のコールバック先として使われる
///////////////////////////////////////////////////////////////
class EnvHdr :
    public RuntimeClass<RuntimeClassFlags<ClassicCom>,
    ceHdr>
{
    // メッセージボックス表示などで使うウィンドウハンドル
    HWND m_hWnd;

public:
    // コンストラクタ：ウィンドウハンドルを受け取る
    EnvHdr(HWND hWnd) : m_hWnd(hWnd) {}
    EnvHdr() = default;

    /////////////////////////////////////////////////////////////
    // MakeAndInitialize によって呼び出される初期化メソッド
    HRESULT RuntimeClassInitialize(HWND hWnd) noexcept
    {
        m_hWnd = hWnd;
        return S_OK;
    }

    /////////////////////////////////////////////////////////////
    // 環境初期化が完了したら自動的に呼び出されるメソッド
    IFACEMETHODIMP Invoke(HRESULT envRes, ICoreWebView2Environment* env) override
    {
        // 環境初期化が失敗した場合、エラーメッセージを表示
        if (FAILED(envRes))
        {
            MessageBox(m_hWnd,
                L"WebView2 環境初期化失敗",
                L"Error",
                MB_OK);
            return envRes;
        }

        /////////////////////////////////////////////////////////////
        // コントローラー（WebView の中身の制御部）を作成するためのハンドラを作成
        ComPtr<ccHdr> ctrlHandler;
        HRESULT hr = MakeAndInitialize<CtrlHdr>(&ctrlHandler, m_hWnd);
        if (FAILED(hr))
        {
            MessageBox(m_hWnd,
                L"CtrlHdr の初期化失敗",
                L"Error",
                MB_OK);
            return hr;
        }

        /////////////////////////////////////////////////////////////
        // WebView2 コントローラーを非同期で作成（完了後は CtrlHdr::Invoke が呼ばれる）
        return env->CreateCoreWebView2Controller(m_hWnd, ctrlHandler.Get());
    }
};

/////////////////////////////////////////////////////////////
// WebView2 コントローラー（表示制御部）が生成されたときに呼ばれるクラス
// CreateCoreWebView2Controller() のコールバック先として使われる
/////////////////////////////////////////////////////////////
class CtrlHdr :
    public RuntimeClass<RuntimeClassFlags<ClassicCom>,
    ccHdr>
{
    HWND m_hWnd;

public:
    // コンストラクタ（親ウィンドウのハンドルを保持）
    CtrlHdr(HWND hWnd) : m_hWnd(hWnd) {}
    CtrlHdr() : m_hWnd(nullptr) {}

    /////////////////////////////////////////////////////////
    // MakeAndInitialize で呼ばれる初期化メソッド
    HRESULT RuntimeClassInitialize(HWND hWnd) noexcept
    {
        m_hWnd = hWnd;
        return S_OK;
    }

    /////////////////////////////////////////////////////////
    // コントローラー生成が完了したら自動的に呼び出されるメソッド
    IFACEMETHODIMP Invoke(HRESULT ctrlRes, ICoreWebView2Controller* controller) override
    {
        // コントローラー生成失敗時
        if (FAILED(ctrlRes) || !controller)
        {
            MessageBox(m_hWnd,
                L"WebView2 コントローラー生成失敗",
                L"Error",
                MB_OK);
            return ctrlRes;
        }

        /////////////////////////////////////////////////////////
        // グローバル変数にコントローラーを保存（後で使うため）
        // コントローラ／WebView をグローバルに保持
        g_webviewController = controller;
        controller->get_CoreWebView2(&g_webview);

        /////////////////////////////////////////////////////////
        // ウィンドウのサイズを取得して、WebView を全体に広げる
        // ウィンドウいっぱいにサイズ設定
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        g_webviewController->put_Bounds(bounds);

        // 初期ページへナビゲート
        g_webview->Navigate(STARTURL);

        return S_OK;
    }
};



/////////////////////////////////////////////////////////////////
// WebView2（組み込みブラウザ）を初期化する関数
// hWnd：WebView2 を表示する親ウィンドウのハンドル
/////////////////////////////////////////////////////////////////
void InitializeWebView2(HWND hWnd)
{
    /////////////////////////////////////////////////////////////
    // ① 環境オプションを作成し、WebView2 の動作設定を行う
    // ここでは「プロキシを使う」などの起動オプションを設定するためのオブジェクトを作成する
    auto options = Make<CoreWebView2EnvironmentOptions>();

    /////////////////////////////////////////////////////////////
    // WebView2 の起動オプションに「追加のブラウザ引数」を設定
    // PROXURL はプロキシ設定などの文字列（例: "--proxy-server=..."）
    options->put_AdditionalBrowserArguments(PROXURL);

    /////////////////////////////////////////////////////////////
    // ② WebView2 の環境を非同期で初期化する準備
    // 環境が準備できたときに呼び出されるコールバック（イベント処理クラス）を用意する

    /////////////////////////////////////////////////////////////
    // コールバック用のハンドラインターフェースを格納する変数
    ComPtr<ceHdr> envHandler;

    /////////////////////////////////////////////////////////////
    // WebView2 環境の初期化完了を受け取るクラス（EnvHdr）を生成し、
    // そのインスタンスを envHandler に格納する
    // hWnd はコールバック内で使うために渡している（例えばエラー表示用）
    HRESULT hr = MakeAndInitialize<EnvHdr>(&envHandler, hWnd);
    if (FAILED(hr)) {
        // インスタンス作成が失敗したらエラーメッセージを表示して処理終了
        MessageBox(hWnd,
            L"EnvHdr の初期化失敗",
            L"Error",
            MB_OK);
        return;
    }

    /////////////////////////////////////////////////////////////
    // WebView2 の環境を作成する関数（非同期）
    // options は起動設定、envHandler は完了時の処理をするコールバック
    // 環境ができると envHandler::Invoke() が自動的に呼ばれる
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,        // ブラウザの実行フォルダ（nullptr で既定値）
        nullptr,        // ユーザーデータフォルダ（nullptr で既定値）
        options.Get(),  // 作成したオプションを渡す
        envHandler.Get() // 初期化完了時に呼ばれるハンドラを登録
    );
}

#endif // _USE_LAMDA
/////////////////////////////////////////////////////////////////
// メイン関数
/////////////////////////////////////////////////////////////////
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShowCmd) 
{
    // STA モードで OLE 初期化
    if (FAILED(OleInitialize(NULL))) {
        MessageBox(NULL, L"OleInitialize に失敗", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // ウィンドウクラス登録
    const wchar_t CLASS_NAME[] = L"WebView2BrowserClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // メニュー作成
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_OPEN, L"開く");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_BACK, L"戻る");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_FORWARD, L"進む");
    HMENU hMenu = CreateMenu();
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"ファイル");

    // メインウィンドウ生成
    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Simple Browser",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        1024, 768, nullptr, hMenu, hInst, nullptr
    );
    if (!hWnd) {
        MessageBox(NULL, L"メインウィンドウ生成失敗", L"Error", MB_OK | MB_ICONERROR);
        OleUninitialize();
        return 1;
    }

    ShowWindow(hWnd, nShowCmd);
    UpdateWindow(hWnd);

    // WebView2 の非同期初期化
    InitializeWebView2(hWnd);

    // メッセージループ
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    OleUninitialize();
    return 0;
}

/////////////////////////////////////////////////////////////////
// ウィンドウプロシージャ
/////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) 
    {
        //ウィンドウサイズ変更処理
        case WM_SIZE:
        if (g_webviewController) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            // コントローラーにバウンドを直接設定
            g_webviewController->put_Bounds(rc);
        }
        break;

        //メニュー処理
        case WM_COMMAND:
        switch (LOWORD(wp)) 
        {
            case ID_FILE_OPEN: 
            {
                // ローカル HTML ファイル選択
                OPENFILENAMEW ofn{ sizeof(ofn) };
                wchar_t szFile[MAX_PATH]{};
                ofn.hwndOwner = hWnd;
                ofn.lpstrFilter = L"HTML Files\0*.htm;*.html\0All Files\0*.*\0";
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) 
                {
                    // file:/// に変換して Navigate
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

        //ウィンドウが閉じられたとき
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, msg, wp, lp);
    }
    return 0;
}
