#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <filesystem>
#include <fstream>

using namespace geode::prelude;

class DeathSoundManager {
public:
    static DeathSoundManager* get() {
        static DeathSoundManager instance;
        return &instance;
    }
    
    std::string soundFilePath = "";
    std::string soundFileName = "Default (explode_001.ogg)";
    float volume = 1.0f;
    float pitch = 1.0f;
    float speed = 1.0f;
    bool enabled = true;
    bool useCustom = false;
    
    void loadSettings() {
        auto mod = Mod::get();
        this->soundFilePath = mod->getSettingValue<std::string>("custom-sound-path");
        this->soundFileName = mod->getSettingValue<std::string>("sound-name");
        this->volume = mod->getSettingValue<double>("volume");
        this->pitch = mod->getSettingValue<double>("pitch");
        this->speed = mod->getSettingValue<double>("speed");
        this->enabled = mod->getSettingValue<bool>("enabled");
        this->useCustom = mod->getSettingValue<bool>("use-custom");
        
        if (!soundFilePath.empty() && std::filesystem::exists(soundFilePath)) {
            soundFileName = std::filesystem::path(soundFilePath).filename().string();
        }
    }
    
    void saveSettings() {
        auto mod = Mod::get();
        mod->setSettingValue("custom-sound-path", this->soundFilePath);
        mod->setSettingValue("sound-name", this->soundFileName);
        mod->setSettingValue("volume", this->volume);
        mod->setSettingValue("pitch", this->pitch);
        mod->setSettingValue("speed", this->speed);
        mod->setSettingValue("enabled", this->enabled);
        mod->setSettingValue("use-custom", this->useCustom);
    }
    
