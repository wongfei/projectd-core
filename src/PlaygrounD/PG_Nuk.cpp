#include "PlaygrounD.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION

#pragma warning(push, 3)
#pragma warning(disable: 4996) // _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4701) // potentially uninitialized local variable
#include "Nuklear/nuklear.h"
#include "Nuklear/nuklear_sdl_gl2.h"
#pragma warning(pop)

#include "Nuklear/style.h"

namespace D {

void PlaygrounD::initNuk()
{
	nukContext_ = nk_sdl_init(appWindow_);

	struct nk_font_atlas *atlas = nullptr;
    nk_sdl_font_stash_begin(&atlas);
	struct nk_font *font = nk_font_atlas_add_from_file(atlas, stra((appDir_ + L"content/demo/DroidSans.ttf")).c_str(), 14, 0);
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_sdl_font_stash_end();

    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    nk_style_set_font(nukContext_, &font->handle);

	set_style(nukContext_, THEME_DARK);
}

void PlaygrounD::shutNuk()
{
	if (nukContext_)
	{
		nk_sdl_shutdown();
		nukContext_ = nullptr;
	}
}

void PlaygrounD::beginInputNuk()
{
	nk_input_begin(nukContext_);
}

void PlaygrounD::processEventNuk(SDL_Event* ev)
{
	nk_sdl_handle_event(ev);
}

void PlaygrounD::endInputNuk()
{
	nk_input_end(nukContext_);
}

void PlaygrounD::renderNuk()
{
	auto* ctx = nukContext_;

	struct nk_colorf bg;
	bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

	float w = 300;
	float h = 550;

	if (nk_begin(ctx, "Debug", nk_rect((float)width_ - w - 10, 100.0f, w, h),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
		NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		float rowh = 20;
		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_checkbox_label(ctx, "wireframe", &wireframeMode_);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_checkbox_label(ctx, "draw world", &drawWorld_);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_checkbox_label(ctx, "draw sky", &drawSky_);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_checkbox_label(ctx, "draw track points", &drawTrackPoints_);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_checkbox_label(ctx, "draw nearby points", &drawNearbyPoints_);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_checkbox_label(ctx, "draw car probes", &drawCarProbes_);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_property_int(ctx, "sim HZ", 30, &simHz_, 1000, 1, 2);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_property_int(ctx, "draw HZ", 30, &drawHz_, 1000, 1, 2);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_property_float(ctx, "mouse sens", 0.01f, &mouseSens_, 3.0f, 0.01f, 0.01f);

		nk_layout_row_dynamic(ctx, rowh, 1);
		nk_property_float(ctx, "camera speed", 0.01f, &moveSpeed_, 1000.0f, 5.0f, 5.0f);

		if (car_)
		{
			if (nk_tree_push(ctx, NK_TREE_TAB, "Agent", NK_MAXIMIZED)) {

			const float maxw = 5.0f;
			const float stepw = 0.01f;
			const float pixelw = 0.02f;

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_checkbox_label(ctx, "hit teleport", &car_->teleportOnCollision);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_checkbox_label(ctx, "badloc teleport", &car_->teleportOnBadLocation);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "drift W", 0.0f, &car_->scoreDriftW, maxw, stepw, pixelw);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "rpm W", 0.0f, &car_->scoreRpmW, maxw, stepw, pixelw);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "speed W", 0.0f, &car_->scoreSpeedW, maxw, stepw, pixelw);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "gear W", 0.0f, &car_->scoreGearW, maxw, stepw, pixelw);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "grind W", 0.0f, &car_->scoreGearGrindW, maxw, stepw, pixelw);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "dir W", 0.0f, &car_->scoreWrongDirW, maxw, stepw, pixelw);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "probe W", 0.0f, &car_->scoreProbeW, maxw, stepw, pixelw);

			nk_layout_row_dynamic(ctx, rowh, 1);
			nk_property_float(ctx, "hit W", 0.0f, &car_->scoreCollisionW, maxw, stepw, pixelw);

			nk_tree_pop(ctx); }
		}
	}
	nk_end(ctx);

	#if 0
	if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
		NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		enum { EASY, HARD };
		static int op = EASY;
		static int property = 20;

		nk_layout_row_static(ctx, 30, 80, 1);
		if (nk_button_label(ctx, "button"))
			fprintf(stdout, "button pressed\n");
		nk_layout_row_dynamic(ctx, 30, 2);
		if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
		if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

		nk_layout_row_dynamic(ctx, 20, 1);
		nk_label(ctx, "background:", NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 25, 1);
		if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400))) {
			nk_layout_row_dynamic(ctx, 120, 1);
			bg = nk_color_picker(ctx, bg, NK_RGBA);
			nk_layout_row_dynamic(ctx, 25, 1);
			bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
			bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
			bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
			bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
			nk_combo_end(ctx);
		}
	}
	nk_end(ctx);
	#endif

	nk_sdl_render(NK_ANTI_ALIASING_ON);
}

}
