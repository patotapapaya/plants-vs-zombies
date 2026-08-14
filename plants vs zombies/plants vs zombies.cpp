#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#include <windows.h>

enum class ZombieType { NORMAL, FAST, TOUGH };
enum class PlantType {
    NONE,
    SHOOTER,
    WALL,
    SLOWER
};
enum class GameState { MENU, LEVEL_SELECT, PLAY };

// Создание текстур для резерва
static sf::Texture createTexture(int w, int h, sf::Color color) {
    sf::Image img({ static_cast<unsigned>(w), static_cast<unsigned>(h) }, color);
    sf::Texture tex;
    tex.loadFromImage(img);
    return tex;
}

class Zombie {
public:
    sf::Sprite sprite;
    float health;
    float speed;
    bool isDead;
    ZombieType type;
    float slowTimer;
    float originalSpeed;

    Zombie(ZombieType t, sf::Texture& texture, float startX, float startY)
        : sprite(texture), type(t), isDead(false), slowTimer(0.0f),
        health(3.0f), speed(-35.0f), originalSpeed(-35.0f)
    {
        sf::Vector2u texSize = texture.getSize();

        switch (t) {
        case ZombieType::NORMAL:
            health = 3.0f;
            speed = -35.0f;
            originalSpeed = -35.0f;
            sprite.setScale({ 0.2f, 0.2f });
            break;
        case ZombieType::FAST:
            health = 2.0f;
            speed = -65.0f;
            originalSpeed = -65.0f;
            sprite.setScale({ 0.2f, 0.2f });
            break;
        case ZombieType::TOUGH:
            health = 8.0f;
            speed = -20.0f;
            originalSpeed = -20.0f;
            sprite.setScale({ 0.2f, 0.2f });
            break;
        }

        // Центрируем спрайт
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin({ bounds.size.x / 2, bounds.size.y / 2 });

        // Ставим в центр клетки
        sprite.setPosition({ startX + 50, startY + 50 });

        // Используем цветовую замену только для резервных текстур
        if (texSize.x == 64 || texSize.x == 80) { 
            switch (t) {
            case ZombieType::NORMAL:
                sprite.setColor(sf::Color(180, 180, 180));
                break;
            case ZombieType::FAST:
                sprite.setColor(sf::Color(255, 80, 80));
                break;
            case ZombieType::TOUGH:
                sprite.setColor(sf::Color(160, 100, 40));
                break;
            }
        }
    }

    void update(float deltaTime) {
        if (!isDead) {
            if (slowTimer > 0) {
                speed = originalSpeed * 0.5f;
                slowTimer -= deltaTime;
                sprite.setColor(sf::Color(150, 200, 255));
            }
            else {
                speed = originalSpeed;
                sprite.setColor(sf::Color::White);
            }
            sprite.move({ speed * deltaTime, 0.0f });
        }
    }

    void takeDamage(float dmg) {
        health -= dmg;
        if (health <= 0) isDead = true;
    }

    void slow(float duration) { slowTimer = duration; }
    float getX() const { return sprite.getPosition().x; }
    float getY() const { return sprite.getPosition().y; }
};

class Plant {
public:
    sf::Sprite sprite;
    PlantType type;
    float shootTimer;
    float slowTimer;
    bool isAlive;
    float health;
    float gridX;
    float gridY;

    Plant(PlantType t, sf::Texture& texture, float posX, float posY)
        : sprite(texture), type(t), shootTimer(0.0f), slowTimer(0.0f), isAlive(true), health(3.0f), gridX(posX), gridY(posY)
    {
        sprite.setPosition({ posX, posY });

        switch (t) {
        case PlantType::SHOOTER:
            sprite.setScale({ 0.12f, 0.12f });
            sprite.setOrigin({ sprite.getLocalBounds().size.x / 2, sprite.getLocalBounds().size.y / 2 });
            sprite.setPosition({ posX + 50, posY + 50 });
            break;
        case PlantType::WALL:
            health = 10.0f;
            sprite.setScale({ 0.14f, 0.14f });
            sprite.setOrigin({ sprite.getLocalBounds().size.x / 2, sprite.getLocalBounds().size.y / 2 });
            sprite.setPosition({ posX + 50, posY + 50 });
            break;
        case PlantType::SLOWER:
            sprite.setScale({ 0.15f, 0.15f });
            sprite.setOrigin({ sprite.getLocalBounds().size.x / 2, sprite.getLocalBounds().size.y / 2 });
            sprite.setPosition({ posX + 50, posY + 50 });
            break;
        }
    }

