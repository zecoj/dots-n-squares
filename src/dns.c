#include <pebble.h>

#define BATT_BAR_WDTH 146
#define SHAKE_TIMEOUT 5000

#define CONF_COLOUR                0
#define CONF_BLUETOOTH             1
#define CONF_WEATHER               2
#define WEATHER_ICON_KEY           3

#define MyTupletCString(_key, _cstring) ((const Tuplet) { .type = TUPLE_CSTRING, .key = _key, .cstring = { .data = _cstring, .length = strlen(_cstring) + 1 }})

static Window *window;
static GFont font_date;
static GFont font_time;
static GFont font_weather;
static GBitmap *bmp_batt_bar;
static GBitmap *bmp_time_bg;
static GBitmap *bmp_date_fg;
static BitmapLayer *layer_batt_bar;
static BitmapLayer *layer_time_fg;
static BitmapLayer *layer_date_fg;
static TextLayer *layer_time_digit_1;
static TextLayer *layer_time_digit_2;
static TextLayer *layer_time_digit_3;
static TextLayer *layer_time_digit_4;
static TextLayer *layer_time_digit_5;
static TextLayer *layer_date;
static TextLayer *layer_weather;
static Layer *layer;
static bool bt_connect_toggle;
static AppTimer *shake_timeout = NULL;
static InverterLayer *inverter_layer;

static AppSync sync;
static uint8_t sync_buffer[128];
static char weather_str[20];
static uint8_t bluetooth = 2;
static uint8_t colour = 1;
static uint8_t weather = 30;

void hide_weather() {
  layer_set_hidden(text_layer_get_layer(layer_weather), true);
  layer_set_hidden(text_layer_get_layer(layer_date), false);
  shake_timeout = NULL;
}

void show_weather() {
  layer_set_hidden(text_layer_get_layer(layer_date), true);
  layer_set_hidden(text_layer_get_layer(layer_weather), false);
}

void wrist_flick_handler(AccelAxisType axis, int32_t direction) {
  if (axis == 1 && !shake_timeout) {
    show_weather();
    shake_timeout = app_timer_register (SHAKE_TIMEOUT, hide_weather, NULL);
  }
}

void bluetooth_connection_handler(bool connected) {
  if (!bt_connect_toggle && connected) {
    bt_connect_toggle = true;
    if(bluetooth==2) vibes_short_pulse();
    if(bluetooth!=0) layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), true);
    // resubscribe from shake event
    accel_tap_service_subscribe(wrist_flick_handler);
  }
  if (bt_connect_toggle && !connected) {
    bt_connect_toggle = false;
    if(bluetooth==2) vibes_long_pulse();
    if(bluetooth!=0) layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), false);
    // unsubscribe from shake event
    accel_tap_service_unsubscribe();
  }
}

void update_window(Layer *me, GContext* ctx)
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
  if(weather && tick_time->tm_min % weather == 0) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "MINUTE_TICK I'm supposed to check the weather now");
    app_message_outbox_send();
  }
  layer_mark_dirty(layer);
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    persist_read_string(WEATHER_ICON_KEY, weather_str, sizeof(weather_str));
    text_layer_set_text(layer_weather, weather_str);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  // process the first and subsequent update
  switch (key) {
    case CONF_BLUETOOTH:
      if (bluetooth != new_tuple->value->uint8) {
        //if off -> on: resubscribe bluetooth & tap
        if (new_tuple->value->uint8 && !bluetooth) {
          bluetooth_connection_service_subscribe(bluetooth_connection_handler);
          //bt_connect_toggle = bluetooth_connection_service_peek();
          //if (bt_connect_toggle) { layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), true); }
          //else { layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), false); }
          //if bluetooth is enabled, only subscribe to tap if weather is enabled
          if (weather) accel_tap_service_subscribe(wrist_flick_handler);
        }
        //if off: unsubscribe bluetooth & tap
        if (!new_tuple->value->uint8) {
          bluetooth_connection_service_unsubscribe();
          accel_tap_service_unsubscribe();
          layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), true);
        }
        bluetooth=new_tuple->value->uint8;
        persist_write_int(CONF_BLUETOOTH, bluetooth);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNCTUPLE> bluetooth: %u", bluetooth);
      }
      break;
    case CONF_WEATHER:
      if(new_tuple->value->uint8 && !weather) {
        app_message_outbox_send();
        accel_tap_service_subscribe(wrist_flick_handler);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNCTUPLE> weather: subscribing");
      }
      if (!new_tuple->value->uint8) {
        accel_tap_service_unsubscribe();
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNCTUPLE> weather: unsubscribing");
      }
      weather = new_tuple->value->uint8;
      persist_write_int(CONF_WEATHER, weather);
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNCTUPLE> weather: %u", weather);
      break;
    case CONF_COLOUR:
      if (new_tuple->value->uint8 != colour) {
        colour = new_tuple->value->uint8;
        layer_set_hidden(inverter_layer_get_layer(inverter_layer), colour);
        persist_write_int(CONF_COLOUR, colour);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNCTUPLE> colour: %u", colour);
      }
      break;
    case WEATHER_ICON_KEY:
      if (new_tuple->value->cstring != NULL) {
        strcpy(weather_str, new_tuple->value->cstring);
        persist_write_string(WEATHER_ICON_KEY, weather_str);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNCTUPLE> weather_icon: %s", weather_str);
        text_layer_set_text(layer_weather, weather_str);
      }
      break;
  }
}

