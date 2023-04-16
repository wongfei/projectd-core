#pragma once

static void initSDL()
{
	int rc = SDL_Init(SDL_INIT_VIDEO);
	GUARD_FATAL(rc == 0);

	SDL_DisplayMode mode = {};
	SDL_GetDesktopDisplayMode(0, &mode);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	unsigned int winFlags = SDL_WINDOW_OPENGL;

	if (fullscreen_)
		winFlags |= (SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_HIDDEN); // hidden to prevent flicker
	else
		winFlags |= (SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

	if (fullscreen_)
	{
		posx_ = posy_ = 0;
	}

	if (!width_ || !height_)
	{
		if (fullscreen_)
		{
			width_ = mode.w;
			height_ = mode.h;
		}
		else
		{
			winFlags |= SDL_WINDOW_MAXIMIZED;
			width_ = mode.w / 2;
			height_ = mode.h / 2;
			posx_ = (mode.w / 2) - width_ / 2;
			posy_ = (mode.h / 2) - height_ / 2;
		}
	}

	appWindow_ = SDL_CreateWindow("DEMO", posx_, posy_, width_, height_, winFlags);
	glContext_ = SDL_GL_CreateContext(appWindow_);
	SDL_GL_SetSwapInterval(swapInterval_); // 0 immediate, 1 vsync, -1 adaptive

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(appWindow_, &wmInfo);
	sysWindow_ = wmInfo.info.win.window;
	log_printf(L"hwnd=%p", sysWindow_);
}

static void shutSDL()
{
	SDL_GL_DeleteContext(glContext_);
	SDL_DestroyWindow(appWindow_);
	//SDL_Quit();
}

static void clearViewport()
{
	SDL_GetWindowSize(appWindow_, &width_, &height_);
	glViewport(0, 0, width_, height_);
	glClearColor(0, 0, 0, 1);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void begin3d()
{
	glMatrixMode(GL_PROJECTION);
	glm::mat4 persp = glm::perspectiveFov(glm::radians(fov_), (float)width_, (float)height_, 0.2f, 20000.0f);
	glLoadMatrixf(&persp[0][0]);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(&camView_[0][0]);
}

static void begin2d()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width_, height_, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -1.0f);
}

static void swap()
{
	glFlush();
	SDL_GL_SwapWindow(appWindow_);
}

static void processEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				if (event.button.button == SDL_BUTTON_LEFT) inpMouseBtn_[0] = event.button.state;
				if (event.button.button == SDL_BUTTON_RIGHT) inpMouseBtn_[1] = event.button.state;

				if (!fullscreen_ && event.button.button == SDL_BUTTON_RIGHT)
				{
					SDL_SetRelativeMouseMode((event.button.state == SDL_PRESSED) ? SDL_TRUE : SDL_FALSE);
				}

				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (inpMouseBtn_[1])
				{
					camYpr_[0] += event.motion.xrel * inpMouseSens_;
					camYpr_[1] -= event.motion.yrel * inpMouseSens_;
				}
				break;
			}

			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				const auto scan = event.key.keysym.scancode;
				const auto sym = event.key.keysym.sym;
				const auto state = event.key.state;

				if (scan >= 0 && scan < SDL_NUM_SCANCODES)
				{
					keys_[scan] = state;
				}

				break;
			}

			case SDL_WINDOWEVENT:
			{
				//width_ = event.window.data1;
				//height_ = event.window.data2;
				break;
			}

			case SDL_QUIT:
			{
				exitFlag_ = true;
				break;
			}
		}
	}
}

inline bool getkey(SDL_Scancode scan) { return keys_[scan]; }
inline bool getkeyf(SDL_Scancode scan) { return keys_[scan] ? 1.0f : 0.0f; }

inline bool asyncKeydown(SDL_Scancode scan) { auto k = keys_[scan]; auto a = asynckeys_[scan]; asynckeys_[scan] = k; return (k && k != a); }
inline bool asyncKeyup(SDL_Scancode scan) { auto k = keys_[scan]; auto a = asynckeys_[scan]; asynckeys_[scan] = k; return (!k && k != a); }
