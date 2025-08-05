#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
#include <string>
#include <filesystem>
#include "aviutl2_sdk/output2.h"
#include "resource.h"

namespace apn
{
	//
	// 出力プラグインの名前です。
	//
	const auto plugin_name = L"連番ファイル出力 (BMP / JPG / GIF / TIFF / PNG)";
	const auto plugin_information = L"🐍連番ファイル出力🔖r1";
	const auto file_filter = L"BMP / JPG / GIF / TIFF / PNG File\0*.bmp;*.jpg;*.gif;*.tiff;*.png\0All File (*.*)\0*.*\0";

	//
	// このクラスはアプリケーションです。
	//
	struct App
	{
		HINSTANCE instance = {};
		std::filesystem::path path = {};

		WCHAR suffix_format[MAX_PATH] = L"_%05d";
		int quality = 90;
		BOOL png_alpha = TRUE;

		BOOL read()
		{
			try
			{
				::GetPrivateProfileString(L"config", L"suffix_format",
					suffix_format, suffix_format, std::size(suffix_format), path.c_str());
				quality = ::GetPrivateProfileIntW(L"config", L"quality", quality, path.c_str());
				png_alpha = ::GetPrivateProfileIntW(L"config", L"png_alpha", png_alpha, path.c_str());

				return TRUE;
			}
			catch (...)
			{
			}

			return FALSE;
		}

		BOOL write()
		{
			try
			{
				// コンフィグフォルダを作成します。
				std::filesystem::create_directories(path.parent_path());

				::WritePrivateProfileString(L"config", L"suffix_format", suffix_format, path.c_str());
				auto quality = std::to_wstring(this->quality);
				::WritePrivateProfileString(L"config", L"quality", quality.c_str(), path.c_str());
				auto png_alpha = std::to_wstring(this->png_alpha);
				::WritePrivateProfileString(L"config", L"png_alpha", png_alpha.c_str(), path.c_str());

				return TRUE;
			}
			catch (...)
			{
			}

			return FALSE;
		}
	} app;

	//
	// 指定されたファイル拡張子二対応するCLSIDを返します。
	//
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
	{
		UINT  num = 0;          // number of image encoders
		UINT  size = 0;         // size of the image encoder array in bytes

		ImageCodecInfo* pImageCodecInfo = NULL;

		GetImageEncodersSize(&num, &size);
		if(size == 0)
			return -1;  // Failure

		pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
		if(pImageCodecInfo == NULL)
			return -1;  // Failure

		GetImageEncoders(num, size, pImageCodecInfo);

		for(UINT j = 0; j < num; ++j)
		{
			if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}    
		}