static void initialise_ui(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, true);

  bmp_batt_bar = gbitmap_create_with_resource(RESOURCE_ID_BATT_BAR);
  bmp_time_bg = gbitmap_create_with_resource(RESOURCE_ID_DIGITS_BACKGROUND);
  bmp_date_fg = gbitmap_create_with_resource(RESOURCE_ID_DATE_FOREGROUND_BLACK);
  font_weather = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_IMAGINE_16));
  font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_IMAGINE_20));
  font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_IMAGINE_48));

  // layer_batt_bar
  layer_batt_bar = bitmap_layer_create(GRect((144-BATT_BAR_WDTH)/2, 100, BATT_BAR_WDTH, 5));
  layer_time_fg = bitmap_layer_create(GRect(0,61,144,34));
  layer_date_fg = bitmap_layer_create(GRect(1,105,144,20));
  bitmap_layer_set_bitmap(layer_batt_bar, bmp_batt_bar);
  bitmap_layer_set_bitmap(layer_time_fg, bmp_time_bg);
  bitmap_layer_set_bitmap(layer_date_fg, bmp_date_fg);
  bitmap_layer_set_background_color(layer_date_fg, GColorClear);
  bitmap_layer_set_compositing_mode(layer_date_fg, GCompOpAnd);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_batt_bar);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_time_fg);
  layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(layer, update_window);
  layer_add_child(window_get_root_layer(window), layer);  
  
  // layer_date
  layer_date = text_layer_create(GRect(2, 103, 145, 24));
  text_layer_set_background_color(layer_date, GColorClear);
  text_layer_set_text_color(layer_date, GColorWhite);
  text_layer_set_text_alignment(layer_date, GTextAlignmentRight);
  text_layer_set_font(layer_date, font_date);
  layer_add_child(window_get_root_layer(window), (Layer *)layer_date);
  
  // layer_weather
  layer_weather = text_layer_create(GRect(0, 104, 145, 48));
  text_layer_set_background_color(layer_weather, GColorClear);
  text_layer_set_text_color(layer_weather, GColorWhite);
  text_layer_set_text_alignment(layer_weather, GTextAlignmentRight);
  text_layer_set_font(layer_weather, font_weather);
  text_layer_set_text(layer_weather, "fetching...");
  layer_add_child(window_get_root_layer(window), (Layer *)layer_weather);
  layer_set_hidden(text_layer_get_layer(layer_weather), true);
  
  layer_add_child(window_get_root_layer(window), (Layer *)layer_date_fg);
  layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), true);
  
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
  
  inverter_layer = inverter_layer_create(GRect(0,0,144,168));
  layer_set_hidden(inverter_layer_get_layer(inverter_layer), colour);
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_layer));
}

static void destroy_ui(void) {
  accel_tap_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  bitmap_layer_destroy(layer_batt_bar);
  bitmap_layer_destroy(layer_time_fg);
  bitmap_layer_destroy(layer_date_fg);
  text_layer_destroy(layer_date);
  text_layer_destroy(layer_weather);
  text_layer_destroy(layer_time_digit_1);
  text_layer_destroy(layer_time_digit_2);
  text_layer_destroy(layer_time_digit_3);
  text_layer_destroy(layer_time_digit_4);
  text_layer_destroy(layer_time_digit_5);
  fonts_unload_custom_font(font_date);
  fonts_unload_custom_font(font_time);
  fonts_unload_custom_font(font_weather);
  gbitmap_destroy(bmp_batt_bar);
  gbitmap_destroy(bmp_time_bg);
  gbitmap_destroy(bmp_date_fg);
  layer_destroy(layer);
  inverter_layer_destroy(inverter_layer);
  window_destroy(window);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_window(void) {
  
  // Load settings from persistent storage
  if (persist_exists(CONF_COLOUR))
  {
    colour = persist_read_bool(CONF_COLOUR);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "INIT> colour: %d", colour);
  }
  if (persist_exists(CONF_BLUETOOTH))
  {
    bluetooth = persist_read_bool(CONF_BLUETOOTH);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "INIT> bluetooth: %u", bluetooth);
  }
  if (persist_exists(CONF_WEATHER))
  {
    weather = persist_read_int(CONF_WEATHER);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "INIT> weather: %u", weather);
  }

  initialise_ui();
  window_set_window_handlers(window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick);

  //only subscribe to bluetooth & tap service if bluetooth is enabled
  if (bluetooth) {
    bluetooth_connection_service_subscribe(bluetooth_connection_handler);
    bt_connect_toggle = bluetooth_connection_service_peek();
    if (bt_connect_toggle) {
      layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), true);
    }
    else {
      layer_set_hidden(bitmap_layer_get_layer(layer_date_fg), false);
    }
    //if bluetooth is enabled, only subscribe to tap if weather is enabled
    if (weather) accel_tap_service_subscribe(wrist_flick_handler);
  }
  
  // prepare the initial values of your data
  Tuplet initial_values[] = {
      TupletInteger(CONF_COLOUR, (uint8_t) colour),
      TupletInteger(CONF_BLUETOOTH, (uint8_t) bluetooth),
      TupletInteger(CONF_WEATHER, (uint8_t) weather),
      MyTupletCString(WEATHER_ICON_KEY, "fetching...")
  };
  // initialize the syncronization
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, sync_error_callback, NULL);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void hide_window(void) {
  window_stack_remove(window, true);
}

int main(void) {
  show_window();
  app_event_loop();
  hide_window();
}