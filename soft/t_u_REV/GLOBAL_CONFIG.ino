#include "util/util_settings.h"
#include "TU_apps.h"
#include "TU_menus.h"
#include "TU_digital_inputs.h"

namespace menu = TU::menu;

static const uint8_t global_divisors[] {
  0,
  24,
  48,
  96
};

enum GLOBAL_CONFIG_Setting {
  GLOBAL_CONFIG_SETTING_DIV1,
  GLOBAL_CONFIG_SETTING_SLAVE_TR1,
  GLOBAL_CONFIG_SETTING_MORE_DUMMY,
  GLOBAL_CONFIG_SETTING_STILL_MORE_DUMMY,
  GLOBAL_CONFIG_SETTING_LAST
};

class Global_Config : public settings::SettingsBase<Global_Config, GLOBAL_CONFIG_SETTING_LAST> {
public:

  void Init() {
    
    InitDefaults();
    global_div1_ = 0x0;
    TR1_master_ = 0x0;
    ticks_ = 0x0;
    update_enabled_settings();
  }

  uint8_t global_div1() const {
    return values_[GLOBAL_CONFIG_SETTING_DIV1];
  }

  bool TR1_master() const {
    return values_[GLOBAL_CONFIG_SETTING_SLAVE_TR1];  
  }
  
  uint32_t ticks() const {
    return ticks_;
  }

  void Update() {

    ticks_++;
    
    if (global_div1_ != global_div1()) {
        TU::DigitalInputs inputs; 
        inputs.set_global_div_TR1(global_divisors[global_div1()]);
        global_div1_ = global_div1();
    }

    if (TR1_master_ != TR1_master()) {
        TU::DigitalInputs inputs;
        inputs.set_master_clock(TR1_master());
        TR1_master_ = TR1_master();
    }
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  GLOBAL_CONFIG_Setting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {
    
    GLOBAL_CONFIG_Setting *settings = enabled_settings_;
    
    *settings++ = GLOBAL_CONFIG_SETTING_DIV1;
    *settings++ = GLOBAL_CONFIG_SETTING_SLAVE_TR1;
    *settings++ = GLOBAL_CONFIG_SETTING_MORE_DUMMY;
    *settings++ = GLOBAL_CONFIG_SETTING_STILL_MORE_DUMMY;
    
    num_enabled_settings_ = settings - enabled_settings_;
  }

  void restore() {
    
    TU::DigitalInputs inputs;
    uint8_t _div = 0;
    
    switch (inputs.global_div_TR1()) {
      case 24:
      _div = 1;
      break;
      case 48:
      _div = 2;
      break;
      case 96:
      _div = 3;
      break;
      default:
      break;
    }
    apply_value(GLOBAL_CONFIG_SETTING_DIV1, _div);
    apply_value(GLOBAL_CONFIG_SETTING_SLAVE_TR1, inputs.master_clock());
  }

  void RenderScreensaver() const;

private:
  int num_enabled_settings_;
  uint8_t global_div1_;
  bool TR1_master_;
  uint32_t ticks_;
  GLOBAL_CONFIG_Setting enabled_settings_[GLOBAL_CONFIG_SETTING_LAST];
};

//////////////////////////////////////////////////////////////////////// 

const char* const g_divisors[CHANNEL_TRIGGER_LAST] = {
  "-", "24PPQ", "48PPQ", "96PPQ"
};


SETTINGS_DECLARE(Global_Config,  GLOBAL_CONFIG_SETTING_LAST) {
  
  { 0, 0, 3, "TR1 global div", g_divisors, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "TR1 master", TU::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 0, "-", nullptr, settings::STORAGE_TYPE_U4 },
  { 0, 0, 0, "-", nullptr, settings::STORAGE_TYPE_U4 }
};

class Config_App {
public:

  void Init() {
    cursor.Init(0, GLOBAL_CONFIG_SETTING_LAST - 1);
  }

  inline bool editing() const {
    return cursor.editing();
  }

  inline int cursor_pos() const {
    return cursor.cursor_pos();
  }

  menu::ScreenCursor<menu::kScreenLines> cursor;
  menu::ScreenCursor<menu::kScreenLines> cursor_state;
};

Global_Config global_config;
Config_App config_app;

// stubs

void GLOBAL_CONFIG_init() {
  
  global_config.Init();
  config_app.Init();
  global_config.update_enabled_settings();
  config_app.cursor.AdjustEnd(global_config.num_enabled_settings() - 1);
}

size_t GLOBAL_CONFIG_storageSize() {
  return 0;
}

size_t GLOBAL_CONFIG_save(void *) {
  return 0;
}

size_t GLOBAL_CONFIG_restore(const void *) {
  return 0;
}

void GLOBAL_CONFIG_isr() {
   global_config.Update();
}

void GLOBAL_CONFIG_handleAppEvent(TU::AppEvent event) {

  switch (event) {
    case TU::APP_EVENT_RESUME:
      global_config.restore();
      config_app.cursor.set_editing(false);
      global_config.update_enabled_settings();
      break;
    case TU::APP_EVENT_SUSPEND:
    case TU::APP_EVENT_SCREENSAVER_ON:
    case TU::APP_EVENT_SCREENSAVER_OFF:
      break;
    default:
      break;  
  }
}

void GLOBAL_CONFIG_loop() {  
}

void GLOBAL_CONFIG_menu() {
  
  menu::TitleBar<0, 4, 0>::Draw();
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(config_app.cursor);
  menu::SettingsListItem list_item;
  graphics.print("global input config.");

  while (settings_list.available()) {
    const int setting = global_config.enabled_setting_at(settings_list.Next(list_item));
    const int value = global_config.get_value(setting);
    const settings::value_attr &attr = Global_Config::value_attr(setting);

    switch (setting) {
      case GLOBAL_CONFIG_SETTING_MORE_DUMMY:
      case GLOBAL_CONFIG_SETTING_STILL_MORE_DUMMY:
        list_item.DrawNoValue<false>(value, attr);
      break;
      default:
        list_item.DrawDefault(value, attr);
      break;
    }
  } 
}

void Global_Config::RenderScreensaver() const {
  
   uint8_t _size = (ticks() >> 7) % 120;
   graphics.drawRect(63 - (_size >> 1), 31 - (_size >> 2), _size, _size >> 1);  
}

void GLOBAL_CONFIG_screensaver() {
  global_config.RenderScreensaver();
}

void GLOBAL_CONFIG_handleButtonEvent(const UI::Event &event) {

  if (TU::CONTROL_BUTTON_R == event.control) {

      GLOBAL_CONFIG_Setting setting = global_config.enabled_setting_at(config_app.cursor_pos());   
      if (setting == GLOBAL_CONFIG_SETTING_DIV1 || setting == GLOBAL_CONFIG_SETTING_SLAVE_TR1)
        config_app.cursor.toggle_editing();
  }
}

void GLOBAL_CONFIG_handleEncoderEvent(const UI::Event &event) {

  if (TU::CONTROL_ENCODER_R == event.control) {
    
    if (config_app.cursor.editing()) {
         
          GLOBAL_CONFIG_Setting setting = global_config.enabled_setting_at(config_app.cursor_pos());
          global_config.change_value(setting, event.value);
     }
     else {
          config_app.cursor.Scroll(event.value);
     }
  }
}
