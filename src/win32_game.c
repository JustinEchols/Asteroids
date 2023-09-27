/*
TODO:
Audio
 - Play any audio
 - Play any audio file
	 - open wav
	 - decode
	 - uncompress
	 - pan audio?
	 - change pitch 

Basics of DSound
Setup DSound
	- Load dsound.dll							
	- Get function ptr to DirectSoundCreate
	- DirectSoundCreate
	- SetCooperativeLevel
	- SetFormat of the PrimaryBuffer
	- Create GlobaalSecondaryBuffer

Loading a WAV file into a DSound buffer
	- Read the header
	- If WAV file small create a secondary buffer large enought to hold sample
	- data. If WAV file large stream data to the buffer.
	- Fill buffer with data using Lock
	- If not streaming lock entire buffer otherwise
	- After copying unlock

Play DSound Buffer
Play


// 2-channels (stereo)
// stereo L/R channel two channels 8 bits per channel
// Block is a Sample which has two channels [L R]
// the member is in units of bytes so byte_count([L R]) = 2 channels * 16 bits per channel / 8 = 4 
// (# samples * sizeof(sample)) / seconds = bytes / second
sfx
*/


#include <windows.h>
#include <stdio.h>
#include <dsound.h>

#include <math.h>



#include "game.c"
#include "win32_game.h"



