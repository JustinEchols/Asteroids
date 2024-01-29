
#include "game_platform.h"

#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <malloc.h>
#include <dsound.h>

#include "win32_game.h"

global_variable b32					Win32GlobalRunning;
global_variable win32_back_buffer	Win32GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER Win32GlobalSecondaryBuffer;
global_variable LARGE_INTEGER		Win32GlobalTickFrequency;
global_variable WINDOWPLACEMENT		GlobalWindowPosition = {sizeof(GlobalWindowPosition)};


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
win32_toggle_full_screen(HWND Window)
{
	DWORD window_style = GetWindowLong(Window, GWL_STYLE);
	if(window_style & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
		if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
		   GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
		{
			SetWindowLong(Window, GWL_STYLE, window_style & ~WS_OVERLAPPEDWINDOW);

			SetWindowPos(Window, HWND_TOP,
					MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
					MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
					MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
					SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		SetWindowLong(Window, GWL_STYLE, window_style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(Window, &GlobalWindowPosition);
		SetWindowPos(Window, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}	
}

inline FILETIME
win32_file_last_write_time(char *filename)
{
	FILETIME Result = {};

	WIN32_FIND_DATA FindData;
	HANDLE FileHandle = FindFirstFileA(filename, &FindData);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		Result = FindData.ftLastWriteTime;
		CloseHandle(FileHandle);
	}

	return(Result);
}


internal win32_game_code 
win32_game_code_load(char *src_dll_full_path, char *temp_dll_full_path)
{
	win32_game_code Result = {};

	Result.DLLLastWriteTime = win32_file_last_write_time(src_dll_full_path);

	CopyFile(src_dll_full_path, temp_dll_full_path, FALSE);

	Result.GameCodeDLL = LoadLibraryA(temp_dll_full_path);
	if(Result.GameCodeDLL)
	{
		Result.UpdateAndRender = (game_update_and_render *)
			GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

		Result.GetSoundSamples = (game_get_sound_samples *)
			GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

		Result.is_valid = (Result.UpdateAndRender && Result.GetSoundSamples);
	}

	if(!Result.is_valid)
	{
		Result.UpdateAndRender = 0;
		Result.GetSoundSamples = 0;
	}

	return(Result);
}


#if 0
internal win32_game_code 
win32_game_code_load(char *src_dll_full_path, char *temp_dll_full_path, char *lock_file_full_path)
{
	win32_game_code Result = {};

	WIN32_FILE_ATTRIBUTE_DATA Ignored;
	if(!GetFileAttributesEx(lock_file_full_path, GetFileExInfoStandard, &Ignored))
	{
		Result.DLLLastWriteTime = win32_file_last_write_time(src_dll_full_path);

		CopyFile(src_dll_full_path, temp_dll_full_path, FALSE);

		Result.GameCodeDLL = LoadLibraryA(temp_dll_full_path);
		if(Result.GameCodeDLL)
		{
			Result.update_and_render = (GameUpdateAndRender *)
				GetProcAddress(Result.GameCodeDLL, "game_update_and_render");

			Result.sound_samples_get = (GameSoundSamplesGet *)
				GetProcAddress(Result.GameCodeDLL, "game_sound_samples_get");

			Result.is_valid = (Result.update_and_render && Result.sound_samples_get);
		}
	}

	if(!Result.is_valid)
	{
		Result.update_and_render = 0;
		Result.sound_samples_get = 0;
	}

	return(Result);
}
#endif

internal void
win32_game_code_unload(win32_game_code *GameCode)
{
	if(GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}

	GameCode->is_valid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;
}

internal void
win32_dsound_init(HWND Window, s32 secondary_buffer_size)
{
	HMODULE dsound_dll = LoadLibraryA("dsound.dll");
	if(dsound_dll)
	{
		direct_sound_create *dsound_create =
			(direct_sound_create *)GetProcAddress(dsound_dll, "DirectSoundCreate");

		LPDIRECTSOUND DSound;
		if(dsound_create)
		{
			if(SUCCEEDED(dsound_create(0, &DSound, 0)))
			{
				WAVEFORMATEX WaveFormat = {};
				WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
				WaveFormat.nChannels = 2;
				WaveFormat.nSamplesPerSec = 44100;
				WaveFormat.wBitsPerSample = 16;	
				WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
				WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
				WaveFormat.cbSize = 0;

				if(SUCCEEDED(DSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
				{
					DSBUFFERDESC BufferDesc;
					BufferDesc.dwSize = sizeof(BufferDesc);
					BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
					BufferDesc.dwBufferBytes = 0;
					BufferDesc.dwReserved = 0;
					BufferDesc.lpwfxFormat = NULL;
					BufferDesc.guid3DAlgorithm = GUID_NULL;

					LPDIRECTSOUNDBUFFER PrimaryBuffer;
					if(SUCCEEDED(DSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0)))
					{
						if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
						{
							OutputDebugStringA("Primary buffer format set successfully.\n");
						}
						else
						{
							// TODO: Diagnostic
						}
					}
					else
					{
						// TODO: Diagnostic
					}
				}
				else
				{
					// TODO: Diagnostic
				}

				DSBUFFERDESC BufferDesc = {0};
				BufferDesc.dwSize = sizeof(BufferDesc);
				BufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS;
				BufferDesc.dwBufferBytes = secondary_buffer_size;
				BufferDesc.dwReserved = 0;
				BufferDesc.lpwfxFormat = &WaveFormat;
				BufferDesc.guid3DAlgorithm = GUID_NULL;

				// NOTE(Justin): Using Win32GlobalSecondaryBuffer here!
				if(SUCCEEDED(DSound->CreateSoundBuffer(&BufferDesc, &Win32GlobalSecondaryBuffer, 0)))
				{
					OutputDebugStringA("Secondary buffer created successfully.\n");
				}
				else
				{
					// TODO: Diagnostic
				}
			}
			else
			{
				// TODO: Diagnostic
			}
		}
		else
		{
			// TODO: Diagnostic
		}
	}
	else
	{
		// TODO: Diagnostic
	}
}

internal void
win32_sound_buffer_clear(win32_sound_buffer *Win32SoundBuffer)
{
	VOID *region_1;
	DWORD region_1_size;
	VOID *region_2;
	DWORD region_2_size;


	HRESULT secondary_buffer_lock = 
		Win32GlobalSecondaryBuffer->Lock(0, Win32SoundBuffer->secondary_buffer_size,
										 &region_1, &region_1_size,
										 &region_2, &region_2_size,
										 DSBLOCK_ENTIREBUFFER);

	if(SUCCEEDED(secondary_buffer_lock))
	{
		u8 *sample_byte = (u8 *)region_1;
		for(DWORD byte_index = 0; byte_index < region_1_size; byte_index++)
		{
			*sample_byte++ = 0;
		}

		sample_byte = (u8 *)region_2;
		for(DWORD byte_index = 0; byte_index < region_2_size; byte_index++)
		{
			*sample_byte++ = 0;
		}

		HRESULT secondary_buffer_unlock =
			Win32GlobalSecondaryBuffer->Unlock(region_1, region_1_size,
											   region_2, region_2_size);

		if(SUCCEEDED(secondary_buffer_unlock))
		{
			return;
		}
		else
		{
		}
	}
	else
	{
	}
}

internal void
win32_sound_buffer_fill(win32_sound_buffer *Win32SoundBuffer, DWORD byte_to_lock, DWORD bytes_to_write,
		sound_buffer *SoundBuffer)
{
	VOID *region_1;
	DWORD region_1_size;
	VOID *region_2;
	DWORD region_2_size;

	HRESULT secondary_buffer_lock =
		Win32GlobalSecondaryBuffer->Lock(byte_to_lock,
										 bytes_to_write,
										 &region_1, &region_1_size,
										 &region_2, &region_2_size,
										 0);

	if(SUCCEEDED(secondary_buffer_lock))
	{
		DWORD region_1_sample_count = region_1_size / Win32SoundBuffer->bytes_per_sample;

		s16 *sample_src = SoundBuffer->samples;
		s16 *sample_dest = (s16 *)region_1;
		for(DWORD sample_index = 0; sample_index < region_1_sample_count; sample_index++)
		{
			*sample_dest++ = *sample_src++;
			*sample_dest++ = *sample_src++;

			++Win32SoundBuffer->sample_count;
		}

		DWORD region_2_sample_count = region_2_size / Win32SoundBuffer->bytes_per_sample;
		sample_dest = (s16 *)region_2;
		for(DWORD sample_index = 0; sample_index < region_2_sample_count; sample_index++)
		{
			*sample_dest++ = *sample_src++;
			*sample_dest++ = *sample_src++;

			++Win32SoundBuffer->sample_count;
		}

		Win32GlobalSecondaryBuffer->Unlock(region_1, region_1_size,
										   region_2, region_2_size);
	}
}

internal void
win32_opengl_init(HWND Window)
{
	HDC WindowDC = GetDC(Window);

	PIXELFORMATDESCRIPTOR DesiredPF = {};

	DesiredPF.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	DesiredPF.nVersion = 1;
	DesiredPF.iPixelType = PFD_TYPE_RGBA;
	DesiredPF.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	DesiredPF.cColorBits = 32;
	DesiredPF.cAlphaBits = 8;
	DesiredPF.iLayerType = PFD_MAIN_PLANE;

	int suggested_pixel_format = ChoosePixelFormat(WindowDC, &DesiredPF);
	PIXELFORMATDESCRIPTOR SuggestedPF;
	DescribePixelFormat(WindowDC, suggested_pixel_format, sizeof(SuggestedPF), &SuggestedPF);

	SetPixelFormat(WindowDC, suggested_pixel_format, &SuggestedPF);

	HGLRC OpenGLRC = wglCreateContext(WindowDC);
	if(wglMakeCurrent(WindowDC, OpenGLRC))
	{
		// NOTE(Justin): Success
	}
	else
	{
		INVALID_CODE_PATH;
	}
	ReleaseDC(Window, WindowDC);
}


internal void
win32_back_buffer_resize(win32_back_buffer *Win32BackBuffer, int width, int height)
{
	if(Win32BackBuffer->memory)
	{
		VirtualFree(Win32BackBuffer->memory, 0, MEM_RELEASE);
	}

	Win32BackBuffer->width = width;
	Win32BackBuffer->height = height;
	Win32BackBuffer->bytes_per_pixel = 4;
	Win32BackBuffer->stride = Win32BackBuffer->width * Win32BackBuffer->bytes_per_pixel;

	Win32BackBuffer->Info.bmiHeader.biSize = sizeof(Win32BackBuffer->Info.bmiHeader);
	Win32BackBuffer->Info.bmiHeader.biWidth = Win32BackBuffer->width;
	Win32BackBuffer->Info.bmiHeader.biHeight = Win32BackBuffer->height;
	Win32BackBuffer->Info.bmiHeader.biPlanes = 1;
	Win32BackBuffer->Info.bmiHeader.biBitCount = 32;
	Win32BackBuffer->Info.bmiHeader.biCompression = BI_RGB;

	int bitmap_memory_size =  
		Win32BackBuffer->width * Win32BackBuffer->height * Win32BackBuffer->bytes_per_pixel;

	Win32BackBuffer->memory = VirtualAlloc(0, bitmap_memory_size, 
			MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal win32_client_dimensions
win32_get_window_dimension(HWND Window)
{
	win32_client_dimensions Result = {};

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.width = ClientRect.right - ClientRect.left;
	Result.height = ClientRect.top - ClientRect.bottom;

	return(Result);
}

internal void
win32_display_buffer_to_window(win32_back_buffer *Win32BackBuffer, HDC DeviceContext,
														int window_width, int window_height)
{
#if 0
	if((window_width >= Win32BackBuffer->width * 2) &&
			(window_height >= Win32BackBuffer->height * 2))
	{
		StretchDIBits(DeviceContext,
				0, 0, Win32BackBuffer->width * 2, Win32BackBuffer->height * 2,
				0, 0, Win32BackBuffer->width, Win32BackBuffer->height,
				Win32BackBuffer->memory, &Win32BackBuffer->Info,
				DIB_RGB_COLORS, SRCCOPY);

	}
	else
	{
		StretchDIBits(DeviceContext,
				0, 0, Win32BackBuffer->width, Win32BackBuffer->height,
				0, 0, Win32BackBuffer->width, Win32BackBuffer->height,
				Win32BackBuffer->memory, &Win32BackBuffer->Info,
				DIB_RGB_COLORS, SRCCOPY);
	}
#endif
	glViewport(0, 0, window_width, window_height);

	GLuint TextureHandle = 0;
	static int init = false;
	if(!init)
	{
		glGenTextures(1, &TextureHandle);
		init = true;
	}

	glBindTexture(GL_TEXTURE_2D, TextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Win32BackBuffer->width, Win32BackBuffer->height, 0,
			GL_BGRA_EXT, GL_UNSIGNED_BYTE, Win32BackBuffer->memory);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable(GL_TEXTURE_2D);

	glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	

	glBegin(GL_TRIANGLES);

	f32 p = 1.0f;

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-p, -p);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(p, -p);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(p, p);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-p, -p);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(p, p);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-p, p);

	glEnd();

	SwapBuffers(DeviceContext);

}


LRESULT CALLBACK
win32_main_window_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch (Message)
	{
		case WM_CLOSE:
		{
			Win32GlobalRunning = false;
		} break;
		case WM_DESTROY:
		{
			// TODO(Justin): Handle as an error.
			Win32GlobalRunning = false;
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			// TODO(Justin): Check this.
			//ASSERT(!"Keyboard input came in through a non-dispatch message!")
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT PaintStruct;
			HDC DeviceContext = BeginPaint(Window, &PaintStruct);

			win32_client_dimensions WindowDimensions = win32_get_window_dimension(Window);

			win32_display_buffer_to_window(&Win32GlobalBackBuffer, DeviceContext, WindowDimensions.width, WindowDimensions.height);

			EndPaint(Window, &PaintStruct);
		} break;
		default:
		{
			Result = DefWindowProc(Window, Message, wParam, lParam);
		} break;
	}
	return(Result);
}

internal void
win32_process_keyboard_messages(game_button_state *NewState, b32 is_down)
{
	if(NewState->ended_down != is_down)
	{
		NewState->ended_down = is_down;
		++NewState->half_transition_count;
	}
}

internal void
win32_process_pending_messgaes(game_controller_input *KeyboardController)
{
	MSG Message; 
	while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_QUIT:
			{
				Win32GlobalRunning = false;
			} break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				u32 vk_code = (u32)Message.wParam;
				b32 key_was_down = ((Message.lParam & (1 << 30)) != 0);
				b32 key_is_down = ((Message.lParam & (1 << 31)) == 0);
				if(key_was_down != key_is_down)
				{
					switch(vk_code)
					{
						case VK_LBUTTON:
						{
						} break;
						case VK_RBUTTON:
						{
						} break;
						case VK_LEFT:
						{
							win32_process_keyboard_messages(&KeyboardController->ArrowLeft, key_is_down);
						} break;
						case VK_UP:
						{
							win32_process_keyboard_messages(&KeyboardController->ArrowUp, key_is_down);
						} break;
						case VK_DOWN:
						{
							win32_process_keyboard_messages(&KeyboardController->ArrowDown, key_is_down);
						} break;
						case VK_RIGHT:
						{
							win32_process_keyboard_messages(&KeyboardController->ArrowRight, key_is_down);
						} break;
						case VK_SPACE:
						{
							win32_process_keyboard_messages(&KeyboardController->Space, key_is_down);
						} break;
						case VK_RETURN:
						{
							win32_process_keyboard_messages(&KeyboardController->Enter, key_is_down);
						} break;
						case 'W':
						{
							win32_process_keyboard_messages(&KeyboardController->Up, key_is_down);
						} break;
						case 'A':
						{
							win32_process_keyboard_messages(&KeyboardController->Left, key_is_down);
						} break;
						case 'S':
						{
							win32_process_keyboard_messages(&KeyboardController->Down, key_is_down);
						} break;
						case 'D':
						{
							win32_process_keyboard_messages(&KeyboardController->Right, key_is_down);
						} break;
						case VK_SHIFT:
						{
							win32_process_keyboard_messages(&KeyboardController->Shift, key_is_down);
						} break;
						case VK_ESCAPE:
						{
							OutputDebugStringA("Escape");
							Win32GlobalRunning = false;
						} break;

					}
					if(key_is_down)
					{
						b32 alt_key_was_down = (Message.lParam & (1 << 29));
						if((vk_code == VK_F4) && (alt_key_was_down))
						{
							Win32GlobalRunning = false;
						}
						if((vk_code == VK_RETURN) && (alt_key_was_down))
						{
							if(Message.hwnd)
							{
								win32_toggle_full_screen(Message.hwnd);
							}
						}
					}
				}
			}
			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			} break;
		}
	}
}