    void update(float deltaTime) {
        if (shootTimer > 0) shootTimer -= deltaTime;
        if (slowTimer > 0) slowTimer -= deltaTime;
        if (type == PlantType::WALL && health <= 0) isAlive = false;
    }

    bool canShoot() const { return type == PlantType::SHOOTER && shootTimer <= 0.0f; }
    void resetShoot() { shootTimer = 0.7f; }
    bool canSlow() const { return type == PlantType::SLOWER && slowTimer <= 0.0f; }
    void resetSlow() { slowTimer = 1.5f; }

    float getX() const { return sprite.getPosition().x; }
    float getY() const { return sprite.getPosition().y; }
};

class Bullet {
public:
    sf::Sprite sprite;
    bool isActive;
    bool isSlow;

    Bullet(sf::Texture& texture, float startX, float startY, bool slow = false)
        : sprite(texture), isActive(true), isSlow(slow)
    {
        // Центрируем пулю
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin({ bounds.size.x / 2, bounds.size.y / 2 });
        sprite.setPosition({ startX, startY });
        sprite.setScale({ 0.5f, 0.5f });
        if (slow) sprite.setColor(sf::Color(100, 200, 255));
    }

    void update(float deltaTime) {
        sprite.move({ 400.0f * deltaTime, 0.0f });
        if (sprite.getPosition().x > 950.0f) isActive = false;
    }

    float getX() const { return sprite.getPosition().x; }
    float getY() const { return sprite.getPosition().y; }
};

struct SaveData {
    int level = 1;
    int score = 0;
    int highestLevel = 1;
};
std::string findPic(const std::string& name) {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exePath(buffer);
    std::string exeDir = exePath.substr(0, exePath.find_last_of("\\/") + 1);

    std::string fullPath = exeDir + "assets/" + name;

    DWORD attr = GetFileAttributesA(fullPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return fullPath;
    }

    return "";
}

