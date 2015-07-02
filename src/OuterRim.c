#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static Layer *window_layer;

#define FACE_BACKGROUND_COLOUR GColorBlack
#define CIRCLE_COLOUR GColorWhite
#define OUTER_HAND_COLOUR GColorWhite
#define INNER_CIRCLE_COLOUR GColorBrilliantRose
#define INNER_HAND_COLOUR GColorBrilliantRose
#define INNER_DOT_COLOUR GColorDarkGray

/*
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

void tap(AccelAxisType axis, int32_t direction) {
//static void tap(AccelAxisType axis, int direction) {
  text_layer_set_text(text_layer, "Tap");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);

  accel_tap_service_subscribe(tap);
}
*/

static void window_update_proc(Layer *l, GContext *ctx) {
	time_t now = time(NULL);
	struct tm t = *localtime(&now);

	GRect f = layer_get_bounds(l);
	grect_standardize(&f);
	GPoint centre = grect_center_point(&f);
	int32_t circle_radius = (f.size.w / 2) - 2;
	int32_t inner_radius = circle_radius / 5;
	int32_t inner_dot = inner_radius / 3;

	GPoint outer_hand_start;
	GPoint outer_hand_end;
	int32_t outer_hand_length = circle_radius / 2;
	GPoint inner_hand_end;
	int32_t inner_hand_length = circle_radius / 2 + inner_radius;

	int32_t angle = TRIG_MAX_ANGLE * t.tm_min / 60;
	outer_hand_start.y = (-cos_lookup(angle) * circle_radius / TRIG_MAX_RATIO) + centre.y;
	outer_hand_start.x = (sin_lookup(angle) * circle_radius / TRIG_MAX_RATIO) + centre.x;
	outer_hand_end.y = (-cos_lookup(angle) * (circle_radius - outer_hand_length) / TRIG_MAX_RATIO) + centre.y;
	outer_hand_end.x = (sin_lookup(angle) * (circle_radius - outer_hand_length) / TRIG_MAX_RATIO) + centre.x;

	angle = TRIG_MAX_ANGLE * (t.tm_hour % 12) / 12;
	// add a little bit because we are likely partially through the minute
	angle += TRIG_MAX_ANGLE * (t.tm_min / 60);
	inner_hand_end.y = (-cos_lookup(angle) * inner_hand_length / TRIG_MAX_RATIO) + centre.y;
	inner_hand_end.x = (sin_lookup(angle) * inner_hand_length / TRIG_MAX_RATIO) + centre.x;

	graphics_context_set_stroke_width(ctx, 3);

	graphics_context_set_fill_color(ctx, FACE_BACKGROUND_COLOUR);
	graphics_fill_rect(ctx, layer_get_bounds(l), 0, GCornerNone);

	graphics_context_set_stroke_color(ctx, CIRCLE_COLOUR);
	graphics_draw_circle(ctx, centre, circle_radius);

	graphics_context_set_stroke_color(ctx, OUTER_HAND_COLOUR);
	graphics_draw_line(ctx, outer_hand_start, outer_hand_end);

	graphics_context_set_fill_color(ctx, INNER_CIRCLE_COLOUR);
	graphics_fill_circle(ctx, centre, inner_radius);

	graphics_context_set_stroke_color(ctx, INNER_HAND_COLOUR);
	graphics_draw_line(ctx, centre, inner_hand_end);

	graphics_context_set_fill_color(ctx, INNER_DOT_COLOUR);
	graphics_fill_circle(ctx, centre, inner_dot);
}

static void window_load(Window *window) {
  window_layer = window_get_root_layer(window);

  layer_set_update_proc(window_layer, window_update_proc);
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(window_layer);
}

static void init(void) {
  window = window_create();
//  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
