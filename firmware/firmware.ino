#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "SPIFFS.h"
#include "FS.h"
#include "SPI.h"
#include "SD.h"
#include <ArduinoJson.h>
#include "Cipher.h"
#include <string>

// Touch screen SPI pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Some config
#define PASSWORDS_FILE_PATH "/passwords.crypt"
#define MAINTANCE_MODE // For dump/write passwords on device via PC (maybe for increasing security you need to disable this feature)
#define DEV_AUTO_AUTH  // Remove on prod build
#define SECURE_NOTES   // External file with same encryption as passwords for storing text info // TODO implement
#ifdef SECURE_NOTES
#define SECURE_NOTES_FILE_PATH "/notes.crypt"
#endif

// #define INSECURE_MAINTANCE_ENABLE //

SPIClass touchscreenSpi = SPIClass(VSPI);
Cipher *cipher = new Cipher();

XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

#define TFT_HOR_RES 320
#define TFT_VER_RES 240

#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	lv_disp_flush_ready(disp);
}

lv_obj_t *passwordTa;
#ifdef SECURE_NOTES
lv_obj_t *notesTa;
#endif
DynamicJsonDocument readedPasswords(2048);

#ifdef SECURE_NOTES
String readedSecureNotes;
#endif

char categoriesList[256];
char servicesList[1024];

// Screens
lv_obj_t *getPasswordScreen;
lv_obj_t *showPasswordScreen;
lv_obj_t *addPasswordScreen;
#ifdef SECURE_NOTES
lv_obj_t *notesScreen;
#endif

#ifdef MAINTANCE_MODE
lv_obj_t *maintanceScreen;
#endif

lv_obj_t *selectServiceDD;
lv_obj_t *selectCategoryDD;
lv_obj_t *passwordsShowTable;

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
	if (touchscreen.touched())
	{
		TS_Point p = touchscreen.getPoint();
		if (p.x < touchScreenMinimumX)
			touchScreenMinimumX = p.x;
		if (p.x > touchScreenMaximumX)
			touchScreenMaximumX = p.x;
		if (p.y < touchScreenMinimumY)
			touchScreenMinimumY = p.y;
		if (p.y > touchScreenMaximumY)
			touchScreenMaximumY = p.y;
		data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 1, TFT_HOR_RES);
		data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 1, TFT_VER_RES);
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

static void deviceUnlock(lv_event_t *e) // Device unlock
{
	const char *password = lv_textarea_get_text(passwordTa);
#ifdef DEV_AUTO_AUTH
	password = "0123456789012345";
#endif
	cipher->setKey((char *)password);

	// Load passswords
	File file = SPIFFS.open(PASSWORDS_FILE_PATH);

	if (!file)
	{
		Serial.printf("Fatal error; Can't read file\n");
		while (1)
		{
		}
	}

	if (file.available())
	{
		String fileContent = "";
		while (file.available())
			fileContent += (char)file.read();
		file.close();
		String decipheredString = cipher->decryptString(fileContent);

		DeserializationError err = deserializeJson(readedPasswords, decipheredString);
		if (!err)
		{
			// Iterate over categories
			JsonObject root = readedPasswords.as<JsonObject>();
			uint8_t index = 0;

			for (JsonPair kv : root)
			{
				strcat(categoriesList, kv.key().c_str());
				strcat(categoriesList, "\n");
			}
			categoriesList[strlen(categoriesList) - 1] = '\0';

			// Load UI
			getPasswordScreenLayout();
			lv_scr_load(getPasswordScreen);
		}
	}
	else
		file.close();
	
}

#ifdef SECURE_NOTES
static void readSecureNotes(){	

	File file = SPIFFS.open(SECURE_NOTES_FILE_PATH);
	if (file.available())
	{
		String fileContent = "";
		while (file.available())
			fileContent += (char)file.read();
		file.close();
		readedSecureNotes = cipher->decryptString(fileContent);
		lv_textarea_set_text(notesTa, readedSecureNotes.c_str());
	}
}
#endif

static void categorySelectHandler(lv_event_t *e)
{
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
	char buf[32];
	lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
	JsonObject services = readedPasswords[buf].as<JsonObject>();
	memset(&servicesList[0], 0, sizeof(servicesList));
	for (JsonPair kv : services)
	{
		strcat(servicesList, kv.key().c_str());
		strcat(servicesList, "\n");
	}
	if (strlen(servicesList) >= 1)
	{
		servicesList[strlen(servicesList) - 1] = '\0';
		lv_dropdown_set_options(selectServiceDD, servicesList);
	}

	// readedPasswords[buf]
	// Serial.printf("New category: %s", buf);
}

