#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_digit1_layer, *s_digit2_layer, *s_sign_layer, *s_date_layer, *s_time_top_layer, *s_weather_layer;
static Layer *container_layer, *footer_layer, *weather_container;
static GFont s_font;
#define PERCMINUTES 0.072
#define PERCHOURS 4.167
#define DAY (24 * 60) //amount of minutes of a day
#define WAKE_TIME 8 * 60 //time to wake up in minutes
#define SLEEP_TIME 0 * 60 //time to go to sleep in minutes
#define BLINK_ANIMATION_LENGTH 38 //Length in frames
#define ANIMATION_FRAMERATE 25
static uint8_t animation_frame = BLINK_ANIMATION_LENGTH;
static uint8_t animation_happening = 0;
static union GColor8 colors[12]; 
static int sleep_total, awake_total;
struct Theme{
  GColor8 text_color; 
  GColor8 box_color;
  GColor8 box_text_color;
  GColor8 background_color;
};
static struct Theme current_theme;
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

// test variables
#define TIME_LAYER_WIDTH 90
#define TIME_LAYER_HEIGHT 70


static void set_day_variables(){
  if(WAKE_TIME < SLEEP_TIME){
    sleep_total = DAY - SLEEP_TIME + WAKE_TIME;
    awake_total = SLEEP_TIME - WAKE_TIME;
  }else{
    sleep_total = WAKE_TIME - SLEEP_TIME;
    awake_total = DAY - WAKE_TIME + SLEEP_TIME;
  }
}

static int calculate_color(int curr_hour, int curr_minutes){
  int curr_time = curr_hour * 60 + curr_minutes;
  int relative_time;
  int response;

  
  // Case 1 like wake up at 7 go to sleep at 22 
  if(WAKE_TIME < SLEEP_TIME){   
    //Sleeping
    if(curr_time < WAKE_TIME || curr_time > SLEEP_TIME){
      if(curr_time > SLEEP_TIME){
        relative_time = curr_time - SLEEP_TIME;
      }
      else{
        relative_time =curr_time + DAY - SLEEP_TIME;
      }
      response = 11 - relative_time*12 / sleep_total;
    }
    // Awake
    else{
      relative_time = curr_time - WAKE_TIME;
      response = relative_time * 12 / awake_total;
    }
  // Case 2 like wake up at 9 go to sleep at 1
  }else{
    // Sleeping
    if(curr_time < WAKE_TIME && curr_time >= SLEEP_TIME){
      relative_time = curr_time - SLEEP_TIME;
      response = 11 - relative_time * 12 / sleep_total;
    }
    // Awake
    else{
      if(curr_time < SLEEP_TIME){
        relative_time = curr_time + DAY - WAKE_TIME;
      }
      else{
        relative_time = curr_time - WAKE_TIME;
      }
      response = relative_time * 12 / awake_total;
    }
  }
  return response;
}

static void set_theme_colors(uint8_t curr_hour, uint8_t curr_minutes){
  uint8_t color_index = calculate_color(curr_hour, curr_minutes);
  current_theme.background_color = colors[color_index];
  if(color_index == 6){
    current_theme.text_color = GColorOrange;
    current_theme.box_color = GColorOrange;  
  }else{
    current_theme.text_color = GColorWhite;
    current_theme.box_color = GColorWhite; 
  }
  current_theme.box_text_color = current_theme.background_color;
}

