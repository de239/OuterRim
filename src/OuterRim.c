#include <pebble.h>

static Window *window;
static Layer *face_layer;

static bool display_second_hand;

static GPoint centre;
static int32_t circle_radius;
static int32_t second_hand_radius;
static int32_t inner_radius;
static int32_t inner_dot;
static int32_t outer_hand_length;
static int32_t inner_hand_length;

#define FACE_BACKGROUND_COLOUR GColorBlack
#define CIRCLE_COLOUR GColorWhite
#define SECOND_HAND_CIRCLE_COLOUR GColorRed
#define OUTER_HAND_COLOUR GColorWhite
#define SECOND_HAND_COLOUR GColorBlack
#define INNER_CIRCLE_COLOUR GColorBrilliantRose
#define INNER_HAND_COLOUR GColorBrilliantRose
#define INNER_DOT_COLOUR GColorDarkGray

#define SECOND_HAND_DURATION 3000

static void face_update_proc(Layer *l, GContext *ctx) {
	time_t now = time(NULL);
	struct tm t = *localtime(&now);
	GPoint outer_hand_start;
	GPoint outer_hand_end;
	GPoint inner_hand_end;
	GPoint second_hand_end;

	float min_angle = TRIG_MAX_ANGLE * t.tm_min / 60;
	outer_hand_start.y = (-cos_lookup(min_angle) * circle_radius / TRIG_MAX_RATIO) + centre.y;
	outer_hand_start.x = (sin_lookup(min_angle) * circle_radius / TRIG_MAX_RATIO) + centre.x;
	outer_hand_end.y = (-cos_lookup(min_angle) * (circle_radius - outer_hand_length) / TRIG_MAX_RATIO) + centre.y;
	outer_hand_end.x = (sin_lookup(min_angle) * (circle_radius - outer_hand_length) / TRIG_MAX_RATIO) + centre.x;

	float hour_angle = TRIG_MAX_ANGLE * (t.tm_hour % 12) / 12;
	// add a little bit because we are likely partially through the hour
	// XXX should look at the generated code to see if the compiler is smart enough
	// to DTRT
	hour_angle += (min_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);
	inner_hand_end.y = (-cos_lookup(hour_angle) * inner_hand_length / TRIG_MAX_RATIO) + centre.y;
	inner_hand_end.x = (sin_lookup(hour_angle) * inner_hand_length / TRIG_MAX_RATIO) + centre.x;

	graphics_context_set_fill_color(ctx, FACE_BACKGROUND_COLOUR);
	graphics_fill_rect(ctx, layer_get_bounds(l), 0, GCornerNone);

	graphics_context_set_stroke_width(ctx, 1);
	graphics_context_set_stroke_color(ctx, CIRCLE_COLOUR);
	graphics_draw_circle(ctx, centre, circle_radius);

	graphics_context_set_stroke_width(ctx, 3);

	if(display_second_hand) {
		graphics_context_set_fill_color(ctx, SECOND_HAND_CIRCLE_COLOUR);
		graphics_fill_circle(ctx, centre, second_hand_radius);

		float angle = TRIG_MAX_ANGLE * t.tm_sec / 60;
		second_hand_end.y = (-cos_lookup(angle) * second_hand_radius / TRIG_MAX_RATIO) + centre.y;
		second_hand_end.x = (sin_lookup(angle) * second_hand_radius / TRIG_MAX_RATIO) + centre.x;

		graphics_context_set_stroke_color(ctx, SECOND_HAND_COLOUR);
		graphics_draw_line(ctx, centre, second_hand_end);
	}

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
	Layer *window_layer = window_get_root_layer(window);
	face_layer = layer_create(layer_get_frame(window_layer));
	layer_add_child(window_layer, face_layer);

	GRect f = layer_get_bounds(face_layer);
	grect_standardize(&f);
	centre = grect_center_point(&f);
	circle_radius = (f.size.w / 2) - 2;
	second_hand_radius = circle_radius * 6 / 10;
	inner_radius = circle_radius / 5;
	inner_dot = inner_radius / 3;

	outer_hand_length = circle_radius / 2;
	inner_hand_length = circle_radius / 2 + inner_radius;

	display_second_hand = false;

	layer_set_update_proc(face_layer, face_update_proc);
}

static void window_unload(Window *window) {
	layer_destroy(face_layer);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(face_layer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(face_layer);
}

static void remove_second_hand(void *data) {
	display_second_hand = false;
	tick_timer_service_unsubscribe();
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
	layer_mark_dirty(face_layer);
}

static void tap(AccelAxisType axis, int32_t direction) {
	display_second_hand = true;
	layer_mark_dirty(face_layer);

	tick_timer_service_unsubscribe();
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

	app_timer_register(SECOND_HAND_DURATION, remove_second_hand, NULL);
}

static void init(void) {
  window = window_create();
  accel_tap_service_subscribe(tap);
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

  app_event_loop();

  deinit();
}