static void getPasswordsHandler(lv_event_t *event)
{
	char currCategory[32], currService[32];
	lv_dropdown_get_selected_str(selectCategoryDD, currCategory, sizeof(currCategory));
	lv_dropdown_get_selected_str(selectServiceDD, currService, sizeof(currService));

	JsonArray keyArray = readedPasswords[currCategory][currService];

	uint8_t countRows = 1;
	for (JsonArray arr : keyArray)
	{
		lv_table_set_cell_value(passwordsShowTable, countRows, 0, arr[0].as<const char *>()); // Login
		lv_table_set_cell_value(passwordsShowTable, countRows, 1, arr[1].as<const char *>()); // Password
		countRows++;
	}

	lv_scr_load(showPasswordScreen);

	// for (JsonObject elem : doc[currCategory][currService].as<JsonArray>()) {
	// 	// Serial.printf("%s\n", elem[0].c_str());
	// }
}

void getPasswordScreenLayout()
{
	lv_obj_t *text_label = lv_label_create(getPasswordScreen);
	lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
	lv_label_set_text(text_label, "Select service");
	lv_obj_set_width(text_label, 150);
	lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align(text_label, LV_ALIGN_CENTER, 0, -90);

	selectCategoryDD = lv_dropdown_create(getPasswordScreen);
	lv_dropdown_set_options(selectCategoryDD, categoriesList);

	lv_obj_align(selectCategoryDD, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_add_event_cb(selectCategoryDD, categorySelectHandler, LV_EVENT_VALUE_CHANGED, NULL);

	selectServiceDD = lv_dropdown_create(getPasswordScreen);
	lv_dropdown_set_options(selectServiceDD, servicesList);

	lv_obj_align(selectServiceDD, LV_ALIGN_RIGHT_MID, 0, 0);
	// lv_obj_add_event_cb(selectServiceDD, event_handler, LV_EVENT_ALL, NULL);

	lv_obj_t *getBtnLabel;
	lv_obj_t *getServicePasswords = lv_btn_create(getPasswordScreen);
	lv_obj_add_event_cb(getServicePasswords, getPasswordsHandler, LV_EVENT_CLICKED, NULL);
	lv_obj_align(getServicePasswords, LV_ALIGN_BOTTOM_MID, 0, -40); // LV_ALIGN_BOTTOM_MID

	getBtnLabel = lv_label_create(getServicePasswords);
	lv_label_set_text(getBtnLabel, "Get");
	lv_obj_center(getServicePasswords);

	lv_obj_t *addPassLabel;
	lv_obj_t *addCategoryLabel;
	lv_obj_t *openNotesLabel;

	lv_obj_t *addPassBtn = lv_btn_create(getPasswordScreen);
	// lv_obj_add_event_cb(gaddPassBtn, getPasswordsHandler, LV_EVENT_CLICKED, NULL);
	lv_obj_align(addPassBtn, LV_ALIGN_BOTTOM_LEFT, 10, -10); // LV_ALIGN_BOTTOM_MID
	addPassLabel = lv_label_create(addPassBtn);
	lv_label_set_text(addPassLabel, "Add pass");

	lv_obj_t *addCategoryBtn = lv_btn_create(getPasswordScreen);
	// lv_obj_add_event_cb(gaddPassBtn, getPasswordsHandler, LV_EVENT_CLICKED, NULL);
	lv_obj_align(addCategoryBtn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
	addCategoryLabel = lv_label_create(addCategoryBtn);
	lv_label_set_text(addCategoryLabel, "Add category");

#ifdef SECURE_NOTES
	lv_obj_t *openNotesBtn = lv_btn_create(getPasswordScreen);
	lv_obj_add_event_cb(openNotesBtn, openNotesScreen, LV_EVENT_CLICKED, NULL);
	lv_obj_align(openNotesBtn, LV_ALIGN_BOTTOM_MID, -15, -10);
	openNotesLabel = lv_label_create(openNotesBtn);
	lv_label_set_text(openNotesLabel, "Notes");
#endif
}

#ifdef SECURE_NOTES
void notesScreenLayout()
{
	notesTa = lv_textarea_create(notesScreen);
	lv_obj_align(notesTa, LV_ALIGN_TOP_MID, 0, 50);
	lv_obj_set_size(notesTa, lv_pct(90), lv_pct(70));
	lv_obj_add_state(notesTa, LV_STATE_FOCUSED);

	lv_obj_t *backBtnLabel;
	lv_obj_t *backBtn = lv_btn_create(notesScreen);
	lv_obj_add_event_cb(backBtn, backToPasswordsSelect, LV_EVENT_CLICKED, NULL);
	lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 10, 10);
	backBtnLabel = lv_label_create(backBtn);
	lv_label_set_text(backBtnLabel, "Back");
}
#endif

void backToPasswordsSelect(lv_event_t *event)
{
	lv_scr_load(getPasswordScreen);
}

#ifdef SECURE_NOTES
void openNotesScreen(lv_event_t *event)
{
	readSecureNotes();
	lv_scr_load(notesScreen);
}
#endif

void showPasswordsScreenLayout()
{

	passwordsShowTable = lv_table_create(showPasswordScreen);
	lv_table_set_cell_value(passwordsShowTable, 0, 0, "Login");
	lv_table_set_cell_value(passwordsShowTable, 0, 1, "Passwords");
	lv_obj_center(passwordsShowTable);

	lv_obj_t *backBtnLabel;

	lv_obj_t *backBtn = lv_btn_create(showPasswordScreen);
	lv_obj_add_event_cb(backBtn, backToPasswordsSelect, LV_EVENT_CLICKED, NULL);
	lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 10, 10);
	// lv_obj_set_pos(backBtn, 5, 5);

	backBtnLabel = lv_label_create(backBtn);
	lv_label_set_text(backBtnLabel, "Back");
}