inline LARGE_INTEGER
win32_get_wall_clock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline f32
win32_get_seconds_elapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	f32 Result = ((f32)(End.QuadPart - Start.QuadPart)) / (f32)(Win32GlobalTickFrequency.QuadPart);
	return(Result);
}

DEBUG_PLATFORM_FILE_FREE_MEMORY(debug_platform_file_free_memory)
{
	if(memory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
	}
}

DEBUG_PLATFORM_FILE_READ_ENTIRE(debug_platform_file_read_entire)
{
	debug_file_read_result  Result = {};
	HANDLE FileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD FileSize = GetFileSize(FileHandle, &FileSize);
		if(FileSize != 0)
		{
			Result.contents = VirtualAlloc(0, FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if(Result.contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.contents, FileSize, &BytesRead, 0) && (FileSize == BytesRead))
				{
					Result.size = FileSize;
				}
				else
				{
					debug_platform_file_free_memory(Thread, Result.contents);
					Result.contents = 0;
				}
			}
			else
			{
				// TODO(Justin): Log
			}
		}
		else
		{
				// TODO(Justin): Log
		}

		CloseHandle(FileHandle);
	}
	else
	{
		// TODO(Justin): Log
	}

	return(Result);
}

DEBUG_PLATFORM_FILE_WRITE_ENTIRE(debug_platform_file_write_entire)
{
	b32 Result = false;
	HANDLE FileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD bytes_written;
		if(WriteFile(FileHandle, memory, memory_size, &bytes_written, 0))
		{
			Result = (bytes_written == memory_size);
		}
		else
		{
			// TODO(Justin): Log
		}

		CloseHandle(FileHandle);
	}
	else
	{
			// TODO(Justin): Log
	}

	return(Result);
}