		free(pImageCodecInfo);
		return -1;  // Failure
	}

	//
	// 連番ファイルを出力します。
	//
	bool func_output(OUTPUT_INFO* oip)
	{
		struct GdiplusManager {
			GdiplusStartupInput si;
			GdiplusStartupOutput so;
			ULONG_PTR token;
			ULONG_PTR hook_token;
			GdiplusManager() {
				si.SuppressBackgroundThread = TRUE;
				GdiplusStartup(&token, &si, &so);
				so.NotificationHook(&hook_token);
			}
			~GdiplusManager() {
				so.NotificationUnhook(hook_token);
				GdiplusShutdown(token);
			}
		} gdiplus_manager = {};

		// 出力ファイルのパスを取得します。
		std::filesystem::path path = oip->savefile;

		// 出力ファイルのフォルダを取得します。
		auto folder = path.parent_path();

		// 出力ファイルのステムを取得します。
		auto stem = path.stem().wstring();

		// 出力ファイルの拡張子を取得します。
		auto extension = path.extension().wstring();

		BITMAPINFO bi = {};
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth = oip->w;
		bi.bmiHeader.biHeight = oip->h;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 24;
		bi.bmiHeader.biCompression = BI_RGB;

		auto quality = (ULONG)app.quality;
		EncoderParameters encoderParameters = {};
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderQuality;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		encoderParameters.Parameter[0].Value = &quality;

		CLSID encoder = {}; int result = -1; BOOL png = FALSE;
		if (::lstrcmpiW(extension.c_str(), L".bmp") == 0) result = GetEncoderClsid(L"image/bmp", &encoder);
		else if (::lstrcmpiW(extension.c_str(), L".jpg") == 0) result = GetEncoderClsid(L"image/jpeg", &encoder);
		else if (::lstrcmpiW(extension.c_str(), L".gif") == 0) result = GetEncoderClsid(L"image/gif", &encoder);
		else if (::lstrcmpiW(extension.c_str(), L".tif") == 0) result = GetEncoderClsid(L"image/tiff", &encoder);
		else if (::lstrcmpiW(extension.c_str(), L".png") == 0) result = GetEncoderClsid(L"image/png", &encoder), png = TRUE;

		if (result == -1)
		{
			::MessageBoxW(nullptr, L"拡張子が無効です", plugin_name, MB_OK);

			return FALSE;
		}

		// フレームを走査します。
		for (decltype(oip->n) i = 0; i < oip->n; i++)
		{
			// 中止フラグがセットされている場合は中止します。
			if (oip->func_is_abort()) break;

			// 進捗状況を出力します。
			oip->func_rest_time_disp(i, oip->n);

			// サフィックス文字列を作成します。
			WCHAR suffix[MAX_PATH] = {};
			swprintf_s(suffix, app.suffix_format, i);

			// 実際の出力ファイルのフルパスを作成します。
			auto file_name = folder / (stem + suffix + extension);

			std::unique_ptr<Bitmap> bitmap;

			if (png && app.png_alpha)
			{
				// フレーム映像をRGB形式で取得します。
				auto pixelp = oip->func_get_video(i, mmioFOURCC('P', 'A', '6', '4'));
//				auto pixelp = oip->func_get_video(i, mmioFOURCC('H', 'F', '6', '4'));

				{
#pragma pack(push, 1)
					struct From { uint16_t r, g, b, a; };
					struct To { uint16_t b, g, r, a; };
//					struct To { uint16_t a, r, g, b; };
#pragma pack(pop)
					for (size_t i = 0; i < oip->w * oip->h; i++)
					{
						auto from = ((From*)pixelp)[i];
						auto& to = ((To*)pixelp)[i];

						to.r = from.r;
						to.g = from.g;
						to.b = from.b;
						to.a = from.a;

						int n = 0;
					}
				}

				// フレーム映像からビットマップを作成します。
//				bitmap = std::make_unique<Bitmap>(oip->w, oip->h, oip->w * 8, PixelFormat64bppARGB, (BYTE*)pixelp);
				bitmap = std::make_unique<Bitmap>(oip->w, oip->h, oip->w * 8, PixelFormat64bppPARGB, (BYTE*)pixelp);
			}
			else
			{
				// フレーム映像をRGB形式で取得します。
				auto pixelp = oip->func_get_video(i, BI_RGB);

				// フレーム映像からビットマップを作成します。
				bitmap = std::make_unique<Bitmap>(&bi, pixelp);
			}

			// ビットマップを保存します。
			auto status = (::lstrcmpiW(extension.c_str(), L".jpg") == 0) ?
				bitmap->Save(file_name.c_str(), &encoder, &encoderParameters) :
				bitmap->Save(file_name.c_str(), &encoder);

			if (status != S_OK)
			{
				::MessageBoxW(nullptr, L"ファイルの保存に失敗しました", plugin_name, MB_OK);

				return FALSE;
			}
		}

		return TRUE;
	}

	//
	// コンフィグダイアログを表示します。
	//
	bool func_config(HWND hwnd, HINSTANCE dll_hinst)
	{
		::DialogBoxW(dll_hinst, MAKEINTRESOURCE(IDD_CONFIG), hwnd,
			[](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR
		{
			switch (message)
			{
			case WM_INITDIALOG:
				{
					::SetDlgItemTextW(hwnd, IDC_FORMAT, app.suffix_format);
					::SetDlgItemInt(hwnd, IDC_QUALITY, app.quality, TRUE);
					::SendDlgItemMessageW(hwnd, IDC_PNG_ALPHA, BM_SETCHECK, app.png_alpha ? BST_CHECKED : BST_UNCHECKED, 0);

					break;
				}
			case WM_COMMAND:
				{
					auto id = LOWORD(wParam);
					auto code = HIWORD(wParam);
					auto control = (HWND)lParam;

					switch (id)
					{
					case IDOK:
						{
							::GetDlgItemTextW(hwnd, IDC_FORMAT, app.suffix_format, std::size(app.suffix_format));
							app.quality = ::GetDlgItemInt(hwnd, IDC_QUALITY, nullptr, TRUE);
							app.png_alpha = ::SendDlgItemMessageW(hwnd, IDC_PNG_ALPHA, BM_GETCHECK, 0, 0) == BST_CHECKED;

							app.write();

							::EndDialog(hwnd, id);

							break;
						}
					case IDCANCEL:
						{
							::EndDialog(hwnd, id);

							break;
						}
					}

					break;
				}
			}

			return 0;
		});

		return TRUE;
	}

	//
	// 出力設定のテキスト情報を返します。
	//
	LPCWSTR func_get_config_text()
	{
		return L"連番ファイル出力の設定";
	}

	//
	// 出力プラグインの構造体です。
	//
	OUTPUT_PLUGIN_TABLE output_plugin_table =
	{
		OUTPUT_PLUGIN_TABLE::FLAG_VIDEO,	// フラグ
		plugin_name,						// プラグインの名前
		file_filter,						// 出力ファイルのフィルタ
		plugin_information,					// プラグインの情報
		func_output,						// 出力時に呼ばれる関数へのポインタ
		func_config,						// 出力設定のダイアログを要求された時に呼ばれる関数へのポインタ (nullptrなら呼ばれません)
		func_get_config_text,				// 出力設定のテキスト情報を取得する時に呼ばれる関数へのポインタ (nullptrなら呼ばれません)
	};

	//
	// 出力プラグインの構造体を返します。
	//
	EXTERN_C __declspec(dllexport) OUTPUT_PLUGIN_TABLE* GetOutputPluginTable()
	{
		return &output_plugin_table;
	}

	//
	// エントリポイントです。
	//
	EXTERN_C BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
	{
		switch (reason)
		{
		case DLL_PROCESS_ATTACH:
			{
				::DisableThreadLibraryCalls(app.instance = instance);

				try
				{
					// 設定ファイル(ini)のパスを作成します。
					WCHAR module_file_name[MAX_PATH] = {};
					::GetModuleFileNameW(instance, module_file_name, std::size(module_file_name));
					app.path = module_file_name;
					app.path = app.path.parent_path() / L"al2" / L"config" /
						app.path.filename().replace_extension(L".ini");

					app.read();
				}
				catch (...)
				{
				}

				break;
			}
		case DLL_PROCESS_DETACH:
			{
				break;
			}
		}

		return TRUE;
	}
}