#ifdef MAINTANCE_MODE
void maintanceScreenLayout()
{
	lv_obj_t *label2 = lv_label_create(maintanceScreen);
	lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(label2, 150);
	lv_label_set_text(label2, "Maintance Mode: Device controlled via USB UART; Send sf_reboot to exit this mode...");
	lv_obj_align(label2, LV_ALIGN_CENTER, 0, 0);
}
#endif

void setup()
{
	Serial.begin(115200);
	Serial.setTimeout(2000);

	// SPIFS Card

	if (!SPIFFS.begin(true))
	{
		Serial.printf("Error in SD card init\n");
		while (1)
		{
		}
	}
	else
		Serial.printf("SPIFS ready\n");

	// Touch screen

	touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
	touchscreen.begin(touchscreenSpi);
	touchscreen.setRotation(3);

	// Lvgl

	lv_init();
	draw_buf = new uint8_t[DRAW_BUF_SIZE];
	lv_display_t *disp;
	disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

	indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_touchpad_read);

	// Init Ok

	// Create screens
	getPasswordScreen = lv_obj_create(NULL);
	showPasswordScreen = lv_obj_create(NULL);
#ifdef SECURE_NOTES
	notesScreen = lv_obj_create(NULL);
#endif

#ifdef MAINTANCE_MODE
	maintanceScreen = lv_obj_create(NULL);
#endif

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
	lv_obj_add_event_cb(kb, deviceUnlock, LV_EVENT_READY, kb);

	passwordTa = lv_textarea_create(lv_screen_active());
	lv_obj_align(passwordTa, LV_ALIGN_TOP_MID, 0, 50);
	lv_obj_set_size(passwordTa, lv_pct(90), 80);
	lv_obj_add_state(passwordTa, LV_STATE_FOCUSED);
	lv_textarea_set_password_mode(passwordTa, true);
	lv_textarea_set_one_line(passwordTa, true);

	lv_keyboard_set_textarea(kb, passwordTa);
	showPasswordsScreenLayout();
#ifdef SECURE_NOTES
	notesScreenLayout();
#endif

#ifdef MAINTANCE_MODE
	maintanceScreenLayout();
#endif

	// Serial.printf("Setup done");
}

