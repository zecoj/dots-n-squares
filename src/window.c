#include <pebble.h>

#define BATT_BAR_WDTH 146

static Window *window;
static GFont font_date;
static GFont font_time;
static GBitmap *bmp_batt_bar;
static GBitmap *bmp_time_bg;
static GBitmap *bmp_date_fg;
static BitmapLayer *layer_batt_bar;
static BitmapLayer *layer_time_fg;
static BitmapLayer *layer_date_bg;
static TextLayer *layer_time_digit_1;
static TextLayer *layer_time_digit_2;
static TextLayer *layer_time_digit_3;
static TextLayer *layer_time_digit_4;
static TextLayer *layer_time_digit_5;
static TextLayer *layer_date;
static Layer *layer;
BitmapLayer *bluetooth_layer;
bool bt_connect_toggle;

void bluetooth_connection_handler(bool connected) {
  if (!bt_connect_toggle && connected) {
    bt_connect_toggle = true;
    vibes_short_pulse();
    layer_set_hidden(bitmap_layer_get_layer(layer_date_bg), true);
  }
  if (bt_connect_toggle && !connected) {
    bt_connect_toggle = false;
    vibes_long_pulse();
    layer_set_hidden(bitmap_layer_get_layer(layer_date_bg), false);
  }
}

void uplayer_date(Layer *me, GContext* ctx) 
{
  BatteryChargeState charge_state =  battery_state_service_peek();
  static char mydate[15];
  static char mytime[5];
  static char time_digits[4][2] = {" ", " ", " ", " "};

  //get tick_time
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  //get weekday
  strftime(mydate, sizeof(mydate), "%a %d", tick_time);
  text_layer_set_text(layer_date, mydate);

  if(clock_is_24h_style() == true) { 
      strftime(mytime, sizeof(mytime), "%H%M", tick_time);
  } else {
      strftime(mytime, sizeof(mytime), "%I%M", tick_time);
  }

  time_digits[0][0] = mytime[3];
  time_digits[1][0] = mytime[2];
  time_digits[2][0] = mytime[1];
  time_digits[3][0] = mytime[0];

  text_layer_set_text(layer_time_digit_1, time_digits[3]);
  text_layer_set_text(layer_time_digit_2, time_digits[2]);
  text_layer_set_text(layer_time_digit_4, time_digits[1]);
  text_layer_set_text(layer_time_digit_5, time_digits[0]);

  graphics_context_set_stroke_color(ctx,GColorWhite);
  graphics_context_set_fill_color(ctx,GColorWhite);
  graphics_fill_rect(ctx, GRect((144-BATT_BAR_WDTH)/2,100,(BATT_BAR_WDTH*charge_state.charge_percent/100),5), 0, GCornerNone);
}

void tick(struct tm *tick_time, TimeUnits units_changed)
{
  layer_mark_dirty(layer);
}


