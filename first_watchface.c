#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont s_time_font;
#define PERCMINUTES 0.072
#define PERCHOURS 4.167

static void update_time(){
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "0000";
  int hours, minutes, percentage;

  // Get a buffer to contain hours space minutes.
  strftime(buffer, 5,"%H", tick_time);
  hours = atoi(buffer);
  
  strftime(buffer, 5,"%M", tick_time);
  minutes = atoi(buffer);
  
  // Transfor the time into percentage
  percentage = hours * PERCHOURS + minutes * PERCMINUTES;
  
  snprintf(buffer, 4, "%d%%", percentage);

  // // Write the current hours and minutes into the buffer
  // if(clock_is_24h_style() == true) {
  //   // Use 24h format 
  //   strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  // } else {
  //   // Use 12 hour format
  //   strftime(buffer,sizeof("00:00"),"%I:%M", tick_time);
  // }

  // Display this time on the TextLayer 
  text_layer_set_text(s_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}

static void main_window_load(Window *window){
  
  window_set_background_color(s_main_window, GColorSunsetOrange);

  // Create time text layer
  s_time_layer = text_layer_create(GRect(0, 55, 144, 50)); 
  text_layer_set_background_color(s_time_layer, GColorClear); 
  text_layer_set_text_color(s_time_layer, GColorBlack);

  // Create font
  // s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CYBERFUNK_42));

  // Improving the layout
  text_layer_set_font(s_time_layer,fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer,GTextAlignmentCenter);

  // Add it as a child layer for the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

}

static void main_window_unload(Window *window){
  // Destroy TextLayer
  text_layer_destroy(s_time_layer); 

  // Unload GFont
  fonts_unload_custom_font(s_time_font);
}

static void init(){
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  window_stack_push(s_main_window, true);

  // Make sure time is displayed from the start.
  update_time();
}

static void deinit(){
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}