void loop()
{
	lv_tick_inc(millis() - lastTick);
	lastTick = millis();
	lv_timer_handler();
	delay(5);
#ifdef MAINTANCE_MODE
	if (Serial.available() > 3)
	{
		char serialBuffer[64];
		uint32_t byteNum = 0;
		while (Serial.available())
		{
			serialBuffer[byteNum] = Serial.read();
			byteNum++;
		}
		if (13 <= strlen(serialBuffer) && (strncmp("set_maintance", serialBuffer, 13) == 0))
		{
			lv_scr_load(maintanceScreen);
		}
		else if (9 <= strlen(serialBuffer) && (strncmp("sf_reboot", serialBuffer, 9) == 0))
		{
			abort();
		}
		else if (13 <= strlen(serialBuffer) && (strncmp("internal_dump", serialBuffer, 13) == 0))
		{
			// TODO; require device unlock
			// You can't dump decrypted passwords via API; You can only dump raw file contents
			File file = SPIFFS.open(PASSWORDS_FILE_PATH);
			if (file)
			{
				String fileContent = "";
				while (file.available())
					fileContent += (char)file.read();
				file.close();
				Serial.println(fileContent);
			}
			else
				Serial.printf("No file\n");
		}
		else if (13 <= strlen(serialBuffer) && (strncmp("fs_clear_init", serialBuffer, 13) == 0))
		{
			Serial.flush();
			Serial.printf("Enter command\n");
			String jsonInput = Serial.readStringUntil('\n');
			Serial.println(jsonInput);
			JsonDocument readedCommand;
			DeserializationError err = deserializeJson(readedCommand, jsonInput);
			if (!err)
			{
				const char *key = readedCommand["master_key"];
				cipher->setKey(key);

				String data = "{\"web\": {\"gmail\": [[\"login\", \"password\"], [\"login2\", \"password2\"]]}}\n";
				String cipherString = cipher->encryptString(data);

				File newFile = SPIFFS.open(PASSWORDS_FILE_PATH, FILE_WRITE);
				if (newFile.print(cipherString))
					Serial.printf("Passwords file create OK");
				newFile.close();
			}
		}
		else if (8 <= strlen(serialBuffer) && (strncmp("add_pass", serialBuffer, 8) == 0))
		{
			// You need to load passwords (unlock device)
			// Only then you can switch to maintance mode and add passwords
			// TODO Add password checking
			Serial.flush();
			Serial.printf("Enter command\n");
			String jsonInput = Serial.readStringUntil('\n');
			Serial.println(jsonInput);
			JsonDocument readedCommand;
			DeserializationError err = deserializeJson(readedCommand, jsonInput);
			if (!err)
			{
				if (!readedPasswords.isNull()) // TODO Fix checking
				{
				JsonArray arr = readedCommand["data"].as<JsonArray>();
				serializeJson(readedCommand, Serial);
				for (JsonObject value : arr)
				{
					const char *categoryName = value["category"];
					const char *serviceName = value["service"];

					// Service exists
					if (!readedPasswords[categoryName].containsKey(serviceName))
					{
						StaticJsonDocument<200> doc;
						JsonArray array = doc.to<JsonArray>();
						JsonArray nested = array.createNestedArray();
						nested.add(value["login"]);
						nested.add(value["password"]);
						readedPasswords[categoryName][serviceName] = array;
					}
					else
					{ // Service exists
						JsonArray serviceArray = readedPasswords[categoryName][serviceName];
						JsonArray newEntry = serviceArray.createNestedArray();
						newEntry.add(value["login"]);
						newEntry.add(value["password"]);
					}
				}
				serializeJson(readedPasswords, Serial);
				const char *key = readedCommand["master_key"];
				cipher->setKey((char *)key);

				String passwordsString;
				serializeJson(readedPasswords, passwordsString);
				Serial.printf("%s", passwordsString);

				String cipherString = cipher->encryptString(passwordsString);

				File newFile = SPIFFS.open(PASSWORDS_FILE_PATH, FILE_WRITE);
				if (newFile.print(cipherString))
					Serial.printf("Passwords file create OK");
				newFile.close();

				}
			}
		}
#ifdef SECURE_NOTES
		else if (11 <= strlen(serialBuffer) && (strncmp("write_notes", serialBuffer, 11) == 0))
		{
			// Command struct
			// {"master_key": key, "content": "some"}
			if (!readedPasswords.isNull())
			{ // If device unlocked
				Serial.flush();
				Serial.printf("Enter command\n");
				String jsonInput = Serial.readStringUntil('\n');
				JsonDocument readedCommand;
				DeserializationError err = deserializeJson(readedCommand, jsonInput);
				if (!err)
				{
					// Set cipher key
					const char *key = readedCommand["master_key"];
					cipher->setKey((char *)key);

					// Crypt
					String notesString = readedCommand["content"];
					Serial.println(notesString);
					String cipherString = cipher->encryptString(notesString);

					// Write
					File newFile = SPIFFS.open(SECURE_NOTES_FILE_PATH, FILE_WRITE);
					if (newFile.print(cipherString))
						Serial.printf("Notes write OK");
					newFile.close();
				}
			}
			else
				Serial.printf("Unlock device...");
			Serial.flush();
		}
		else if (10 <= strlen(serialBuffer) && (strncmp("read_notes", serialBuffer, 10) == 0))
		{
			if (!readedPasswords.isNull())
			{
				Serial.flush();
				Serial.printf("Enter master key\n");

				// Set key
				String jsonInput = Serial.readStringUntil('\n');
				JsonDocument readedCommand;
				DeserializationError err = deserializeJson(readedCommand, jsonInput);
				if (!err)
				{
					const char *key = readedCommand["master_key"];
					cipher->setKey((char *)key);

					// Decrypt
					File file = SPIFFS.open(SECURE_NOTES_FILE_PATH);
					if (file)
					{
						String fileContent = "";
						while (file.available())
							fileContent += (char)file.read();
						file.close();
						String decipheredString = cipher->decryptString(fileContent);
						Serial.println(decipheredString);
					}
				}
			} else
				Serial.printf("Unlock device...");
		}

#endif // SECEURE_NOTES
	}
#endif
}