static void initialise_ui(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, true);

  bmp_batt_bar = gbitmap_create_with_resource(RESOURCE_ID_BATT_BAR);
  bmp_time_bg = gbitmap_create_with_resource(RESOURCE_ID_DIGITS_BACKGROUND);
  bmp_date_fg = gbitmap_create_with_resource(RESOURCE_ID_DATE_FOREGROUND_BLACK);
  font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_IMAGINE_20));
  font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_IMAGINE_48));

  // layer_batt_bar
  layer_batt_bar = bitmap_layer_create(GRect((144-BATT_BAR_WDTH)/2, 100, BATT_BAR_WDTH, 5));
  layer_time_fg = bitmap_layer_create(GRect(0,61,144,34));
  layer_date_bg = bitmap_layer_create(GRect(1,105,144,20));
  bitmap_layer_set_bitmap(layer_batt_bar, bmp_batt_bar);
  bitmap_layer_set_bitmap(layer_time_fg, bmp_time_bg);
  bitmap_layer_set_bitmap(layer_date_bg, bmp_date_fg);
  bitmap_layer_set_background_color(layer_date_bg, GColorClear);
  bitmap_layer_set_compositing_mode(layer_date_bg, GCompOpAnd);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_batt_bar);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_time_fg);
  layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(layer, uplayer_date);
  layer_add_child(window_get_root_layer(window), layer);  
  
  // layer_date
  layer_date = text_layer_create(GRect(2, 103, 145, 24));
  text_layer_set_background_color(layer_date, GColorClear);
  text_layer_set_text_color(layer_date, GColorWhite);
  text_layer_set_text_alignment(layer_date, GTextAlignmentRight);
  text_layer_set_font(layer_date, font_date);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_date);
  
  layer_add_child(window_get_root_layer(window), (Layer *)layer_date_bg);
  layer_set_hidden(bitmap_layer_get_layer(layer_date_bg), true);
  
  // layer_time_digit_1
  layer_time_digit_1 = text_layer_create(GRect(0, 45, 36, 50));
  text_layer_set_background_color(layer_time_digit_1, GColorClear);
  text_layer_set_text_color(layer_time_digit_1, GColorWhite);
  text_layer_set_text_alignment(layer_time_digit_1, GTextAlignmentRight);
  text_layer_set_font(layer_time_digit_1, font_time);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_time_digit_1);
  // layer_time_digit_2
  layer_time_digit_2 = text_layer_create(GRect(36, 45, 36, 50));
  text_layer_set_background_color(layer_time_digit_2, GColorClear);
  text_layer_set_text_color(layer_time_digit_2, GColorWhite);
  text_layer_set_text_alignment(layer_time_digit_2, GTextAlignmentRight);
  text_layer_set_font(layer_time_digit_2, font_time);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_time_digit_2);
  // layer_time_digit_3
  layer_time_digit_3 = text_layer_create(GRect(69, 39, 36, 50));
  text_layer_set_background_color(layer_time_digit_3, GColorClear);
  text_layer_set_text_color(layer_time_digit_3, GColorWhite);
  text_layer_set_text(layer_time_digit_3, ":");
  text_layer_set_font(layer_time_digit_3, font_time);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_time_digit_3);
  // layer_time_digit_4
  layer_time_digit_4 = text_layer_create(GRect(78, 45, 36, 50));
  text_layer_set_background_color(layer_time_digit_4, GColorClear);
  text_layer_set_text_color(layer_time_digit_4, GColorWhite);
  text_layer_set_text_alignment(layer_time_digit_4, GTextAlignmentRight);
  text_layer_set_font(layer_time_digit_4, font_time);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_time_digit_4);
  // layer_time_digit_5
  layer_time_digit_5 = text_layer_create(GRect(114, 45, 36, 50));
  text_layer_set_background_color(layer_time_digit_5, GColorClear);
  text_layer_set_text_color(layer_time_digit_5, GColorWhite);
  text_layer_set_text_alignment(layer_time_digit_5, GTextAlignmentRight);
  text_layer_set_font(layer_time_digit_5, font_time);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_time_digit_5);
}

static void destroy_ui(void) {
  bitmap_layer_destroy(layer_batt_bar);
  bitmap_layer_destroy(layer_time_fg);
  bitmap_layer_destroy(layer_date_bg);
  text_layer_destroy(layer_date);
  text_layer_destroy(layer_time_digit_1);
  text_layer_destroy(layer_time_digit_2);
  text_layer_destroy(layer_time_digit_3);
  text_layer_destroy(layer_time_digit_4);
  text_layer_destroy(layer_time_digit_5);
  fonts_unload_custom_font(font_date);
  fonts_unload_custom_font(font_time);
  gbitmap_destroy(bmp_batt_bar);
  gbitmap_destroy(bmp_time_bg);
  gbitmap_destroy(bmp_date_fg);
  layer_destroy(layer);
  window_destroy(window);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_window(void) {
  initialise_ui();
  window_set_window_handlers(window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick);
  bluetooth_connection_service_subscribe(bluetooth_connection_handler);
  bt_connect_toggle = bluetooth_connection_service_peek();
  if (bt_connect_toggle) {
    layer_set_hidden(bitmap_layer_get_layer(layer_date_bg), true);
  }
  else {
    layer_set_hidden(bitmap_layer_get_layer(layer_date_bg), false);
  }
}

void hide_window(void) {
  window_stack_remove(window, true);
}

int main(void) {
  show_window();
  app_event_loop();
  hide_window();
}