global_variable b32					Win32GlobalRunning;
global_variable win32_back_buffer	Win32GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER Win32GlobalSecondaryBuffer;
global_variable LARGE_INTEGER		tick_frequency;


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
win32_dsound_init(HWND WindowHandle, s32 secondary_buffer_size)
{
	HMODULE dsound_dll = LoadLibraryA("dsound.dll");
	if (dsound_dll) {
		direct_sound_create *dsound_create =
			(direct_sound_create *)GetProcAddress(dsound_dll, "DirectSoundCreate");

		LPDIRECTSOUND DSound;
		if (dsound_create) {
			if (SUCCEEDED(dsound_create(0, &DSound, 0))) {
				WAVEFORMATEX WaveFormat = {0};
				WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
				WaveFormat.nChannels = 2;
				WaveFormat.nSamplesPerSec = 44100;
				WaveFormat.wBitsPerSample = 16;	
				WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8; // 8 bits per byte
				WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;// #samples * bytes/sample
				WaveFormat.cbSize = 0;

				if (SUCCEEDED(DSound->lpVtbl->SetCooperativeLevel(DSound, WindowHandle, DSSCL_PRIORITY))) {
					DSBUFFERDESC BufferDesc;// = {0};
					BufferDesc.dwSize = sizeof(BufferDesc);
					BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
					BufferDesc.dwBufferBytes = 0;//required for primary buffer
					BufferDesc.dwReserved = 0;
					BufferDesc.lpwfxFormat = NULL;
					BufferDesc.guid3DAlgorithm = GUID_NULL;

					LPDIRECTSOUNDBUFFER PrimaryBuffer;
					if (SUCCEEDED(DSound->lpVtbl->CreateSoundBuffer(DSound, &BufferDesc, &PrimaryBuffer, 0))) {
						if (SUCCEEDED(PrimaryBuffer->lpVtbl->SetFormat(PrimaryBuffer, &WaveFormat))) {
							OutputDebugStringA("Primary buffer format set successfully.\n");
						} else {
							// TODO: Diagnostic
						}
					} else {
						// TODO: Diagnostic
					}
				} else {
					// TODO: Diagnostic
				}

				DSBUFFERDESC BufferDesc = {0};
				BufferDesc.dwSize = sizeof(BufferDesc);
				BufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS;
				BufferDesc.dwBufferBytes = secondary_buffer_size;//(4 * WaveFormat.nChannels) * WaveFormat.nSamplesPerSec;
				BufferDesc.dwReserved = 0;
				BufferDesc.lpwfxFormat = &WaveFormat;
				BufferDesc.guid3DAlgorithm = GUID_NULL;

				// NOTE(Justin): Using Win32GlobalSecondaryBuffer here!!
				if (SUCCEEDED(DSound->lpVtbl->CreateSoundBuffer(DSound, &BufferDesc, &Win32GlobalSecondaryBuffer, 0))) {
					OutputDebugStringA("Secondary buffer created successfully.\n");
				} else {
					// TODO: Diagnostic
				}
			} else {
				// TODO: Diagnostic
			}
		} else {
			// TODO: Diagnostic
		}
	} else {
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
		Win32GlobalSecondaryBuffer->lpVtbl->Lock(Win32GlobalSecondaryBuffer, 
												0,
												Win32SoundBuffer->secondary_buffer_size,
												&region_1, &region_1_size,
												&region_2, &region_2_size,
												DSBLOCK_ENTIREBUFFER);

	// If we are clearing the sound buffer can probably only clear in one loop
	// as region 1 should probably be the entire buffer. region_2 returns NULL
	// and the size is 0 so the second loop does not start...
	if (SUCCEEDED(secondary_buffer_lock)) {

		u8 *sample_byte = (u8 *)region_1;
		for (DWORD byte_index = 0; byte_index < region_1_size; byte_index++) {
			*sample_byte++ = 0;
		}

		sample_byte = (u8 *)region_2;
		for (DWORD byte_index = 0; byte_index < region_2_size; byte_index++) {
			*sample_byte++ = 0;
		}

		HRESULT secondary_buffer_unlock = Win32GlobalSecondaryBuffer->lpVtbl->Unlock(Win32GlobalSecondaryBuffer,
				region_1, region_1_size,
				region_2, region_2_size);

		if (SUCCEEDED(secondary_buffer_unlock)) {
			return;
		}
		else {
		}
	}
	else {
	}
}

internal void
win32_sound_buffer_fill(win32_sound_buffer *Win32SoundBuffer, DWORD byte_to_lock, DWORD bytes_to_write,
		sound_buffer *SoundBuffer)
{
	//NOTE: The SoundBuffer has the correct # samples b/c the sample_count of
	// the SoundBuffer is assigned the value of bytes_to_write / bytes_per_sample.
	// Since we also lock the sound buffer with bytes_to_write # of bytes, then
	//
	//	region_1_sample_count + region_2_sample_count = SoundBuffer.sample_count
	//
	// The access violation we had previously should be resolved. Since the
	// total number of samples we loop throught the win32 sound buffer and game
	// sound buffer is the same.
	
	VOID *region_1;
	DWORD region_1_size;
	VOID *region_2;
	DWORD region_2_size;

	HRESULT secondary_buffer_lock =
		Win32GlobalSecondaryBuffer->lpVtbl->Lock(Win32GlobalSecondaryBuffer,
												byte_to_lock,
												bytes_to_write,
												&region_1, &region_1_size,
												&region_2, &region_2_size,
												0);

	if (SUCCEEDED(secondary_buffer_lock)) {
		DWORD region_1_sample_count = region_1_size / Win32SoundBuffer->bytes_per_sample;

		s16 *sample_src = SoundBuffer->samples;
		s16 *sample_dest = (s16 *)region_1;
		for (DWORD sample_index = 0; sample_index < region_1_sample_count; sample_index++) {
			*sample_dest++ = *sample_src++;
			*sample_dest++ = *sample_src++;

			++Win32SoundBuffer->sample_count;
		}

		DWORD region_2_sample_count = region_2_size / Win32SoundBuffer->bytes_per_sample;
		sample_dest = (s16 *)region_2;
		for (DWORD sample_index = 0; sample_index < region_2_sample_count; sample_index++) {
			*sample_dest++ = *sample_src++;
			*sample_dest++ = *sample_src++;

			++Win32SoundBuffer->sample_count;
		}

		Win32GlobalSecondaryBuffer->lpVtbl->Unlock(Win32GlobalSecondaryBuffer,
				region_1, region_1_size,
				region_2, region_2_size);
	}
}

internal void
win32_back_buffer_resize(win32_back_buffer *Win32GlobalBackBuffer, int width, int height)
{
	if (Win32GlobalBackBuffer->memory) {
		VirtualFree(Win32GlobalBackBuffer->memory, 0, MEM_RELEASE);
	}

	Win32GlobalBackBuffer->width = width;
	Win32GlobalBackBuffer->height = height;
	Win32GlobalBackBuffer->bytes_per_pixel = 4;
	Win32GlobalBackBuffer->stride = Win32GlobalBackBuffer->width * Win32GlobalBackBuffer->bytes_per_pixel;

	Win32GlobalBackBuffer->Info.bmiHeader.biSize = sizeof(Win32GlobalBackBuffer->Info.bmiHeader); // Necessary?
	Win32GlobalBackBuffer->Info.bmiHeader.biWidth = Win32GlobalBackBuffer->width;
	Win32GlobalBackBuffer->Info.bmiHeader.biHeight = Win32GlobalBackBuffer->height;
 
	Win32GlobalBackBuffer->Info.bmiHeader.biPlanes = 1;
	Win32GlobalBackBuffer->Info.bmiHeader.biBitCount = 32;
	Win32GlobalBackBuffer->Info.bmiHeader.biCompression = BI_RGB;

	Win32GlobalBackBuffer->Info.bmiHeader.biSizeImage = 0;
	Win32GlobalBackBuffer->Info.bmiHeader.biXPelsPerMeter = 0;
	Win32GlobalBackBuffer->Info.bmiHeader.biYPelsPerMeter = 0;
	Win32GlobalBackBuffer->Info.bmiHeader.biClrUsed = 0;
	Win32GlobalBackBuffer->Info.bmiHeader.biClrImportant = 0;

	int bitmap_memory_size =  
		Win32GlobalBackBuffer->width * Win32GlobalBackBuffer->height * Win32GlobalBackBuffer->bytes_per_pixel;

	Win32GlobalBackBuffer->memory = VirtualAlloc((LPVOID)0, bitmap_memory_size, 
			MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

LRESULT CALLBACK
WndProc(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (Message) {
		case WM_CLOSE:
		{
			Win32GlobalRunning = FALSE;
		} break;
		case WM_DESTROY:
		{
			// TODO: Handle as an error. Why?
		} break;
		case WM_QUIT:
		{
		} break;
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(WindowHandle, &ClientRect);
			int client_width = ClientRect.right - ClientRect.left;
			int client_height = ClientRect.bottom - ClientRect.top;
			win32_back_buffer_resize(&Win32GlobalBackBuffer, client_width, client_height);
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			// TODO(Justin): Assert macro
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT PaintStruct;
			HDC DeviceContext = BeginPaint(WindowHandle, &PaintStruct);

			StretchDIBits(DeviceContext,
					0, 0, Win32GlobalBackBuffer.width, Win32GlobalBackBuffer.height,
					0, 0, Win32GlobalBackBuffer.width, Win32GlobalBackBuffer.height,
					Win32GlobalBackBuffer.memory, &Win32GlobalBackBuffer.Info,
					DIB_RGB_COLORS, SRCCOPY);

			EndPaint(WindowHandle, &PaintStruct);
		} break;
		default:
		{
			result = DefWindowProc(WindowHandle, Message, wParam, lParam);
		} break;
	}
	return(result);
}

internal void
win32_process_pending_messgaes(game_controller_input *KeyboardController)
{
	MSG Message; 
	while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
		switch(Message.message) {
			case WM_QUIT:
			{
				Win32GlobalRunning = FALSE;
			} break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				WPARAM vk_code = Message.wParam;
				s32 key_was_down = ((Message.lParam & (1 << 30)) != 0);
				s32 key_is_down = ((Message.lParam & (1 << 31)) == 0);
				if (key_was_down != key_is_down) {
					switch(vk_code) {
						case VK_LBUTTON:
						{
						} break;
						case VK_RBUTTON:
						{
						} break;
						case VK_LEFT:
						{
						} break;
						case VK_UP:
						{
						} break;
						case VK_DOWN:
						{
						} break;
						case VK_RIGHT:
						{
						} break;
						case VK_SPACE:
						{
							KeyboardController->Space.ended_down = key_is_down;
							KeyboardController->Space.half_transition_count++;
						} break;
						case 'W':
						{
							KeyboardController->Up.ended_down = key_is_down;
							KeyboardController->Up.half_transition_count++;
						} break;
						case 'A':
						{
							KeyboardController->Left.ended_down = key_is_down;
							KeyboardController->Left.half_transition_count++;
						} break;
						case 'S':
						{
							KeyboardController->Down.ended_down = key_is_down;
							KeyboardController->Down.half_transition_count++;
						} break;
						case 'D':
						{
							KeyboardController->Right.ended_down = key_is_down;
							KeyboardController->Right.half_transition_count++;
						} break;
						case VK_ESCAPE:
						{
							OutputDebugStringA("Escape");
							Win32GlobalRunning = FALSE;
						} break;
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



internal void
platform_file_free_memory(void *file_memory)
{
	if (file_memory) {
		VirtualFree(file_memory, 0, MEM_RELEASE);
	}
}

internal debug_file_read
platform_file_read_entire(char *filename)
{
	debug_file_read  Result = {0};
	HANDLE FileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE) {
		DWORD FileSize = GetFileSize(FileHandle, &FileSize);
		if (FileSize != 0) {
			Result.contents = VirtualAlloc(0, FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.contents) {
				DWORD BytesRead;
				ReadFile(FileHandle, Result.contents, FileSize, &BytesRead, 0);
				if (FileSize == BytesRead) {
					Result.size = FileSize;
				} else {

					platform_file_free_memory(Result.contents);
					Result.contents = 0;
				}
			} else {

			}
		} else {

		}
	} else {
		CloseHandle(FileHandle);
	}
	return(Result);
}



int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR CmdLine, int nCmdShow)
{
	const char window_class_name[]	= "TestWindowClass";
	const char window_title[]		= "Test";

	WNDCLASSEX WindowClass;
	HWND WindowHandle;


	WindowClass.cbSize			= sizeof(WindowClass);
	WindowClass.style			= CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc		= WndProc;
	WindowClass.cbClsExtra		= 0;
	WindowClass.cbWndExtra		= 0;
	WindowClass.hInstance		= hInstance;
	WindowClass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	WindowClass.hCursor			= LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground	= NULL;
	WindowClass.lpszMenuName	= NULL;
	WindowClass.lpszClassName	= window_class_name;
	WindowClass.hIconSm			= LoadIcon(NULL, IDI_APPLICATION);


	// f	= 10MHz,
	// 1/f	= 1e-7  = 0.1 µs
	QueryPerformanceFrequency(&tick_frequency);
	s64 ticks_per_second = tick_frequency.QuadPart;
	f32 time_for_each_tick = 1.0f / (f32)ticks_per_second;

	u32 min_miliseconds_to_sleep = 1;
	MMRESULT sleep_is_granular = timeBeginPeriod(min_miliseconds_to_sleep);

	int monitor_hz = 60;
	int game_hz = monitor_hz / 2;
	f32 seconds_per_frame_target = 1.0f / (f32)game_hz;

	if (RegisterClassEx(&WindowClass))  {
		WindowHandle = CreateWindowEx(0,
				window_class_name,
				window_title,
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				0, 0, hInstance, 0);

		if (WindowHandle) {
			HDC DeviceContext = GetDC(WindowHandle);

			win32_back_buffer_resize(&Win32GlobalBackBuffer, 960, 540);

			win32_sound_buffer Win32SoundBuffer = {0};

			Win32SoundBuffer.samples_per_second = 44100;
			Win32SoundBuffer.bytes_per_sample = sizeof(s16) * 2;
			Win32SoundBuffer.secondary_buffer_size = (Win32SoundBuffer.samples_per_second * Win32SoundBuffer.bytes_per_sample);
			Win32SoundBuffer.sample_count = 0;
			Win32SoundBuffer.wave_frequency = 256;
			Win32SoundBuffer.wave_amplitude = 6000;
			Win32SoundBuffer.samples_per_period = (Win32SoundBuffer.samples_per_second / Win32SoundBuffer.wave_frequency);
			Win32SoundBuffer.sample_count_latency = Win32SoundBuffer.samples_per_second / 15;//TODO more aggresive latency 1/30? 1/60?


			s16 *samples = (s16 *)VirtualAlloc((LPVOID)0, Win32SoundBuffer.secondary_buffer_size, 
			MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);


			game_memory GameMemory = {0};
			GameMemory.total_size = KILOBYTES(16);
			GameMemory.memory_block = VirtualAlloc((LPVOID)0, GameMemory.total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);


			if (GameMemory.memory_block && samples) {

				win32_dsound_init(WindowHandle, Win32SoundBuffer.secondary_buffer_size);

				win32_sound_buffer_clear(&Win32SoundBuffer);

				Win32GlobalSecondaryBuffer->lpVtbl->Play(Win32GlobalSecondaryBuffer, 0, 0, DSBPLAY_LOOPING);


				//ShowWindow(WindowHandle, nCmdShow);

				game_input GameInput[2] = {0};
				game_input *NewInput = &GameInput[0];
				game_input *OldInput = &GameInput[1];

				Win32GlobalRunning = TRUE;

				u64 cycle_count_last = __rdtsc();
				LARGE_INTEGER tick_count_last;

				QueryPerformanceCounter(&tick_count_last);

				//
				// NOTE(Justin): Game loop.
				//
				f32 seconds_per_frame_actual = 0.0f;
				while (Win32GlobalRunning) {

					game_controller_input *NewKeyboardController = &NewInput->Controller;
					game_controller_input *OldKeyboardController = &OldInput->Controller;
					game_controller_input ZeroController = {0}; 
					*NewKeyboardController = ZeroController;
					for (u32 button_index = 0;
							button_index < ARRAY_COUNT(NewKeyboardController->Buttons);
							button_index++) {
						NewKeyboardController->Buttons[button_index].ended_down =
							OldKeyboardController->Buttons[button_index].ended_down;
					}

					win32_process_pending_messgaes(NewKeyboardController);


					DWORD play_cursor;
					DWORD write_cursor;
					DWORD target_cursor;
					DWORD byte_to_lock;
					DWORD bytes_to_write;
					b32 sound_is_valid = FALSE;

					HRESULT get_current_position =
						Win32GlobalSecondaryBuffer->lpVtbl->GetCurrentPosition(Win32GlobalSecondaryBuffer, 
								&play_cursor, &write_cursor);

					if (SUCCEEDED(get_current_position)) {
						byte_to_lock =  ((Win32SoundBuffer.sample_count * Win32SoundBuffer.bytes_per_sample) % 
								Win32SoundBuffer.secondary_buffer_size);

						target_cursor = ((play_cursor + 
									(Win32SoundBuffer.bytes_per_sample * Win32SoundBuffer.sample_count_latency)) % 
								Win32SoundBuffer.secondary_buffer_size);

						if (byte_to_lock > target_cursor) {
							bytes_to_write = Win32SoundBuffer.secondary_buffer_size - byte_to_lock;
							bytes_to_write += target_cursor;
						} else {
							bytes_to_write = target_cursor - byte_to_lock;
						}
						sound_is_valid = TRUE;
					} else {
						byte_to_lock = 0;
						bytes_to_write = 0;
					}

					sound_buffer SoundBuffer = {0};
					SoundBuffer.samples_per_second = Win32SoundBuffer.samples_per_second;
					SoundBuffer.sample_count = bytes_to_write / Win32SoundBuffer.bytes_per_sample;
					SoundBuffer.samples = samples;

					back_buffer BackBuffer = {NULL, 0, 0, 0};
					BackBuffer.memory = Win32GlobalBackBuffer.memory;
					BackBuffer.width = Win32GlobalBackBuffer.width;
					BackBuffer.height = Win32GlobalBackBuffer.height;
					BackBuffer.bytes_per_pixel = Win32GlobalBackBuffer.bytes_per_pixel;
					BackBuffer.stride = Win32GlobalBackBuffer.stride;

					update_and_render(&GameMemory, &BackBuffer, &SoundBuffer, NewInput);

					if (sound_is_valid) {
						win32_sound_buffer_fill(&Win32SoundBuffer, byte_to_lock, bytes_to_write, &SoundBuffer);
					}

					u64 cycle_count_end = __rdtsc();

					LARGE_INTEGER tick_count_end;
					QueryPerformanceCounter(&tick_count_end);


					s64 cycles_elapsed = cycle_count_end - cycle_count_last;
					s64 ticks_elapsed = tick_count_end.QuadPart - tick_count_last.QuadPart;

					seconds_per_frame_actual = (f32)ticks_elapsed / (f32)ticks_per_second;

					if (seconds_per_frame_actual < seconds_per_frame_target) {
						while (seconds_per_frame_actual < seconds_per_frame_target) {
							if (sleep_is_granular) {
								DWORD ms_to_sleep =
									(DWORD)(1000 *(seconds_per_frame_target - seconds_per_frame_actual));
								Sleep(ms_to_sleep);
							}
							LARGE_INTEGER tick_count_after_sleep;
							QueryPerformanceCounter(&tick_count_after_sleep);
							ticks_elapsed = tick_count_after_sleep.QuadPart - tick_count_last.QuadPart;
							seconds_per_frame_actual = (f32)ticks_elapsed / (f32)ticks_per_second;
						}
					} else {
						// Missed frame
					}

					StretchDIBits(DeviceContext,
							0, 0, Win32GlobalBackBuffer.width, Win32GlobalBackBuffer.height,
							0, 0, Win32GlobalBackBuffer.width, Win32GlobalBackBuffer.height,
							Win32GlobalBackBuffer.memory, 
							&Win32GlobalBackBuffer.Info,
							DIB_RGB_COLORS, SRCCOPY);

					f32 fps = (f32)ticks_per_second / (f32)ticks_elapsed;
					f32 miliseconds_elapsed = (((1000 * (f32)ticks_elapsed) * time_for_each_tick));
					f32 megacycles_per_frame = (f32)(cycles_elapsed / (1000.0f * 1000.0f));

					char textbuffer[256];
					sprintf_s(textbuffer, sizeof(textbuffer), "%0.2fms/f %0.2ff/s %0.2fMc/f\n", miliseconds_elapsed, fps, megacycles_per_frame);
					//OutputDebugStringA(textbuffer);

					tick_count_last = tick_count_end;
					cycle_count_last = cycle_count_end;

					NewInput->dt_for_frame = seconds_per_frame_actual;
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
				}
			}
		} else {

		}
	} else {
	}

	return 0;
} 