    bool loadSoundFromPath(const std::string& path) {
        if (path.empty()) return false;
        
        if (!std::filesystem::exists(path)) {
            Notification::create("File doesn't exist!", 
                CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
            return false;
        }
        
        auto fileSize = std::filesystem::file_size(path);
        if (fileSize > 5 * 1024 * 1024) {
            Notification::create("File too large! Max 5MB", 
                CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
            return false;
        }
        
        std::string ext = std::filesystem::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext != ".ogg" && ext != ".mp3" && ext != ".wav") {
            Notification::create("Must be .ogg, .mp3, or .wav!", 
                CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
            return false;
        }
        
        std::string modDir = Mod::get()->getConfigDir().string() + "/crazydeathsfx/";
        std::filesystem::create_directories(modDir);
        
        std::string filename = std::filesystem::path(path).filename().string();
        std::string destPath = modDir + filename;
        
        try {
            if (std::filesystem::exists(destPath)) {
                std::filesystem::remove(destPath);
            }
            
            std::filesystem::copy_file(path, destPath, 
                std::filesystem::copy_options::overwrite_existing);
            
            soundFilePath = destPath;
            soundFileName = filename;
            useCustom = true;
            
            Notification::create("Sound loaded successfully!", 
                CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png"))->show();
            return true;
            
        } catch (const std::exception& e) {
            Notification::create("Failed to copy file!", 
                CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
            return false;
        }
    }
    
    void playDeathSound() {
        if (!enabled) return;
        
        if (useCustom && !soundFilePath.empty() && std::filesystem::exists(soundFilePath)) {
            FMODAudioEngine::sharedEngine()->playEffect(
                soundFilePath,
                volume,
                pitch,
                speed
            );
        } else {
            FMODAudioEngine::sharedEngine()->playEffect(
                "explode_001.ogg",
                volume,
                pitch,
                speed
            );
        }
    }
};

class CrazyDeathSettingsLayer : public FLAlertLayer {
protected:
    CCMenu* m_buttonMenu;
    CCLabelBMFont* m_currentSoundLabel;
    
    bool setup() override {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({ 400.f, 280.f });
        bg->setPosition(winSize.width / 2, winSize.height / 2);
        this->addChild(bg);
        
        auto title = CCLabelBMFont::create("Crazy Death SFX", "bigFont.fnt");
        title->setPosition(winSize.width / 2, winSize.height / 2 + 120);
        title->setScale(0.7f);
        this->addChild(title);
        
        m_buttonMenu = CCMenu::create();
        m_buttonMenu->setPosition(0, 0);
        this->addChild(m_buttonMenu);
        
        auto manager = DeathSoundManager::get();
        
        this->createToggle("Enabled", manager->enabled, 
            menu_selector(CrazyDeathSettingsLayer::onToggleEnabled),
            ccp(winSize.width / 2 - 140, winSize.height / 2 + 80));
        
        this->createToggle("Custom Sound", manager->useCustom,
            menu_selector(CrazyDeathSettingsLayer::onToggleCustom),
            ccp(winSize.width / 2 - 140, winSize.height / 2 + 40));
        
        auto loadBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Load Sound File"),
            this,
            menu_selector(CrazyDeathSettingsLayer::onLoadSound)
        );
        loadBtn->setPosition(winSize.width / 2, winSize.height / 2 + 10);
        m_buttonMenu->addChild(loadBtn);
        
        m_currentSoundLabel = CCLabelBMFont::create(
            fmt::format("Current: {}", manager->soundFileName).c_str(),
            "chatFont.fnt"
        );
        m_currentSoundLabel->setPosition(winSize.width / 2, winSize.height / 2 - 25);
        m_currentSoundLabel->setScale(0.4f);
        m_currentSoundLabel->setColor({200, 200, 255});
        this->addChild(m_currentSoundLabel);
        
        this->createSlider("Volume", manager->volume,
            menu_selector(CrazyDeathSettingsLayer::onVolumeChanged),
            ccp(winSize.width / 2, winSize.height / 2 - 60));
        
        this->createSlider("Pitch", manager->pitch,
            menu_selector(CrazyDeathSettingsLayer::onPitchChanged),
            ccp(winSize.width / 2, winSize.height / 2 - 95));
        
        this->createSlider("Speed", manager->speed,
            menu_selector(CrazyDeathSettingsLayer::onSpeedChanged),
            ccp(winSize.width / 2, winSize.height / 2 - 130));
        
        auto testBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Test Sound"),
            this,
            menu_selector(CrazyDeathSettingsLayer::onTestSound)
        );
        testBtn->setPosition(winSize.width / 2 - 80, winSize.height / 2 - 170);
        m_buttonMenu->addChild(testBtn);
        
        auto closeBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
            this,
            menu_selector(CrazyDeathSettingsLayer::onClose)
        );
        closeBtn->setPosition(winSize.width / 2 + 180, winSize.height / 2 + 120);
        m_buttonMenu->addChild(closeBtn);
        
        return true;
    }
    
    void createToggle(const char* label, bool enabled, SEL_MenuHandler callback, CCPoint pos) {
        auto toggle = CCMenuItemToggler::createWithStandardSprites(
            this, callback, 0.8f
        );
        toggle->setPosition(pos);
        toggle->toggle(enabled);
        m_buttonMenu->addChild(toggle);
        
        auto labelText = CCLabelBMFont::create(label, "goldFont.fnt");
        labelText->setPosition(pos.x + 70, pos.y);
        labelText->setScale(0.5f);
        this->addChild(labelText);
    }
    
    void createSlider(const char* label, float value, SEL_MenuHandler callback, CCPoint pos) {
        auto slider = Slider::create(this, callback, 0.8f);
        slider->setPosition(pos);
        slider->setValue(value);
        m_buttonMenu->addChild(slider);
        
        auto labelText = CCLabelBMFont::create(label, "goldFont.fnt");
        labelText->setPosition(pos.x - 80, pos.y);
        labelText->setScale(0.45f);
        this->addChild(labelText);
    }
    
    void onToggleEnabled(CCObject* sender) {
        auto toggle = static_cast<CCMenuItemToggler*>(sender);
        DeathSoundManager::get()->enabled = toggle->isOn();
    }
    
    void onToggleCustom(CCObject* sender) {
        auto toggle = static_cast<CCMenuItemToggler*>(sender);
        DeathSoundManager::get()->useCustom = toggle->isOn();
        
        if (!toggle->isOn()) {
            DeathSoundManager::get()->soundFileName = "Default (explode_001.ogg)";
            m_currentSoundLabel->setString(
                fmt::format("Current: {}", DeathSoundManager::get()->soundFileName).c_str()
            );
        }
    }
    
    void onLoadSound(CCObject*) {
        geode::createQuickPopup(
            "Load Sound File",
            "Enter full path to your sound file:\nExample: /storage/emulated/0/Download/mysound.ogg",
            "Cancel", "OK",
            [this](auto, bool btn2) {
                if (btn2) {
                    auto input = geode::createTextInput("File Path");
                    if (input && !input->getString().empty()) {
                        std::string path = input->getString();
                        if (DeathSoundManager::get()->loadSoundFromPath(path)) {
                            m_currentSoundLabel->setString(
                                fmt::format("Current: {}", DeathSoundManager::get()->soundFileName).c_str()
                            );
                        }
                    }
                }
            }
        );
    }
    
    void onVolumeChanged(CCObject* sender) {
        auto slider = static_cast<Slider*>(sender);
        DeathSoundManager::get()->volume = slider->getValue();
    }
    
    void onPitchChanged(CCObject* sender) {
        auto slider = static_cast<Slider*>(sender);
        DeathSoundManager::get()->pitch = slider->getValue();
    }
    
    void onSpeedChanged(CCObject* sender) {
        auto slider = static_cast<Slider*>(sender);
        DeathSoundManager::get()->speed = slider->getValue();
    }
    
    void onTestSound(CCObject*) {
        DeathSoundManager::get()->playDeathSound();
    }
    
    void onClose(CCObject*) {
        DeathSoundManager::get()->saveSettings();
        this->removeFromParentAndCleanup(true);
    }
    
public:
    static CrazyDeathSettingsLayer* create() {
        auto ret = new CrazyDeathSettingsLayer();
        if (ret && ret->init(400.f, 280.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(PlayerObject) {
    void playerDestroyed(bool p0) {
        DeathSoundManager::get()->playDeathSound();
        PlayerObject::playerDestroyed(p0);
    }
};

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        
        DeathSoundManager::get()->loadSettings();
        
        auto settingsBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Death SFX"),
            this,
            menu_selector(MenuLayer_mod::onSettings)
        );
        
        auto rightMenu = this->getChildByID("right-side-menu");
        if (rightMenu) {
            rightMenu->addChild(settingsBtn);
            rightMenu->updateLayout();
        }
        
        return true;
    }
    
    void onSettings(CCObject*) {
        auto settings = CrazyDeathSettingsLayer::create();
        CCDirector::sharedDirector()->getRunningScene()->addChild(settings);
    }
};

$on_mod(Loaded) {
    auto mod = Mod::get();
    
    mod->addCustomSetting<Setting>("custom-sound-path", "", "Custom Sound Path");
    mod->addCustomSetting<Setting>("sound-name", "Default (explode_001.ogg)", "Sound Name");
    mod->addCustomSetting<Setting>("volume", 1.0, "Volume");
    mod->addCustomSetting<Setting>("pitch", 1.0, "Pitch");
    mod->addCustomSetting<Setting>("speed", 1.0, "Speed");
    mod->addCustomSetting<Setting>("enabled", true, "Enabled");
    mod->addCustomSetting<Setting>("use-custom", false, "Use Custom Sound");
    
    DeathSoundManager::get()->loadSettings();
    
    std::string soundDir = Mod::get()->getConfigDir().string() + "/crazydeathsfx/";
    std::filesystem::create_directories(soundDir);
}