int main() {
    GameState currentState = GameState::MENU;

    sf::Font font;
    std::string fontPath = findPic("Sonic_1_Title_Screen_Filled.ttf");
    if (!font.openFromFile(fontPath)) {
        return -1;
    }
    srand(static_cast<unsigned>(time(nullptr)));

    sf::Texture menuBgTexture;
    std::string menuBgPath = findPic("menu_background.jpg");
    if (!menuBgTexture.loadFromFile(menuBgPath)) {
        menuBgTexture = createTexture(1000, 700, sf::Color::Black);
    }
    sf::Sprite menuBgSprite(menuBgTexture);

    sf::RenderWindow window(sf::VideoMode({ 1000, 700 }), "Zombies vs Plants");
    window.setFramerateLimit(60);

    // Растягиваем фон меню
    sf::Vector2u menuTexSize = menuBgTexture.getSize();
    sf::Vector2u menuWinSize = window.getSize();
    float menuScaleX = static_cast<float>(menuWinSize.x) / menuTexSize.x;
    float menuScaleY = static_cast<float>(menuWinSize.y) / menuTexSize.y;
    float menuScale = std::max(menuScaleX, menuScaleY);
    menuBgSprite.setScale({ menuScale, menuScale });
    sf::FloatRect menuBounds = menuBgSprite.getGlobalBounds();
    float menuOffsetX = (menuWinSize.x - menuBounds.size.x) / 2.0f;
    float menuOffsetY = (menuWinSize.y - menuBounds.size.y) / 2.0f;
    menuBgSprite.setPosition({ menuOffsetX, menuOffsetY });

    sf::Texture levelsBgTexture;
    std::string levelsBgPath = findPic("cemetery_bg.jpg");
    if (!levelsBgTexture.loadFromFile(levelsBgPath)) {
        levelsBgTexture = createTexture(1000, 700, sf::Color(30, 20, 50));
    }
    sf::Sprite levelsBgSprite(levelsBgTexture);

    sf::Vector2u texSize = levelsBgTexture.getSize();
    sf::Vector2u winSize = window.getSize();

    float scaleX = static_cast<float>(winSize.x) / texSize.x;
    float scaleY = static_cast<float>(winSize.y) / texSize.y;
    float scale = std::max(scaleX, scaleY);

    levelsBgSprite.setScale({ scale, scale });

    sf::FloatRect bounds = levelsBgSprite.getGlobalBounds();
    float offsetX = (winSize.x - bounds.size.x) / 2.0f;
    float offsetY = (winSize.y - bounds.size.y) / 2.0f;
    levelsBgSprite.setPosition({ offsetX, offsetY });

    sf::Texture gameBgTexture;
    std::string gameBgPath = findPic("game_bg.png");
    if (!gameBgTexture.loadFromFile(gameBgPath)) {
        gameBgTexture = createTexture(1000, 700, sf::Color(34, 139, 34));
    }
    sf::Sprite gameBgSprite(gameBgTexture);

    // Растягиваем фон игры
    sf::Vector2u gameTexSize = gameBgTexture.getSize();
    sf::Vector2u gameWinSize = window.getSize();
    float gameScaleX = static_cast<float>(gameWinSize.x) / gameTexSize.x;
    float gameScaleY = static_cast<float>(gameWinSize.y) / gameTexSize.y;
    gameBgSprite.setScale({ gameScaleX, gameScaleY });
    gameBgSprite.setPosition({ 0, 0 });

    // Загрузка текстур с заглушками
    sf::Texture shooterTex;
    std::string shooterPath = findPic("shooter.png");
    if (!shooterTex.loadFromFile(shooterPath)) {
        shooterTex = createTexture(100, 300, sf::Color(50, 200, 50));
    }

    sf::Texture wallTex;
    std::string wallPath = findPic("wall.png");
    if (!wallTex.loadFromFile(wallPath)) {
        wallTex = createTexture(150, 150, sf::Color(80, 80, 255));
    }

    sf::Texture slowerTex;
    std::string slowerPath = findPic("slower.png");
    if (!slowerTex.loadFromFile(slowerPath)) {
        slowerTex = createTexture(100, 300, sf::Color(255, 255, 80));
    }
    // Текстуры зомби с заглушками
    sf::Texture zombieNormTex;
    std::string zombieNormPath = findPic("zombie.png");
    if (!zombieNormTex.loadFromFile(zombieNormPath)) {
        zombieNormTex = createTexture(250, 250, sf::Color(180, 180, 180));
    }

    sf::Texture zombieFastTex;
    std::string zombieFastPath = findPic("zombie_fast.png");
    if (!zombieFastTex.loadFromFile(zombieFastPath)) {
        zombieFastTex = createTexture(200, 200, sf::Color(255, 80, 80));
    }

    sf::Texture zombieToughTex;
    std::string zombieToughPath = findPic("zombie_tough.png");
    if (!zombieToughTex.loadFromFile(zombieToughPath)) {
        zombieToughTex = createTexture(300, 300, sf::Color(160, 100, 40));
    }

    // Текстура лопаты
    sf::Texture shovelTex;
    std::string shovelPath = findPic("shovel.png");
    if (!shovelTex.loadFromFile(shovelPath)) {
        shovelTex = createTexture(32, 32, sf::Color(200, 150, 100));
    }
    sf::Sprite shovelSprite(shovelTex);
    shovelSprite.setScale({ 0.5f, 0.5f });

    //уровни
    sf::Texture levelButtonTex;
    std::string levelButtonPath = findPic("button_lvl.png");
    if (!levelButtonTex.loadFromFile(levelButtonPath)) {
        levelButtonTex = createTexture(80, 80, sf::Color(150, 50, 200));
    }

    // Текстура пули
    sf::Texture bulletTex = createTexture(20, 20, sf::Color(255, 255, 0));

    // Дом
    sf::RectangleShape house(sf::Vector2f(80.0f, 150.0f));
    house.setFillColor(sf::Color(160, 82, 45));
    house.setPosition({ 20.0f, 280.0f });

    // Игровые объекты
    std::vector<Zombie> zombies;
    std::vector<Plant> plants;
    std::vector<Bullet> bullets;

    // Состояние игры
    SaveData save;
    std::ifstream loadFile("save.dat", std::ios::binary);
    if (loadFile.is_open()) {
        loadFile.read(reinterpret_cast<char*>(&save), sizeof(save));
        loadFile.close();
    }

    int score = save.score;
    int currentLevel = save.highestLevel;
    int zombiesToKill = 5 + currentLevel * 2;
    int zombiesKilled = 0;
    bool waveActive = true;
    bool gameOver = false;
    bool levelComplete = false;
    bool isShovelActive = false;
    PlantType selectedPlant = PlantType::NONE;
    int pressedLevel = -1;

    sf::Clock gameClock;
    sf::Clock spawnClock;

    // Выделение выбранного растения
    sf::RectangleShape selector(sf::Vector2f(30.0f, 30.0f));
    selector.setFillColor(sf::Color::Transparent);
    selector.setOutlineColor(sf::Color::White);
    selector.setOutlineThickness(2.0f);

    // Основной цикл
    while (window.isOpen()) {
        float deltaTime = gameClock.restart().asSeconds();
        if (deltaTime > 0.033f) deltaTime = 0.033f;

        // СОБЫТИЯ
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                std::ofstream saveFile("save.dat", std::ios::binary);
                if (saveFile.is_open()) {
                    saveFile.write(reinterpret_cast<char*>(&save), sizeof(save));
                    saveFile.close();
                }
                window.close();
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Enter && currentState == GameState::MENU) {
                    currentState = GameState::LEVEL_SELECT;
                }
                if (key->code == sf::Keyboard::Key::L) {
                    isShovelActive = !isShovelActive;
                }
                if (key->code == sf::Keyboard::Key::Num1) selectedPlant = PlantType::SHOOTER;
                if (key->code == sf::Keyboard::Key::Num2) selectedPlant = PlantType::WALL;
                if (key->code == sf::Keyboard::Key::Num3) selectedPlant = PlantType::SLOWER;

                if (key->code == sf::Keyboard::Key::R && gameOver) {
                    zombies.clear();
                    plants.clear();
                    bullets.clear();
                    score = 0;
                    currentLevel = 1;
                    zombiesToKill = 5 + currentLevel * 2;
                    zombiesKilled = 0;
                    gameOver = false;
                    levelComplete = false;
                    waveActive = true;

                    save.score = score;
                    save.highestLevel = currentLevel;
                    save.level = currentLevel;
                }

                if (key->code == sf::Keyboard::Key::N && levelComplete && currentLevel < 5) {
                    currentLevel++;
                    zombiesToKill = 5 + currentLevel * 2;
                    zombiesKilled = 0;
                    levelComplete = false;
                    waveActive = true;
                    save.highestLevel = currentLevel;
                    save.score = score;
                    save.level = currentLevel;
                    std::ofstream sf("save.dat", std::ios::binary);
                    if (sf.is_open()) sf.write(reinterpret_cast<char*>(&save), sizeof(save));
                }

                if (key->code == sf::Keyboard::Key::S) {
                    save.score = score;
                    save.highestLevel = currentLevel;
                    save.level = currentLevel;
                    std::ofstream sf("save.dat", std::ios::binary);
                    if (sf.is_open()) sf.write(reinterpret_cast<char*>(&save), sizeof(save));
                }
            }

            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse->button == sf::Mouse::Button::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                    // Проверяем, кликнули ли по лопате
                    sf::FloatRect shovelBounds = shovelSprite.getGlobalBounds();
                    if (shovelBounds.contains(static_cast<sf::Vector2f>(mousePos))) {
                        isShovelActive = !isShovelActive;
                    }

                    // ВЫБОР УРОВНЯ
                    if (currentState == GameState::LEVEL_SELECT) {
                        float positionsX[5] = { 150, 400, 650, 275, 525 };
                        float positionsY[5] = { 200, 200, 200, 350, 350 };
                        for (int i = 0; i < 5; i++) {
                            sf::Sprite button(levelButtonTex);
                            button.setScale({ 0.13f, 0.13f });
                            button.setPosition({ positionsX[i], positionsY[i] });

                            if (button.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                                currentLevel = i + 1;
                                zombiesToKill = 5 + currentLevel * 2;
                                zombiesKilled = 0;
                                waveActive = true;
                                gameOver = false;
                                levelComplete = false;
                                currentState = GameState::PLAY;
                                break;
                            }
                        }

                        // Кнопка "Назад"
                        sf::FloatRect backRect;
                        backRect.position = sf::Vector2f(450, 580);
                        backRect.size = sf::Vector2f(120, 40);
                        if (backRect.contains(static_cast<sf::Vector2f>(mousePos))) {
                            currentState = GameState::MENU;
                        }
                    }
                    if (currentState != GameState::PLAY)
                        continue;

                    // Удаление лопатой
                    if (isShovelActive && currentState == GameState::PLAY) {
                        for (auto it = plants.begin(); it != plants.end(); ++it) {
                            sf::FloatRect plantBounds = it->sprite.getGlobalBounds();
                            if (plantBounds.contains(static_cast<sf::Vector2f>(mousePos))) {
                                it->isAlive = false;
                                break;
                            }
                        }
                    }

                    // Посадка растений
                    if (currentState == GameState::PLAY && !gameOver && !levelComplete && waveActive) {
                        int gx = (mousePos.x / 100) * 100;
                        int gy = (mousePos.y / 100) * 100;

                        if (gx >= 100 && gx < 900 && gy >= 100 && gy < 600) {
                            bool occupied = false;
                            for (const auto& p : plants) {
                                if (std::abs(p.getX() - static_cast<float>(gx + 50)) < 50 &&
                                    std::abs(p.getY() - static_cast<float>(gy + 50)) < 50) {
                                    occupied = true;
                                    break;
                                }
                            }
                            if (!occupied) {
                                if (selectedPlant == PlantType::SHOOTER) {
                                    plants.emplace_back(selectedPlant, shooterTex, static_cast<float>(gx), static_cast<float>(gy));
                                }
                                else if (selectedPlant == PlantType::WALL) {
                                    plants.emplace_back(selectedPlant, wallTex, static_cast<float>(gx), static_cast<float>(gy));
                                }
                                else if (selectedPlant == PlantType::SLOWER) {
                                    plants.emplace_back(selectedPlant, slowerTex, static_cast<float>(gx), static_cast<float>(gy));
                                }
                            }
                        }
                    }
                }
            }
        }

        // ЛОГИКА
        if (currentState == GameState::PLAY) {
            if (!gameOver && !levelComplete) {
                if (waveActive && static_cast<int>(zombies.size()) + zombiesKilled < zombiesToKill) {
                    if (spawnClock.getElapsedTime().asSeconds() >= 1.2f) {
                        int row = rand() % 5;
                        float y = 100.0f + static_cast<float>(row) * 100.0f;

                        int r = rand() % 10;
                        ZombieType type;
                        if (currentLevel >= 3 && r < 2) type = ZombieType::TOUGH;
                        else if (currentLevel >= 2 && r < 4) type = ZombieType::FAST;
                        else type = ZombieType::NORMAL;

                        sf::Texture* tex = &zombieNormTex;
                        if (type == ZombieType::FAST) tex = &zombieFastTex;
                        if (type == ZombieType::TOUGH) tex = &zombieToughTex;

                        zombies.emplace_back(type, *tex, 950.0f, y);
                        spawnClock.restart();
                    }
                }

                if (waveActive && zombies.empty() && zombiesKilled >= zombiesToKill) {
                    waveActive = false;
                    levelComplete = true;
                    save.highestLevel = currentLevel;
                    save.score = score;
                }

                for (auto& z : zombies) z.update(deltaTime);

                for (auto& p : plants) {
                    p.update(deltaTime);
                    if (p.canShoot()) {
                        bullets.emplace_back(bulletTex, p.gridX + 70, p.gridY + 30, false);
                        p.resetShoot();
                    }
                    if (p.canSlow()) {
                        for (auto& z : zombies) {
                            if (std::abs(z.getY() - (p.gridY + 50)) < 50) {
                                bullets.emplace_back(bulletTex, p.gridX + 70, p.gridY + 30, true);
                                p.resetSlow();
                                break;
                            }
                        }
                    }
                }

                for (auto& b : bullets) b.update(deltaTime);

                for (auto& b : bullets) {
                    if (!b.isActive) continue;
                    for (auto& z : zombies) {
                        if (!z.isDead && b.sprite.getGlobalBounds().findIntersection(z.sprite.getGlobalBounds())) {
                            z.takeDamage(1.0f);
                            if (b.isSlow) z.slow(3.0f);
                            b.isActive = false;
                            if (z.isDead) {
                                int add = (z.type == ZombieType::TOUGH) ? 30 : (z.type == ZombieType::FAST ? 15 : 10);
                                score += add;
                                zombiesKilled++;
                            }
                            break;
                        }
                    }
                }

                for (const auto& z : zombies) {
                    if (!z.isDead && z.getX() < 100.0f) {
                        gameOver = true;
                        break;
                    }
                }

                zombies.erase(std::remove_if(zombies.begin(), zombies.end(),
                    [](const Zombie& z) { return z.isDead; }), zombies.end());
                bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                    [](const Bullet& b) { return !b.isActive; }), bullets.end());
                plants.erase(std::remove_if(plants.begin(), plants.end(),
                    [](const Plant& p) { return !p.isAlive; }), plants.end());
            }
        }

        window.clear(sf::Color(34, 139, 34));
        // ОТРИСОВКА
        if (currentState == GameState::PLAY) {
            window.clear(sf::Color(34, 139, 34));
            window.draw(gameBgSprite); 

            for (int i = 100; i < 900; i += 100) {
                for (int j = 100; j < 600; j += 100) {
                    sf::RectangleShape cell(sf::Vector2f(90.0f, 90.0f));
                    cell.setPosition({ static_cast<float>(i) + 5, static_cast<float>(j) + 5 });
                    cell.setFillColor(sf::Color::Transparent);
                    cell.setOutlineColor(sf::Color(80, 80, 80));
                    cell.setOutlineThickness(1.0f);
                    window.draw(cell);
                }
            }

            window.draw(house);
            for (auto& p : plants) window.draw(p.sprite);
            for (auto& z : zombies) window.draw(z.sprite);
            for (auto& b : bullets) window.draw(b.sprite);

            float selX = 10.0f + static_cast<int>(selectedPlant) * 35.0f;
            selector.setPosition({ selX, 660.0f });
            window.draw(selector);

            sf::RectangleShape scoreBg(sf::Vector2f(150.0f, 25.0f));
            scoreBg.setFillColor(sf::Color(0, 0, 0, 150));
            scoreBg.setPosition({ 10.0f, 10.0f });
            window.draw(scoreBg);

            sf::RectangleShape levelBg(sf::Vector2f(150.0f, 25.0f));
            levelBg.setFillColor(sf::Color(0, 0, 0, 150));
            levelBg.setPosition({ 10.0f, 40.0f });
            window.draw(levelBg);

            // Иконка лопаты 
            shovelSprite.setScale({ 0.13f, 0.13f });
            shovelSprite.setPosition({ -15, 100 }); 
            if (isShovelActive) {
                shovelSprite.setColor(sf::Color::Yellow); 
            }
            else {
                shovelSprite.setColor(sf::Color::White);
            }
            window.draw(shovelSprite);

            if (gameOver) {
                sf::RectangleShape overlay(sf::Vector2f(400.0f, 150.0f));
                overlay.setFillColor(sf::Color(0, 0, 0, 220));
                overlay.setPosition({ 300.0f, 280.0f });
                window.draw(overlay);
            }

            if (levelComplete) {
                sf::RectangleShape overlay(sf::Vector2f(400.0f, 150.0f));
                overlay.setFillColor(sf::Color(0, 0, 0, 220));
                overlay.setPosition({ 300.0f, 280.0f });
                window.draw(overlay);
            }
        }
        if (currentState == GameState::MENU) {
            window.draw(menuBgSprite);
            // Текст на главном меню
            sf::Text menuText(font);
            menuText.setString("MAIN MENU");
            menuText.setCharacterSize(60);
            menuText.setFillColor(sf::Color::White);

            // Выравнивание по центру
            sf::FloatRect menuBounds = menuText.getLocalBounds();
            menuText.setOrigin({ menuBounds.size.x / 2, menuBounds.size.y / 2 });
            menuText.setPosition({ 500, 250 });

            window.draw(menuText);

            sf::Text playText(font);
            playText.setString("Press ENTER to play");
            playText.setCharacterSize(30);
            playText.setFillColor(sf::Color::Yellow);

            sf::FloatRect playBounds = playText.getLocalBounds();
            playText.setOrigin({ playBounds.size.x / 2, playBounds.size.y / 2 });
            playText.setPosition({ 500, 350 });

            window.draw(playText);
        }

        if (currentState == GameState::LEVEL_SELECT) {
            window.draw(levelsBgSprite); 

            sf::Text title(font);
            title.setString("SELECT LEVEL");
            title.setCharacterSize(50);
            title.setFillColor(sf::Color::White);
            title.setOrigin({ title.getLocalBounds().size.x / 2, title.getLocalBounds().size.y / 2 });
            title.setPosition({ 500, 80 });
            window.draw(title);

            // Позиции кнопок
            float positionsX[5] = { 150, 400, 650, 275, 525 };
            float positionsY[5] = { 200, 200, 200, 350, 350 };

            for (int i = 0; i < 5; i++) {
                sf::Sprite button(levelButtonTex);
                button.setScale({ 0.13f, 0.13f });
                button.setPosition({ positionsX[i], positionsY[i] });
                window.draw(button);

                // Цифра уровня
                sf::Text levelNum(font);
                levelNum.setString(std::to_string(i + 1));
                levelNum.setCharacterSize(30);
                levelNum.setFillColor(sf::Color::White);
                sf::FloatRect btnBounds = button.getGlobalBounds();
                levelNum.setOrigin({ levelNum.getLocalBounds().size.x / 2, levelNum.getLocalBounds().size.y / 2 });
                levelNum.setPosition({ btnBounds.position.x + btnBounds.size.x / 2, btnBounds.position.y + btnBounds.size.y / 2 });
                window.draw(levelNum);
            }

            // Кнопка "Назад"
            sf::Text backText(font);
            backText.setString("BACK");
            backText.setCharacterSize(30);
            backText.setFillColor(sf::Color::White);
            backText.setOrigin({ backText.getLocalBounds().size.x / 2, backText.getLocalBounds().size.y / 2 });
            backText.setPosition({ 500, 600 });
            window.draw(backText);
        }

        window.display();
    }

    return 0;
}