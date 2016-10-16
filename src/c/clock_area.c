#include <pebble.h>
#include "clock_area.h"
#include "settings.h"

#define ROUND_VERTICAL_PADDING 15

static int tm_min;
static int tm_hour;

Layer* clock_area_layer;

GRect screen_rect;

/**
 * Draw a line with a given width.
 */
void graphics_draw_line_with_width(GContext *ctx, GPoint p0, GPoint p1, uint8_t width) {
    graphics_context_set_stroke_width(ctx, width);
    graphics_draw_line(ctx, p0, p1);
}

/**
 * Returns a point on the line from the center away at an angle specified by tick/maxtick, at a specified distance
 */
GPoint get_radial_point(const GPoint center, const int16_t distance_from_center, const int32_t angle) {
    GPoint result = {
            .x = (int16_t) (sin_lookup(angle) * (int32_t) distance_from_center / TRIG_MAX_RATIO) + center.x,
            .y = (int16_t) (-cos_lookup(angle) * (int32_t) distance_from_center / TRIG_MAX_RATIO) + center.y,
    };
    return result;
}

void update_clock_area_layer(Layer *l, GContext* ctx) {
  // check layer bounds
  GRect bounds = layer_get_unobstructed_bounds(l);

  #ifdef PBL_ROUND
    bounds = GRect(0, ROUND_VERTICAL_PADDING, screen_rect.size.w, screen_rect.size.h - ROUND_VERTICAL_PADDING * 2);
  #endif
  
      // hour ticks
    uint8_t tick_width = 2;
    int16_t radius = (bounds.size.w-31) / 2;
    GPoint center = grect_center_point(&bounds);

    // avenir + avenir bold metrics
    //int v_padding = bounds.size.h / 16;
    int h_adjust = 0;
    //int v_adjust = 0;

    // if it's a round watch, EVERYTHING CHANGES
    #ifdef PBL_ROUND
      //v_adjust = ROUND_VERTICAL_PADDING;
    #else
      // for rectangular watches, adjust X position based on sidebar position 
      if(globalSettings.sidebarOnLeft) {
        h_adjust += 15;
      } else {
        h_adjust -= 16;
      }
    #endif
    center.x += h_adjust;

    graphics_context_set_stroke_color(ctx, globalSettings.timeColor);
    for (int i = 0; i < 12; ++i) 
    {
        int32_t angle = i * TRIG_MAX_ANGLE / 12;
        int tick_length = PBL_IF_ROUND_ELSE(8, 6);

        graphics_draw_line_with_width(ctx, get_radial_point(center, radius + PBL_IF_ROUND_ELSE(3, 0), angle),
                                      get_radial_point(center, radius - tick_length, angle),
                                      tick_width);
    }

    // only relevant minute ticks
    int start_min_tick = (tm_min / 5) * 5;
    for (int i = start_min_tick; i < start_min_tick + 5; ++i) 
    {
        int32_t angle = i * TRIG_MAX_ANGLE / 60;
        graphics_draw_line_with_width(ctx, get_radial_point(center, radius + PBL_IF_ROUND_ELSE(3, 0), angle),
                                      get_radial_point(center, radius - PBL_IF_ROUND_ELSE(5, 3), angle), 1);
    }

        // compute angles
    int32_t minute_angle = tm_min * TRIG_MAX_ANGLE / 60;
    GPoint minute_hand = get_radial_point(center, radius - PBL_IF_ROUND_ELSE(16, 10), minute_angle);
    int hour_tick = ((tm_hour % 12) * 6) + (tm_min / 10);
    int32_t hour_angle = hour_tick * TRIG_MAX_ANGLE / (12 * 6);
    GPoint hour_hand = get_radial_point(center, radius * 55 / 100, hour_angle);

        // minute hand
    graphics_context_set_stroke_color(ctx, globalSettings.timeColor);
    graphics_draw_line_with_width(ctx, minute_hand, center, 4);
    graphics_context_set_stroke_color(ctx, globalSettings.timeBgColor);
    graphics_draw_line_with_width(ctx, get_radial_point(center, radius - PBL_IF_ROUND_ELSE(20, 12), minute_angle), center, 1);

    // hour hand
    graphics_context_set_stroke_color(ctx, globalSettings.timeColor);
    graphics_draw_line_with_width(ctx, hour_hand, center, 4);
    graphics_context_set_stroke_color(ctx, globalSettings.timeBgColor);
    graphics_draw_line_with_width(ctx, get_radial_point(center, radius * 55 / 100 - 2, hour_angle), center, 1);

    // dot in the middle
    graphics_context_set_fill_color(ctx, globalSettings.timeColor);
    graphics_fill_circle(ctx, center, 5);
}


void ClockArea_init(Window* window) {
  // record the screen size, since we NEVER GET IT AGAIN
  screen_rect = layer_get_bounds(window_get_root_layer(window));

  GRect bounds;
  bounds = GRect(0, 0, screen_rect.size.w, screen_rect.size.h);

  // init the clock area layer
  clock_area_layer = layer_create(bounds);
  layer_add_child(window_get_root_layer(window), clock_area_layer);
  layer_set_update_proc(clock_area_layer, update_clock_area_layer);

}

void ClockArea_deinit() {
  layer_destroy(clock_area_layer);
}

void ClockArea_redraw() {
  layer_mark_dirty(clock_area_layer);  
}

void ClockArea_update_time(struct tm* time_info) {

  tm_min = time_info->tm_min;
  tm_hour = time_info->tm_hour;
  ClockArea_redraw();
}