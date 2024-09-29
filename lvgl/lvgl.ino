#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass touchscreenSpi = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

#define TFT_HOR_RES 320
#define TFT_VER_RES 240

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	/*Call it to tell LVGL you are ready*/
	lv_disp_flush_ready(disp);
}

lv_obj_t *passwordTa;

// Screens
lv_obj_t *getPasswordScreen;

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
	if (touchscreen.touched())
	{
		TS_Point p = touchscreen.getPoint();
		// Some very basic auto calibration so it doesn't go out of range
		if (p.x < touchScreenMinimumX)
			touchScreenMinimumX = p.x;
		if (p.x > touchScreenMaximumX)
			touchScreenMaximumX = p.x;
		if (p.y < touchScreenMinimumY)
			touchScreenMinimumY = p.y;
		if (p.y > touchScreenMaximumY)
			touchScreenMaximumY = p.y;
		// Map this to the pixel position
		data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 1, TFT_HOR_RES); /* Touchscreen X calibration */
		data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 1, TFT_VER_RES); /* Touchscreen Y calibration */
		data->state = LV_INDEV_STATE_PRESSED;
	}
	else
	{
		data->state = LV_INDEV_STATE_RELEASED;
	}
}

lv_indev_t *indev;
uint8_t *draw_buf;
uint32_t lastTick = 0;

static void my_keyboard_event_cb(lv_event_t *e)
{
	const char *password = lv_textarea_get_text(passwordTa);
	Serial.printf("Inputed pass: %s\n", password);
	if (!strcmp(password, "123"))
	{
		getPasswordScreenLayout();
		lv_scr_load(getPasswordScreen);
	}
	else
	{
		Serial.printf("Invalid password\n");
	}
}

void getPasswordScreenLayout()
{
	lv_obj_t *text_label = lv_label_create(getPasswordScreen);
	lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
	lv_label_set_text(text_label, "Select service");
	lv_obj_set_width(text_label, 150);
	lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(text_label, LV_ALIGN_CENTER, 0, -90);

	lv_obj_t *selectCategoryDD = lv_dropdown_create(getPasswordScreen);
	lv_dropdown_set_options(selectCategoryDD, "web\n"
								"ssh\n");

	lv_obj_align(selectCategoryDD, LV_ALIGN_LEFT_MID, 0, 0);
	// lv_obj_add_event_cb(selectCategoryDD, event_handler, LV_EVENT_ALL, NULL);

	lv_obj_t *selectServiceDD = lv_dropdown_create(getPasswordScreen);
	lv_dropdown_set_options(selectServiceDD, "gmail\n"
								"protonmail\n");

	lv_obj_align(selectServiceDD, LV_ALIGN_LEFT_MID, 0, 0);
	// lv_obj_add_event_cb(selectServiceDD, event_handler, LV_EVENT_ALL, NULL);



}

void setup()
{
	// Some basic info on the Serial console
	Serial.begin(115200);

	// Initialise the touchscreen
	touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); /* Start second SPI bus for touchscreen */
	touchscreen.begin(touchscreenSpi);										   /* Touchscreen init */
	touchscreen.setRotation(3);												   /* Inverted landscape orientation to match screen */

	// Initialise LVGL
	lv_init();
	draw_buf = new uint8_t[DRAW_BUF_SIZE];
	lv_display_t *disp;
	disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

	// Initialize the XPT2046 input device driver
	indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_touchpad_read);

	// Init Ok

	// Create screens
	getPasswordScreen = lv_obj_create(NULL);

	// Main screen input passsword

	lv_obj_t *text_label = lv_label_create(lv_screen_active());
	lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
	lv_label_set_text(text_label, "Enter master-key");
	lv_obj_set_width(text_label, 150);
	lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(text_label, LV_ALIGN_CENTER, 0, -90);

	static const char *kb_map[] = {"0", "1", "2", "3", "4", "5", "6", "\n",
								   "7", "8", "9", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, NULL};
	static const lv_buttonmatrix_ctrl_t kb_ctrl[] = {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

	lv_obj_t *kb = lv_keyboard_create(lv_screen_active());

	lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
	lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
	lv_obj_add_event_cb(kb, my_keyboard_event_cb, LV_EVENT_READY, kb);

	passwordTa = lv_textarea_create(lv_screen_active());
	lv_obj_align(passwordTa, LV_ALIGN_TOP_MID, 0, 50);
	lv_obj_set_size(passwordTa, lv_pct(90), 80);
	lv_obj_add_state(passwordTa, LV_STATE_FOCUSED);
	lv_textarea_set_password_mode(passwordTa, true);
	lv_textarea_set_one_line(passwordTa, true);

	lv_keyboard_set_textarea(kb, passwordTa);

	Serial.println("Setup done");
}

void loop()
{
	lv_tick_inc(millis() - lastTick); // Update the tick timer. Tick is new for LVGL 9
	lastTick = millis();
	lv_timer_handler(); // Update the UI
	delay(5);
}
