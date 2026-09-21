#include "HealthBar.h"
#include <flixel/FlxG.h>

HealthBar::HealthBar(float x, float y, flixel::FlxCamera* camera)
    : health(1.0f), camera(camera)
{
    background = new flixel::FlxSprite();
    background->makeGraphic(601, 19, {0, 0, 0, 255});
    background->setPosition(x, y);
    background->scrollFactor.x = 0.0f;
    background->scrollFactor.y = 0.0f;
    if (camera) background->camera = camera;
    
    redBar = new flixel::FlxSprite();
    redBar->makeGraphic(593, 11, {175, 102, 206, 255});
    redBar->setPosition(background->x + 4, background->y + 4);
    redBar->scrollFactor.x = 0.0f;
    redBar->scrollFactor.y = 0.0f;
    if (camera) redBar->camera = camera;
    
    greenBar = new flixel::FlxSprite();
    greenBar->makeGraphic(593, 11, {49, 176, 209, 255});
    greenBar->setPosition(background->x + 4, background->y + 4);
    greenBar->scrollFactor.x = 0.0f;
    greenBar->scrollFactor.y = 0.0f;
    if (camera) greenBar->camera = camera;
    
    iconP1 = nullptr;
    iconP2 = nullptr;
}

HealthBar::~HealthBar() {
    if (background) delete background;
    if (redBar) delete redBar;
    if (greenBar) delete greenBar;
    if (iconP1) delete iconP1;
    if (iconP2) delete iconP2;
}

void HealthBar::setHealth(float newHealth) {
    health = newHealth;
    if (health > 2.0f) health = 2.0f;
    if (health < 0.0f) health = 0.0f;
}

void HealthBar::setIcons(const std::string& player, const std::string& opponent) {
    if (iconP1) delete iconP1;
    if (iconP2) delete iconP2;
    
    iconP1 = new HealthIcon(player, true);
    iconP1->y = background->y - 75;
    iconP1->scrollFactor.x = 0.0f;
    iconP1->scrollFactor.y = 0.0f;
    if (camera) iconP1->camera = camera;
    
    iconP2 = new HealthIcon(opponent, false);
    iconP2->y = background->y - 75;
    iconP2->scrollFactor.x = 0.0f;
    iconP2->scrollFactor.y = 0.0f;
    if (camera) iconP2->camera = camera;
}

void HealthBar::configureIcon(int character, const std::string& iconId, bool shouldBop,
                              float scale, bool flipX, bool isPixel, float offsetX, float offsetY) {
    HealthIcon* icon = nullptr;
    if (character == 0) {
        icon = iconP1;
    } else if (character == 1) {
        icon = iconP2;
    }

    if (icon) {
        icon->configure(iconId, shouldBop, scale, flipX, isPixel, offsetX, offsetY);
        if (camera) {
            icon->camera = camera;
        }
    }
}

void HealthBar::setColors(int player1R, int player1G, int player1B, int player2R, int player2G, int player2B) {
    if (greenBar) {
        greenBar->makeGraphic(593, 11, {static_cast<uint8_t>(player1R), static_cast<uint8_t>(player1G), static_cast<uint8_t>(player1B), 255});
    }
    
    if (redBar) {
        redBar->makeGraphic(593, 11, {static_cast<uint8_t>(player2R), static_cast<uint8_t>(player2G), static_cast<uint8_t>(player2B), 255});
    }
}

void HealthBar::update(float elapsed) {
    if (!greenBar || !redBar || !iconP1 || !iconP2) return;
    
    float healthPercent = (health / 2.0f) * 100.0f;
    float barWidth = 593.0f;
    
    float greenWidth = (healthPercent / 100.0f) * barWidth;
    greenBar->setScale((healthPercent / 100.0f), 1.0f);
    greenBar->x = background->x + 4 + barWidth - greenWidth;
    
    int iconOffset = 26;
    float iconX = background->x + 4 + barWidth - greenWidth;
    iconP1->x = iconX + (150 * iconP1->scale.x - 150) / 2 - iconOffset;
    iconP2->x = iconX - (150 * iconP2->scale.x) / 2 - iconOffset * 2;
    
    iconP1->update(elapsed);
    iconP2->update(elapsed);
    
    int iconWidth = iconP1->fullIconWidth / 2;
    if (healthPercent <= 20.0f) {
        iconP1->sourceRect.x = iconWidth;
    } else {
        iconP1->sourceRect.x = 0;
    }
    
    iconWidth = iconP2->fullIconWidth / 2;
    if (healthPercent >= 80.0f) {
        iconP2->sourceRect.x = iconWidth;
    } else {
        iconP2->sourceRect.x = 0;
    }
}

void HealthBar::draw() {
    if (background && background->visible) background->draw();
    if (redBar && redBar->visible) redBar->draw();
    if (greenBar && greenBar->visible) greenBar->draw();
    if (iconP1 && iconP1->visible) iconP1->draw();
    if (iconP2 && iconP2->visible) iconP2->draw();
}