static Animation * animate_numbers(){
  // Animation digit 1
  GRect start, finish;
  Layer * display_layer = text_layer_get_layer(s_digit1_layer);
  // going up
  start = GRect(42,55, TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  finish = GRect(42,- TIME_LAYER_HEIGHT,TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  PropertyAnimation * prop_anim_middle_up_digit1 = property_animation_create_layer_frame(display_layer,&start,&finish);
  Animation * anim_middle_up_digit1 = property_animation_get_animation(prop_anim_middle_up_digit1);
  animation_set_duration(anim_middle_up_digit1, 700);
  animation_set_curve((Animation*) anim_middle_up_digit1, AnimationCurveEaseIn);

  // going down
  start = GRect(42,168 + TIME_LAYER_HEIGHT,TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  finish = GRect(42,55,TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  PropertyAnimation * prop_anim_bottom_up_digit1 = property_animation_create_layer_frame(display_layer,&start,&finish);
  Animation * anim_bottom_up_digit1 = property_animation_get_animation(prop_anim_bottom_up_digit1);
  animation_set_duration(anim_bottom_up_digit1, 900);
  animation_set_curve((Animation*) anim_bottom_up_digit1, AnimationCurveEaseOut); 
  // Set as sequence
  Animation * seq_text1 = animation_sequence_create(anim_middle_up_digit1,anim_bottom_up_digit1, NULL);

  // Animation digit 2
  // going down
  display_layer = text_layer_get_layer(s_digit2_layer);
  start = GRect(67,55,TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  finish = GRect(67,168 + TIME_LAYER_HEIGHT,TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  PropertyAnimation * prop_anim_middle_down_digit2 = property_animation_create_layer_frame(display_layer,&start,&finish);
  Animation * anim_middle_down_digit2 = property_animation_get_animation(prop_anim_middle_down_digit2);
  animation_set_duration(anim_middle_down_digit2, 900);
  animation_set_curve((Animation*) anim_middle_down_digit2, AnimationCurveEaseIn);
  
  // going up
  start = GRect(67,- TIME_LAYER_HEIGHT, TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  finish = GRect(67,55,TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT);
  PropertyAnimation * prop_anim_top_down_digit2 = property_animation_create_layer_frame(display_layer,&start,&finish);
  Animation * anim_top_down_digit2 = property_animation_get_animation(prop_anim_top_down_digit2);
  animation_set_duration(anim_top_down_digit2, 700);
  animation_set_curve((Animation*) anim_top_down_digit2, AnimationCurveEaseOut);
  // set as sequence
  Animation * seq_text2 = animation_sequence_create(anim_middle_down_digit2, anim_top_down_digit2, NULL);
  Animation *spawn = animation_spawn_create(seq_text1, seq_text2, NULL);
  return spawn;
}

static void blink_colors(){
  // Temporarily changed the first item from white to orange for better visibility
  GColor8 colors[19] = {GColorWhite, GColorSunsetOrange, GColorOrange, GColorChromeYellow, GColorYellow, GColorSpringBud, GColorBrightGreen, GColorScreaminGreen, GColorMalachite, GColorMediumSpringGreen, GColorCyan, GColorVividCerulean, GColorBlueMoon, GColorVeryLightBlue, GColorElectricUltramarine, GColorVividViolet, GColorMagenta, GColorFashionMagenta, GColorFolly};
  if(animation_frame < BLINK_ANIMATION_LENGTH){
    // Register start time of animation
    uint16_t start_ms = time_ms(NULL,NULL);
    // Increment animation frame and redraq window.
    animation_frame++;
    uint8_t tempvalue = animation_frame % 19;

    text_layer_set_text_color(s_digit1_layer,colors[tempvalue]);
    text_layer_set_text_color(s_digit2_layer,colors[(19 - tempvalue)%19]);
    // Register end time of drawing
    uint16_t end_ms = time_ms(NULL,NULL);
    uint16_t draw_time = end_ms - start_ms;

    if(draw_time < 1000/ANIMATION_FRAMERATE){
      // If draw time is faster than the framerate wait until it is time to draw the next frame
      int wait_time = 1000/ANIMATION_FRAMERATE - draw_time;

      app_timer_register(wait_time,blink_colors,NULL);
    }else{
      blink_colors();
    }
  }else{
    // Else set the animation to the final frame and don't recall the function
    animation_frame = BLINK_ANIMATION_LENGTH;
    time_t temp = time(NULL);
    struct tm *t = localtime(&temp);
    set_theme_colors(t->tm_hour,t->tm_min);
  }
}

static void update_date(){
  static char tempbuffer[12];
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  strftime(tempbuffer,12,"%c",tick_time);
  text_layer_set_text(s_date_layer,tempbuffer);
}

static void update_time(){
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  uint8_t curr_hour = t->tm_hour;
  uint8_t curr_minutes = t->tm_min;
  uint16_t percentage;
  static char buffer1[2], buffer2[2], buffer3[6];
  // Transfor the time into percentage
  percentage = (((curr_hour * 60) + curr_minutes)*100) / DAY;
  
  // Update theme colors according to time of the day.
  set_theme_colors(curr_hour, curr_minutes);
  window_set_background_color(s_main_window, current_theme.background_color);
  // Make everything refresh
  layer_mark_dirty(window_get_root_layer(s_main_window));
 
  snprintf(buffer1, 2, "%d", percentage/10);
  snprintf(buffer2, 2, "%d", percentage %10);
  snprintf(buffer3,6, "%02d:%02d", curr_hour,curr_minutes);
  // Update Percentage 
  text_layer_set_text(s_digit1_layer, buffer1);
  text_layer_set_text(s_digit2_layer, buffer2);

  // Update Top Time layer
  text_layer_set_text(s_time_top_layer,buffer3);

  APP_LOG(APP_LOG_LEVEL_INFO, "Percentage: %d", percentage);
}

static void container_update(){
  if(animation_happening == 0){
    text_layer_set_text_color(s_digit1_layer, current_theme.text_color);
    text_layer_set_text_color(s_digit2_layer, current_theme.text_color);
    text_layer_set_text_color(s_sign_layer, current_theme.text_color);
  }
}
static void weather_container_update_procedure(Layer *layer, GContext *ctx){
  graphics_context_set_stroke_color(ctx,current_theme.text_color);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_fill_color(ctx,GColorClear);
  graphics_draw_circle(ctx, GPoint(26,9), 2);
  text_layer_set_text_color(s_weather_layer, current_theme.text_color);
}

static void update_weather(){
  // Request Weather info
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);

  // Send the message!
  app_message_outbox_send();
}

static Animation * animate_weather_container(){
  GRect start, finish;
  start = GRect(110,-30,50,30);
  finish = GRect(110,5,50,30);
  PropertyAnimation * prop_anim_top_down_weather_container = property_animation_create_layer_frame(weather_container,&start,&finish);
  Animation * anim_top_down_weather_container = property_animation_get_animation(prop_anim_top_down_weather_container);
  animation_set_duration(anim_top_down_weather_container,600);
  animation_set_delay(anim_top_down_weather_container, 200);

  PropertyAnimation * prop_anim_down_up_weather_container = property_animation_create_layer_frame(weather_container,&finish,&start);
  Animation * anim_down_up_weather_container = property_animation_get_animation(prop_anim_down_up_weather_container);
  animation_set_duration(anim_down_up_weather_container,1000);
  animation_set_delay(anim_down_up_weather_container, 3000);

  Animation * seq_weather_container = animation_sequence_create(anim_top_down_weather_container, anim_down_up_weather_container,NULL);

  return seq_weather_container;
}
static Animation * animate_top_time_layer(){
  GRect start, finish;
  start = GRect(-70,5,70,40);
  finish = GRect(7,5,70,40);
  PropertyAnimation * prop_anim_right_time_top_layer = property_animation_create_layer_frame(text_layer_get_layer(s_time_top_layer),&start,&finish);
  Animation * anim_right_time_top_layer = property_animation_get_animation(prop_anim_right_time_top_layer);
  animation_set_duration(anim_right_time_top_layer, 600);
  animation_set_delay(anim_right_time_top_layer, 100);
  animation_set_curve((Animation*) anim_right_time_top_layer, AnimationCurveEaseOut);

  PropertyAnimation * prop_anim_left_time_top_layer = property_animation_create_layer_frame(text_layer_get_layer(s_time_top_layer),&finish,&start);
  Animation * anim_left_time_top_layer = property_animation_get_animation(prop_anim_left_time_top_layer);
  animation_set_duration(anim_left_time_top_layer, 700);
  animation_set_delay(anim_left_time_top_layer, 3000);
animation_set_curve((Animation*) anim_left_time_top_layer, AnimationCurveEaseIn);

  Animation * seq_time_top = animation_sequence_create(anim_right_time_top_layer,anim_left_time_top_layer,NULL);
  return seq_time_top;

}

static Animation * animate_footer(){
  GRect start, finish;

  // Going up
  start = GRect(0,168,144, 30);
  finish = GRect(0,145,144, 30);
  PropertyAnimation * prop_anim_bottom_up_footer = property_animation_create_layer_frame(footer_layer,&start,&finish);
  Animation * anim_bottom_up_footer = property_animation_get_animation(prop_anim_bottom_up_footer);
  animation_set_duration(anim_bottom_up_footer, 500);
  animation_set_curve((Animation*) anim_bottom_up_footer, AnimationCurveEaseOut); 

  // Going down
  PropertyAnimation * prop_anim_going_down_footer = property_animation_create_layer_frame(footer_layer,&finish,&start);
  Animation * anim_going_down_footer = property_animation_get_animation(prop_anim_going_down_footer);
  animation_set_duration(anim_going_down_footer, 600);
  animation_set_delay(anim_going_down_footer,3000);
  animation_set_curve((Animation*) anim_going_down_footer, AnimationCurveEaseIn); 
  // Set as sequence
  Animation * seq_footer = animation_sequence_create(anim_bottom_up_footer,anim_going_down_footer, NULL);

  return seq_footer;
}

static void footer_update(Layer *layer, GContext *ctx){
  graphics_context_set_fill_color(ctx, current_theme.box_color);
  graphics_fill_rect(ctx, layer_get_bounds(footer_layer),0, GCornerNone);
  text_layer_set_text_color(s_date_layer,current_theme.box_text_color);

  // This is not in the best place but is just to avoid to much headache
  text_layer_set_text_color(s_time_top_layer,current_theme.text_color);
}


static void reset_animation_happening(){
  animation_happening = 0;
}

static void animate_everything(){
  // If animation is not happening;
  if(animation_happening == 0){
    // Set animation as happening
    animation_happening = 1;
    uint16_t wait_time = 4000;
    // Set animation as not happening in wait_time miliseonds
    app_timer_register(wait_time,reset_animation_happening,NULL);
    animation_frame = 0;
    blink_colors();
    Animation * anim_everything = animation_spawn_create(animate_numbers(),animate_footer(),animate_top_time_layer(), animate_weather_container());  
    animation_schedule(anim_everything);
  }
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  animate_everything();
  update_time();
  update_date();
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  
  switch (axis) {
  case ACCEL_AXIS_X:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "X axis positive.");
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "X axis negative.");
    }
    break;
  case ACCEL_AXIS_Y:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Y axis positive.");
      // Call all animations when flickering the wrist inwards.
      light_enable_interaction();
      animate_everything();
      update_weather();  
    } else {
      
      APP_LOG(APP_LOG_LEVEL_INFO, "Y axis negative.");   
    }
    break;
  case ACCEL_AXIS_Z:
    if (direction > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Z axis positive.");
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "Z axis negative.");
    }
    break;
  }
}

static void main_window_load(Window *window){
  
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ALLER_36));

  window_set_background_color(s_main_window, GColorRed);
    // Setting theme text color.
  current_theme.text_color = GColorWhite;
  current_theme.box_text_color = GColorRed;
  current_theme.box_color = GColorWhite;

  //PERCENTAGE SETUP
  // Create container layer for percentage
  container_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(container_layer,container_update);
  layer_add_child(window_get_root_layer(window), container_layer);

  // Create Text layers
  s_digit1_layer = text_layer_create(GRect(42,55,TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT)); 
  text_layer_set_background_color(s_digit1_layer, GColorClear); 
  text_layer_set_text_color(s_digit1_layer, current_theme.text_color);
  text_layer_set_font(s_digit1_layer,fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  layer_add_child(container_layer, text_layer_get_layer(s_digit1_layer));

  s_digit2_layer = text_layer_create(GRect(67,55, TIME_LAYER_WIDTH, TIME_LAYER_HEIGHT)); 
  text_layer_set_background_color(s_digit2_layer, GColorClear); 
  text_layer_set_text_color(s_digit2_layer, current_theme.text_color);
  text_layer_set_font(s_digit2_layer,fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  layer_add_child(container_layer, text_layer_get_layer(s_digit2_layer));

  s_sign_layer = text_layer_create(GRect(90,57,45,55)); 
  text_layer_set_background_color(s_sign_layer, GColorClear); 
  text_layer_set_text_color(s_sign_layer, current_theme.text_color);
  text_layer_set_font(s_sign_layer,s_font);
  text_layer_set_text_alignment(s_sign_layer,GTextAlignmentCenter);
  text_layer_set_text(s_sign_layer,"%");
  layer_add_child(container_layer, text_layer_get_layer(s_sign_layer));
  // --------------END OF PERCENTAGE SETUP------------------ //

  // FOOTER SETUP
  footer_layer = layer_create(GRect(0,148,144,30));
  layer_set_update_proc(footer_layer,footer_update);
  layer_add_child(window_get_root_layer(window),footer_layer);

  s_date_layer = text_layer_create(GRect(0,-2,144,30));
  text_layer_set_background_color(s_date_layer,GColorClear);
  text_layer_set_text_color(s_date_layer,current_theme.box_text_color);
  text_layer_set_font(s_date_layer,fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_layer,GTextAlignmentCenter);
  layer_add_child(footer_layer,text_layer_get_layer(s_date_layer));

  // SETUP TIME TOP LAYER
  s_time_top_layer = text_layer_create(GRect(-70,5,70,40));
  text_layer_set_font(s_time_top_layer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_background_color(s_time_top_layer,GColorClear);
  text_layer_set_text_color(s_time_top_layer, current_theme.text_color);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_top_layer));



  // SETUP WEATHER LAYER
  weather_container =  layer_create(GRect(110,-30,50,30));
  layer_set_update_proc(weather_container,weather_container_update_procedure);
  layer_add_child(window_get_root_layer(window),weather_container);


  s_weather_layer = text_layer_create(GRect(0,0,40,29));
  text_layer_set_background_color(s_weather_layer,GColorClear);
  text_layer_set_text_color(s_weather_layer, current_theme.text_color);
  text_layer_set_font(s_weather_layer,fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  layer_add_child(weather_container, text_layer_get_layer(s_weather_layer));
}

static void main_window_unload(Window *window){
  // Destroy TextLayer
  text_layer_destroy(s_digit1_layer);
  text_layer_destroy(s_digit2_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_top_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_sign_layer);
  layer_destroy(container_layer);
  layer_destroy(footer_layer);
  layer_destroy(weather_container);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int) t->value->int32);
      APP_LOG(APP_LOG_LEVEL_INFO, "%s", temperature_buffer);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  APP_LOG(APP_LOG_LEVEL_INFO, "%s", weather_layer_buffer);
  text_layer_set_text(s_weather_layer, temperature_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


static void init(){
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // adding colors to array
  colors[0] = GColorDarkGreen;
  colors[1] = GColorKellyGreen;
  colors[2] = GColorIslamicGreen;
  colors[3] = GColorGreen;
  colors[4] = GColorSpringBud;
  colors[5] = GColorInchworm;
  colors[6] = GColorIcterine;
  colors[7] = GColorRajah; //Yellow;
  colors[8] = GColorRajah;
  colors[9] = GColorChromeYellow;
  colors[10] = GColorOrange;
  colors[11] = GColorRed;

  // start sleep_time and awake_time
  set_day_variables();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  //  Subscribe to tap service ( will call the callback when the user flicker his wrist)
  accel_tap_service_subscribe(tap_handler);

  window_stack_push(s_main_window, true);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Make sure time is displayed from the start.
  update_time();
  update_date();
}

static void deinit(){
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}