internal void
str_cat(size_t src_a_count, char *src_a,
		size_t src_b_count, char *src_b,
		size_t dest_count, char *dest)
{
	for(int index = 0; index < src_a_count; index++)
	{
		*dest++ = *src_a++;
	}

	for(int index = 0; index < src_b_count; index++)
	{
		*dest++ = *src_b++;
	}

	*dest = 0;
}


int CALLBACK 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR CmdLine, int nCmdShow)
{
	char ExeFullPathBuffer[MAX_PATH];
	DWORD exe_filename_size = GetModuleFileNameA(0, ExeFullPathBuffer, sizeof(ExeFullPathBuffer));
	char *one_past_last_slash = ExeFullPathBuffer;

	for(char *c = ExeFullPathBuffer; *c; c++)
	{
		if(*c == '\\')
		{
			one_past_last_slash = c + 1;
		}
	}

	char SourceGameDLLFilename[] = "game.dll";
	char TempGameDLLFilename[] = "game_temp.dll";
	char LockFilename[] = "lock.tmp";

	char SourceGameDLLFullPath[MAX_PATH];
	char TempGameDLLFullPath[MAX_PATH];
	char LockFileFullPath[MAX_PATH];

	str_cat(one_past_last_slash - ExeFullPathBuffer, ExeFullPathBuffer,
			sizeof(SourceGameDLLFilename) - 1, SourceGameDLLFilename,
			sizeof(SourceGameDLLFullPath), SourceGameDLLFullPath);

	str_cat(one_past_last_slash - ExeFullPathBuffer, ExeFullPathBuffer,
			sizeof(TempGameDLLFilename) - 1, TempGameDLLFilename,
			sizeof(TempGameDLLFullPath), TempGameDLLFullPath);

	str_cat(one_past_last_slash - ExeFullPathBuffer, ExeFullPathBuffer,
			sizeof(LockFilename) - 1, LockFilename,
			sizeof(LockFileFullPath), LockFileFullPath);


	const char window_class_name[]	= "TestWindowClass";
	const char window_title[]		= "Asteroids";

	WNDCLASSA WindowClass = {};
	HWND Window;

	WindowClass.style			= CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc		= win32_main_window_callback;
	WindowClass.hInstance		= hInstance;
	WindowClass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	WindowClass.hCursor			= LoadCursor(NULL, IDC_ARROW);
	WindowClass.lpszClassName	= window_class_name;

	// f	= 10MHz,
	// 1/f	= 1e-7  = 0.1 µs
	QueryPerformanceFrequency(&Win32GlobalTickFrequency);
	s64 ticks_per_second = Win32GlobalTickFrequency.QuadPart;
	f32 time_for_each_tick = 1.0f / (f32)ticks_per_second;

	u32 min_miliseconds_to_sleep = 1;
	MMRESULT sleep_is_granular = timeBeginPeriod(min_miliseconds_to_sleep);

	int monitor_hz = 60;
	int game_hz = monitor_hz / 2;
	f32 seconds_per_frame_target = 1.0f / (f32)game_hz;

	if(RegisterClassA(&WindowClass))
	{
		Window = CreateWindowExA(0,
				WindowClass.lpszClassName,
				window_title,
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				0,
				0,
				hInstance,
				0);
		if(Window)
		{
			win32_opengl_init(Window);
			win32_back_buffer_resize(&Win32GlobalBackBuffer, 960, 540);
			//win32_back_buffer_resize(&Win32GlobalBackBuffer, 1920, 1080);

			win32_sound_buffer Win32SoundBuffer = {};

			Win32SoundBuffer.samples_per_second = 48000;
			Win32SoundBuffer.bytes_per_sample = sizeof(s16) * 2;
			Win32SoundBuffer.secondary_buffer_size = (Win32SoundBuffer.samples_per_second * Win32SoundBuffer.bytes_per_sample);
			Win32SoundBuffer.sample_count = 0;
			Win32SoundBuffer.wave_frequency = 256;
			Win32SoundBuffer.wave_amplitude = 6000;
			Win32SoundBuffer.samples_per_period = (Win32SoundBuffer.samples_per_second / Win32SoundBuffer.wave_frequency);
			Win32SoundBuffer.sample_count_latency = Win32SoundBuffer.samples_per_second / 15;//TODO more aggresive latency 1/30? 1/60?


			s16 *samples = (s16 *)VirtualAlloc((LPVOID)0, Win32SoundBuffer.secondary_buffer_size, 
			MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if GAME_INTERNAL
			LPVOID base_address = (LPVOID)TERABYTES(2);
#else
			LPVOID base_address = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.permanent_storage_size = MEGABYTES(32);
			GameMemory.transient_storage_size = MEGABYTES(4);
			GameMemory.debug_platform_file_free_memory =  debug_platform_file_free_memory;
			GameMemory.debug_platform_file_read_entire =  debug_platform_file_read_entire;
			GameMemory.debug_platform_file_write_entire =  debug_platform_file_write_entire;

			u64 total_size = GameMemory.permanent_storage_size + GameMemory.transient_storage_size;
			GameMemory.permanent_storage = VirtualAlloc(base_address, (size_t)total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.transient_storage = (u8 *)GameMemory.permanent_storage + GameMemory.permanent_storage_size;


			if((GameMemory.permanent_storage) && (GameMemory.transient_storage) && samples)
			{

				win32_dsound_init(Window, Win32SoundBuffer.secondary_buffer_size);

				win32_sound_buffer_clear(&Win32SoundBuffer);

				Win32GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

				game_input GameInput[2] = {0};
				game_input *NewInput = &GameInput[0];
				game_input *OldInput = &GameInput[1];

				Win32GlobalRunning = true;

				u64 cycle_count_last = __rdtsc();
				LARGE_INTEGER tick_count_last;

				win32_game_code Game = win32_game_code_load(SourceGameDLLFullPath, TempGameDLLFullPath);
				u32 counter = 0;
				QueryPerformanceCounter(&tick_count_last);


				//
				// NOTE(Justin): Game loop.
				//

				f32 seconds_per_frame_actual = 0.0f;
				while(Win32GlobalRunning)
				{

					FILETIME NewDLLWriteTime = win32_file_last_write_time(SourceGameDLLFullPath);
					if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
					{
						win32_game_code_unload(&Game);
						Game = win32_game_code_load(SourceGameDLLFullPath, TempGameDLLFullPath);
					}



					game_controller_input *NewKeyboardController = &NewInput->Controller;
					game_controller_input *OldKeyboardController = &OldInput->Controller;
					game_controller_input ZeroController = {0}; 
					*NewKeyboardController = ZeroController;

					for(u32 button_index = 0;
							button_index < ARRAY_COUNT(NewKeyboardController->Buttons);
								button_index++)
					{
						NewKeyboardController->Buttons[button_index].ended_down =
							OldKeyboardController->Buttons[button_index].ended_down;
					}

					win32_process_pending_messgaes(NewKeyboardController);

					POINT MousePos;
					GetCursorPos(&MousePos);
					ScreenToClient(Window, &MousePos);

					NewInput->mouse_x = MousePos.x;
					NewInput->mouse_y = Win32GlobalBackBuffer.height - MousePos.y;

					win32_process_keyboard_messages(&NewInput->MouseButtons[0],
													GetKeyState(VK_LBUTTON) & (1 << 15));
					win32_process_keyboard_messages(&NewInput->MouseButtons[1],
													GetKeyState(VK_MBUTTON) & (1 << 15));
					win32_process_keyboard_messages(&NewInput->MouseButtons[2],
													GetKeyState(VK_RBUTTON) & (1 << 15));
					win32_process_keyboard_messages(&NewInput->MouseButtons[3],
													GetKeyState(VK_XBUTTON1) & (1 << 15));
					win32_process_keyboard_messages(&NewInput->MouseButtons[4],
													GetKeyState(VK_XBUTTON2) & (1 << 15));


					thread_context Thread = {};

					back_buffer BackBuffer = {};
					BackBuffer.memory = Win32GlobalBackBuffer.memory;
					BackBuffer.width = Win32GlobalBackBuffer.width;
					BackBuffer.height = Win32GlobalBackBuffer.height;
					BackBuffer.stride = Win32GlobalBackBuffer.stride;

					Game.UpdateAndRender(&Thread, &GameMemory, &BackBuffer, NewInput);

					DWORD play_cursor;
					DWORD write_cursor;
					DWORD target_cursor;
					DWORD byte_to_lock;
					DWORD bytes_to_write;
					b32 sound_is_valid = false;

					HRESULT get_current_position =
						Win32GlobalSecondaryBuffer->GetCurrentPosition(&play_cursor, &write_cursor);

					if(SUCCEEDED(get_current_position))
					{
						byte_to_lock =  ((Win32SoundBuffer.sample_count * Win32SoundBuffer.bytes_per_sample) % 
								Win32SoundBuffer.secondary_buffer_size);

						target_cursor = ((play_cursor + 
									(Win32SoundBuffer.bytes_per_sample * Win32SoundBuffer.sample_count_latency)) % 
								Win32SoundBuffer.secondary_buffer_size);

						if(byte_to_lock > target_cursor)
						{
							bytes_to_write = Win32SoundBuffer.secondary_buffer_size - byte_to_lock;
							bytes_to_write += target_cursor;
						}
						else
						{
							bytes_to_write = target_cursor - byte_to_lock;
						}
						sound_is_valid = true;
					}
					else
					{
						byte_to_lock = 0;
						bytes_to_write = 0;
					}

					sound_buffer SoundBuffer = {};
					SoundBuffer.samples_per_second = Win32SoundBuffer.samples_per_second;
					SoundBuffer.sample_count = bytes_to_write / Win32SoundBuffer.bytes_per_sample;
					SoundBuffer.samples = samples;
					Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);

					if(sound_is_valid)
					{
						win32_sound_buffer_fill(&Win32SoundBuffer, byte_to_lock, bytes_to_write, &SoundBuffer);
					}

					u64 cycle_count_end = __rdtsc();

					LARGE_INTEGER tick_count_end;
					QueryPerformanceCounter(&tick_count_end);


					s64 cycles_elapsed = cycle_count_end - cycle_count_last;
					s64 ticks_elapsed = tick_count_end.QuadPart - tick_count_last.QuadPart;

					seconds_per_frame_actual = (f32)ticks_elapsed / (f32)ticks_per_second;

					if(seconds_per_frame_actual < seconds_per_frame_target)
					{
						while(seconds_per_frame_actual < seconds_per_frame_target)
						{
							if(sleep_is_granular)
							{
								DWORD ms_to_sleep =
									(DWORD)(1000 *(seconds_per_frame_target - seconds_per_frame_actual));
								Sleep(ms_to_sleep);
							}
							LARGE_INTEGER tick_count_after_sleep;
							QueryPerformanceCounter(&tick_count_after_sleep);
							ticks_elapsed = tick_count_after_sleep.QuadPart - tick_count_last.QuadPart;
							seconds_per_frame_actual = (f32)ticks_elapsed / (f32)ticks_per_second;
						}
					}
					else
					{
						// Missed frame
					}

					win32_client_dimensions WindowDimensions = win32_get_window_dimension(Window);
					HDC DeviceContext = GetDC(Window);
					win32_display_buffer_to_window(&Win32GlobalBackBuffer, DeviceContext,
							WindowDimensions.width, WindowDimensions.height);
					ReleaseDC(Window, DeviceContext);

					f32 fps = (f32)ticks_per_second / (f32)ticks_elapsed;
					f32 miliseconds_elapsed = (((1000 * (f32)ticks_elapsed) * time_for_each_tick));
					f32 megacycles_per_frame = (f32)(cycles_elapsed / (1000.0f * 1000.0f));

					char textbuffer[256];
					sprintf_s(textbuffer, sizeof(textbuffer),
							"%0.2fms/f %0.2ff/s %0.2fMc/f\n", miliseconds_elapsed, fps, megacycles_per_frame);

					tick_count_last = tick_count_end;
					cycle_count_last = cycle_count_end;

					NewInput->dt_for_frame = seconds_per_frame_actual;
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;

				}
			}
			else
			{
			}
		}
		else
		{
		}
	}
	else
	{
	}

	return 0;
} 
