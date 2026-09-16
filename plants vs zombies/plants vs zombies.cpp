#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#include <memory>
#include <windows.h>

enum class ZombieType { NORMAL, FAST, TOUGH };
enum class PlantType { NONE, SHOOTER, WALL, SLOWER };
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
        // Получаем размер текстуры
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

        // Ставим в центр клетки (startX - это левый верхний угол клетки)
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
                // При замедлении делаем синеватый оттенок поверх текстуры
                sprite.setColor(sf::Color(150, 200, 255));
            }
            else {
                speed = originalSpeed;
                // Возвращаем белый цвет, чтобы текстура отображалась нормально
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
            sprite.setScale({ 0.45f, 0.45f });
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
        if (slow) sprite.setColor(sf::Color(30, 60, 255));
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
    bool levelsCompleted[5] = { false, false, false, false, false };
};
void saveGame(const SaveData& save, const std::string& fileName)
{
    std::ofstream file(fileName, std::ios::binary);

    if (file.is_open())
    {
        file.write(reinterpret_cast<const char*>(&save), sizeof(save));
        file.close();
    }
}

void loadGame(SaveData& save, const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::binary);

    if (file.is_open())
    {
        file.read(reinterpret_cast<char*>(&save), sizeof(save));
        file.close();
    }
    else
    {
        save.level = 1;
        save.score = 0;
        save.highestLevel = 1;
        for (int i = 0; i < 5; i++) save.levelsCompleted[i] = false;
    }
}

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

    sf::RenderWindow window(sf::VideoMode({ 1000, 700 }), "Zombies vs Plants");
    window.setFramerateLimit(60);

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
    std::string wallPath = findPic("wall1.png");
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
    
    // Текущий уровень
    sf::Texture levelBgTex;
    std::string levelBgPath = findPic("lvl_screen.png");
    if (!levelBgTex.loadFromFile(levelBgPath)) {
        levelBgTex = createTexture(80, 80, sf::Color(100, 30, 150));
    }
    sf::Sprite levelBgSprite(levelBgTex);
    levelBgSprite.setScale({ 0.26f, 0.23f });
    levelBgSprite.setPosition({ -7.0f, 5.0f });

    // Кнопки уровней
    sf::Texture levelButtonTex;
    std::string levelButtonPath = findPic("button_lvl.png");
    if (!levelButtonTex.loadFromFile(levelButtonPath)) {
        levelButtonTex = createTexture(80, 80, sf::Color(150, 50, 200));
    }
    sf::Texture levelButtonDarkTex;
    std::string levelButtonDarkPath = findPic("darkbutton_lvl.png");
    if (!levelButtonDarkTex.loadFromFile(levelButtonDarkPath)) {
        levelButtonDarkTex = createTexture(80, 80, sf::Color(100, 30, 150));
    }

    // Кнопки заблокированных уровней
    sf::Texture levelLockedTex;
    std::string levelLockedPath = findPic("locked_btn.png");
    if (!levelLockedTex.loadFromFile(levelLockedPath)) {
        levelLockedTex = createTexture(80, 80, sf::Color(80, 80, 80));
    }

    // Кнопки врхнего меню
    sf::Texture ButtonTex;
    std::string ButtonPath = findPic("stone_button.png");
    if (!ButtonTex.loadFromFile(ButtonPath)) {
        ButtonTex = createTexture(60, 40, sf::Color(200, 200, 200));
    }

    sf::Texture ButtonDarkTex;
    std::string ButtonDarkPath = findPic("stone_dark.png");
    if (!ButtonDarkTex.loadFromFile(ButtonDarkPath)) {
        ButtonDarkTex = createTexture(80, 80, sf::Color(100, 30, 150));
    }
    // Кнопка "back"
    sf::Sprite backButtonSprite(ButtonTex);
    backButtonSprite.setScale({ 0.3f, 0.3f });
    backButtonSprite.setPosition({ 880.0f, 5.0f });
    // Кнопка "pause"
    sf::Sprite pauseButtonSprite(ButtonTex);
    pauseButtonSprite.setScale({ 0.2f, 0.3f });
    pauseButtonSprite.setPosition({ 710.0f, 5.0f });
    // Кнопка "save"
    sf::Sprite saveButtonSprite(ButtonTex);
    saveButtonSprite.setScale({ 0.3f, 0.3f });
    saveButtonSprite.setPosition({ 780.0f, 5.0f });

    // Экран паузы
    sf::Texture pauseBgTexture;
    std::string pauseBgPath = findPic("display_board.png");
    if (!pauseBgTexture.loadFromFile(pauseBgPath)) {
        pauseBgTexture = createTexture(400, 200, sf::Color(50, 50, 50));
    }
    sf::Sprite pauseBgSprite(pauseBgTexture);

    // Продолжить
    sf::Sprite pauseResumeBtn(ButtonTex);  

    sf::Texture playIconTex;
    std::string playIconPath = findPic("icon_play.png");
    if (!playIconTex.loadFromFile(playIconPath)) {
        playIconTex = createTexture(1000, 700, sf::Color(34, 139, 34));
    }
    sf::Sprite playIcon(playIconTex);

    // Перезапустить
    sf::Sprite pauseRestartBtn(ButtonTex); 

    sf::Texture restartIconTex;
    std::string restartIconPath = findPic("icon_restart.png");
    if (!restartIconTex.loadFromFile(restartIconPath)) {
        restartIconTex = createTexture(1000, 700, sf::Color(34, 139, 34));
    }
    sf::Sprite restartIcon(restartIconTex);

    // Кнопка "load"
    sf::Texture loadButtonTex;
    std::string loadButtonPath = findPic("hand_btn.png");
    if (!loadButtonTex.loadFromFile(loadButtonPath)) {
        loadButtonTex = createTexture(60, 40, sf::Color(50, 50, 200));
    }
    sf::Sprite loadButtonSprite(loadButtonTex);
    loadButtonSprite.setScale({ 0.2f, 0.2f });
    loadButtonSprite.setPosition({ 500.0f, 600.0f });

    // Текстура пули
    sf::Texture bulletTex = createTexture(20, 20, sf::Color(255, 255, 0));

    // Игровые объекты
    std::vector<Zombie> zombies;
    std::vector<Plant> plants;
    std::vector<Bullet> bullets;

    // Иконки для панели выбора 
    sf::Sprite shooterIcon(shooterTex);
    shooterIcon.setScale({ 0.08f, 0.08f });
    shooterIcon.setPosition({ 190.0f, 10.0f });

    sf::Sprite wallIcon(wallTex);
    wallIcon.setScale({ 0.33f, 0.33f });
    wallIcon.setPosition({ 220.0f, -10.0f });

    sf::Sprite slowerIcon(slowerTex);
    slowerIcon.setScale({ 0.103f, 0.103f });
    slowerIcon.setPosition({ 320.0f, -25.0f });

    // Состояние игры
    SaveData save;
    std::string currentSaveFile = "save1.dat";
    loadGame(save, currentSaveFile);

    int score = save.score;
    int currentLevel = save.highestLevel;
    int zombiesToKill = 5 + currentLevel * 2;
    int zombiesKilled = 0;
    bool waveActive = true;
    bool gameOver = false;
    bool levelComplete = false;
    bool isShovelActive = false;
    bool showSaveSlots = false;
    bool showLoadSlots = false;
    bool isPaused = false;

    PlantType selectedPlant = PlantType::NONE;

    sf::Clock gameClock;
    sf::Clock spawnClock;

    // Выделение выбранного растения
    sf::RectangleShape selector(sf::Vector2f(60.0f, 60.0f));
    selector.setFillColor(sf::Color::Transparent);
    selector.setOutlineColor(sf::Color::White);
    selector.setOutlineThickness(4.0f);

    // Переменная для отслеживания, все ли уровни пройдены
    bool allLevelsCompleted = false;

    while (window.isOpen()) {
        float deltaTime = gameClock.restart().asSeconds();
        if (deltaTime > 0.033f) deltaTime = 0.033f;

        // СОБЫТИЯ
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                saveGame(save, currentSaveFile);
                window.close();
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::P && currentState == GameState::PLAY) {
                    isPaused = !isPaused; 
                }
                if (key->code == sf::Keyboard::Key::Enter && currentState == GameState::MENU) {
                    currentState = GameState::LEVEL_SELECT;
                }
                if (key->code == sf::Keyboard::Key::L) {
                    isShovelActive = !isShovelActive;
                    if (isShovelActive) {
                        selectedPlant = PlantType::NONE;
                    }
                }
                if (key->code == sf::Keyboard::Key::Num1) {
                    selectedPlant = PlantType::SHOOTER;
                    isShovelActive = false;
                }
                if (key->code == sf::Keyboard::Key::Num2) {
                    selectedPlant = PlantType::WALL;
                    isShovelActive = false;
                }
                if (key->code == sf::Keyboard::Key::Num3) {
                    selectedPlant = PlantType::SLOWER;
                    isShovelActive = false;
                }

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

                    selectedPlant = PlantType::NONE;
                    isShovelActive = false;

                    save.score = score;
                    save.highestLevel = currentLevel;
                    save.level = currentLevel;

                    for (int i = 0; i < 5; i++) save.levelsCompleted[i] = false;
                    allLevelsCompleted = false;
                }

                if (key->code == sf::Keyboard::Key::N && levelComplete && currentLevel < 5) {
                    save.levelsCompleted[currentLevel - 1] = true;

                    // Очищаем всё с поля
                    plants.clear();     
                    bullets.clear();    
                    zombies.clear();    

                    selectedPlant = PlantType::NONE;   
                    isShovelActive = false;            

                    currentLevel++;
                    zombiesToKill = 5 + currentLevel * 2;
                    zombiesKilled = 0;
                    levelComplete = false;
                    waveActive = true;
                    save.highestLevel = currentLevel;
                    save.score = score;
                    save.level = currentLevel;
                    saveGame(save, currentSaveFile);
                }

                if (key->code == sf::Keyboard::Key::S) {
                    save.score = score;
                    save.highestLevel = currentLevel;
                    save.level = currentLevel;
                    std::ofstream sf("save.dat", std::ios::binary);
                    if (sf.is_open()) sf.write(reinterpret_cast<char*>(&save), sizeof(save));
                }
                if (key->code == sf::Keyboard::Key::Escape) {
                    selectedPlant = PlantType::NONE;
                    isShovelActive = false;
                }
            }

            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));

                    if (showSaveSlots)
                    {
                        sf::FloatRect slot1;
                        slot1.position = { 300.f, 200.f };
                        slot1.size = { 200.f, 60.f };

                        sf::FloatRect slot2;
                        slot2.position = { 300.f, 300.f };
                        slot2.size = { 200.f, 60.f };

                        sf::FloatRect slot3;
                        slot3.position = { 300.f, 400.f };
                        slot3.size = { 200.f, 60.f };

                        if (slot1.contains(mousePos))
                        {
                            saveGame(save, "save1.dat");
                            showSaveSlots = false;
                        }
                        else if (slot2.contains(mousePos))
                        {
                            saveGame(save, "save2.dat");
                            showSaveSlots = false;
                        }
                        else if (slot3.contains(mousePos))
                        {
                            saveGame(save, "save3.dat");
                            showSaveSlots = false;
                        }
                        continue;
                    }

                    if (showLoadSlots)
                    {
                        sf::FloatRect slot1;
                        slot1.position = { 300.f, 200.f };
                        slot1.size = { 200.f, 60.f };

                        sf::FloatRect slot2;
                        slot2.position = { 300.f, 300.f };
                        slot2.size = { 200.f, 60.f };

                        sf::FloatRect slot3;
                        slot3.position = { 300.f, 400.f };
                        slot3.size = { 200.f, 60.f };

                        if (slot1.contains(mousePos))
                        {
                            loadGame(save, "save1.dat");
                            showLoadSlots = false;

                            // Проверяем все ли уровни пройдены после загрузки
                            allLevelsCompleted = true;
                            for (int i = 0; i < 5; i++) {
                                if (!save.levelsCompleted[i]) {
                                    allLevelsCompleted = false;
                                    break;
                                }
                            }
                        }
                        else if (slot2.contains(mousePos))
                        {
                            loadGame(save, "save2.dat");
                            showLoadSlots = false;

                            allLevelsCompleted = true;
                            for (int i = 0; i < 5; i++) {
                                if (!save.levelsCompleted[i]) {
                                    allLevelsCompleted = false;
                                    break;
                                }
                            }
                        }
                        else if (slot3.contains(mousePos))
                        {
                            loadGame(save, "save3.dat");
                            showLoadSlots = false;

                            allLevelsCompleted = true;
                            for (int i = 0; i < 5; i++) {
                                if (!save.levelsCompleted[i]) {
                                    allLevelsCompleted = false;
                                    break;
                                }
                            }
                        }
                        continue;
                    }

                    if (currentState == GameState::PLAY) {
                        // Если игра на паузе - игнорируем клики
                        if (isPaused) {
                            // Проверяем клик по кнопке "Продолжить"
                            if (pauseResumeBtn.getGlobalBounds().contains(mousePos)) {
                                isPaused = false;
                                continue;
                            }
                            // Проверяем клик по кнопке "Перезапуск"
                            if (pauseRestartBtn.getGlobalBounds().contains(mousePos)) {
                                // Перезапускаем уровень
                                zombies.clear();
                                plants.clear();
                                bullets.clear();
                                zombiesKilled = 0;
                                gameOver = false;
                                levelComplete = false;
                                waveActive = true;
                                isPaused = false;
                                spawnClock.restart();
                                // Сбрасываем выбранное
                                selectedPlant = PlantType::NONE;
                                isShovelActive = false;
                                continue;
                            }
                            
                            sf::FloatRect backRect;
                            sf::FloatRect bgBounds = pauseBgSprite.getGlobalBounds();

                            sf::Text tempText(font);
                            tempText.setString("BACK TO LEVELS");
                            tempText.setCharacterSize(20);
                            tempText.setStyle(sf::Text::Bold);
                            sf::FloatRect textBounds = tempText.getLocalBounds();

                            float textX = bgBounds.position.x + bgBounds.size.x / 2 + 10.0f - textBounds.size.x / 2;
                            float textY = bgBounds.position.y + bgBounds.size.y - 80.0f - textBounds.size.y / 2;

                            backRect.position = { textX, textY };
                            backRect.size = { textBounds.size.x, textBounds.size.y };

                            if (backRect.contains(mousePos)) {
                                currentState = GameState::LEVEL_SELECT;
                                zombies.clear();
                                plants.clear();
                                bullets.clear();
                                gameOver = false;
                                levelComplete = false;
                                waveActive = true;
                                zombiesKilled = 0;
                                isShovelActive = false;
                                selectedPlant = PlantType::NONE;
                                isPaused = false;
                                continue;
                            }
                            continue;
                        }
                        if (saveButtonSprite.getGlobalBounds().contains(mousePos)) {
                            showSaveSlots = true;
                            continue;
                        }
                        // Проверка кнопки паузы
                        if (pauseButtonSprite.getGlobalBounds().contains(mousePos)) {
                            isPaused = !isPaused; 
                            continue;
                        }
                        if (backButtonSprite.getGlobalBounds().contains(mousePos)) {
                            currentState = GameState::LEVEL_SELECT;
                            zombies.clear();
                            plants.clear();
                            bullets.clear();
                            gameOver = false;
                            levelComplete = false;
                            waveActive = true;
                            zombiesKilled = 0;
                            isShovelActive = false;
                            selectedPlant = PlantType::NONE;
                            continue;
                        }

                        sf::FloatRect shovelBounds = shovelSprite.getGlobalBounds();
                        if (shovelBounds.contains(mousePos)) {
                            isShovelActive = !isShovelActive;
                            if (isShovelActive) {
                                selectedPlant = PlantType::NONE;
                            }
                            continue;
                        }

                        if (isShovelActive) {
                            for (auto it = plants.begin(); it != plants.end(); ++it) {
                                sf::FloatRect plantBounds = it->sprite.getGlobalBounds();
                                if (plantBounds.contains(mousePos)) {
                                    it->isAlive = false;
                                    break;
                                }
                            }
                            continue;
                        }
                        // Выбор растений мышкой
                        if (shooterIcon.getGlobalBounds().contains(mousePos)) {
                            selectedPlant = PlantType::SHOOTER;
                            isShovelActive = false;
                            continue;
                        }
                        if (wallIcon.getGlobalBounds().contains(mousePos)) {
                            selectedPlant = PlantType::WALL;
                            isShovelActive = false;
                            continue;
                        }
                        if (slowerIcon.getGlobalBounds().contains(mousePos)) {
                            selectedPlant = PlantType::SLOWER;
                            isShovelActive = false;
                            continue;
                        }

                        if (!gameOver && !levelComplete && waveActive) {
                            int gx = (static_cast<int>(mousePos.x) / 100) * 100;
                            int gy = (static_cast<int>(mousePos.y) / 100) * 100;

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

                    if (currentState == GameState::LEVEL_SELECT) {
                        if (loadButtonSprite.getGlobalBounds().contains(mousePos)) {
                            showLoadSlots = true;
                            continue;
                        }

                        sf::FloatRect backRect;
                        backRect.position = sf::Vector2f(450, 580);
                        backRect.size = sf::Vector2f(120, 40);
                        if (backRect.contains(mousePos)) {
                            currentState = GameState::MENU;
                            continue;
                        }

                        float positionsX[5] = { 150, 400, 650, 275, 525 };
                        float positionsY[5] = { 200, 200, 200, 350, 350 };
                        for (int i = 0; i < 5; i++) {
                            // Если все уровни пройдены - все разблокированы
                            bool isLocked;
                            if (allLevelsCompleted) {
                                isLocked = false; 
                            }
                            else {
                                isLocked = save.levelsCompleted[i]; 
                            }

                            std::unique_ptr<sf::Sprite> button;
                            if (isLocked) {
                                button = std::make_unique<sf::Sprite>(levelLockedTex);
                            }
                            else {
                                button = std::make_unique<sf::Sprite>(levelButtonTex);
                            }
                            button->setScale({ 0.13f, 0.13f });
                            button->setPosition({ positionsX[i], positionsY[i] });

                            if (button->getGlobalBounds().contains(mousePos)) {
                                if (!isLocked) {
                                    currentLevel = i + 1;
                                    zombiesToKill = 5 + currentLevel * 2;
                                    zombiesKilled = 0;
                                    waveActive = true;
                                    gameOver = false;
                                    levelComplete = false;
                                    currentState = GameState::PLAY;

                                    isShovelActive = false;
                                    selectedPlant = PlantType::NONE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (currentState == GameState::PLAY) {
            if (!isPaused) {
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

                        // Отмечаем уровень как пройденный
                        save.levelsCompleted[currentLevel - 1] = true;

                        // Проверяем, все ли уровни пройдены
                        allLevelsCompleted = true;
                        for (int i = 0; i < 5; i++) {
                            if (!save.levelsCompleted[i]) {
                                allLevelsCompleted = false;
                                break;
                            }
                        }

                        // Если все уровни пройдены - сбрасываем
                        if (allLevelsCompleted) {
                            for (int i = 0; i < 5; i++) {
                                save.levelsCompleted[i] = false;
                            }
                            allLevelsCompleted = false;
                            // Сохраняем изменения
                            saveGame(save, currentSaveFile);
                        }
                    }

                    for (auto& z : zombies) {
                        bool blocked = false;
                        for (auto& p : plants) {
                            if (p.type == PlantType::WALL && p.isAlive) {
                                // Получаем позиции
                                sf::Vector2f zombiePos = z.sprite.getPosition();
                                sf::Vector2f wallPos = p.sprite.getPosition();

                                // Вычисляем расстояние между зомби и стеной
                                float distance = std::abs(zombiePos.x - wallPos.x);

                                // Если зомби подошел близко к стене 
                                if (distance < 50.0f && std::abs(zombiePos.y - wallPos.y) < 50.0f) {
                                    blocked = true;
                                    p.health -= deltaTime * 5.0f;

                                    // Мигаем красным + уменьшаем размер
                                    p.sprite.setColor(sf::Color(255, 100, 100));

                                    float healthPercent = p.health / 10.0f;
                                    float scale = 0.45f * (0.6f + 0.4f * healthPercent);
                                    p.sprite.setScale({ scale, scale });

                                    // Отталкиваем зомби от стены
                                    //z.sprite.setPosition({ wallPos.x + 55.0f, zombiePos.y });

                                    if (p.health <= 0) {
                                        p.isAlive = false;
                                    }
                                    break;
                                }
                            }
                        }

                        if (!blocked) {
                            z.update(deltaTime);
                        }
                    }

                    // Если стена не получает урон - возвращаем белый цвет
                    for (auto& p : plants) {
                        if (p.type == PlantType::WALL && p.isAlive) {
                            // Проверяем, получает ли стена урон сейчас
                            bool isBeingAttacked = false;
                            for (auto& z : zombies) {
                                sf::FloatRect zombieBounds = z.sprite.getGlobalBounds();
                                sf::FloatRect wallBounds = p.sprite.getGlobalBounds();
                                if (zombieBounds.findIntersection(wallBounds)) {
                                    isBeingAttacked = true;
                                    break;
                                }
                            }
                            if (!isBeingAttacked) {
                                p.sprite.setColor(sf::Color::White);
                            }
                        }
                    }

                    for (auto& p : plants) {
                        p.update(deltaTime);

                        // Проверяем, есть ли зомби на этом ряду
                        bool hasZombieInRow = false;
                        for (const auto& z : zombies) {
                            if (!z.isDead && std::abs(z.getY() - (p.gridY + 50)) < 50) {
                                hasZombieInRow = true;
                                break;
                            }
                        }

                        // Стреляем только если есть зомби в ряду
                        if (p.canShoot() && hasZombieInRow) {
                            bullets.emplace_back(bulletTex, p.gridX + 70, p.gridY + 30, false);
                            p.resetShoot();
                        }

                        // Слоуэр тоже работает только если есть зомби в ряду
                        if (p.canSlow() && hasZombieInRow) {
                            for (auto& z : zombies) {
                                if (!z.isDead && std::abs(z.getY() - (p.gridY + 50)) < 50) {
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
        }

        window.clear(sf::Color(34, 139, 34));

        if (currentState == GameState::PLAY) {
            window.clear(sf::Color(34, 139, 34));
            window.draw(gameBgSprite);

            // Получаем позицию мыши
            sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));

            window.draw(shooterIcon);
            window.draw(wallIcon);
            window.draw(slowerIcon);

            sf::Text hint1(font);
            hint1.setString("1");
            hint1.setCharacterSize(14);
            hint1.setFillColor(sf::Color::White);
            hint1.setPosition({ 35.0f, 700.0f });
            window.draw(hint1);

            sf::Text hint2(font);
            hint2.setString("2");
            hint2.setCharacterSize(14);
            hint2.setFillColor(sf::Color::White);
            hint2.setPosition({ 95.0f, 700.0f });
            window.draw(hint2);

            sf::Text hint3(font);
            hint3.setString("3");
            hint3.setCharacterSize(14);
            hint3.setFillColor(sf::Color::White);
            hint3.setPosition({ 155.0f, 700.0f });
            window.draw(hint3);

            for (auto& p : plants) window.draw(p.sprite);
            for (auto& z : zombies) window.draw(z.sprite);
            for (auto& b : bullets) window.draw(b.sprite);

            float selectorX;
            if (selectedPlant == PlantType::WALL) selectorX = 260.0f;
            else if (selectedPlant == PlantType::SLOWER) selectorX = 330.0f;
            else if (selectedPlant == PlantType::SHOOTER) selectorX = 190.0f;
            else if (isShovelActive) selectorX = 470.0f;
            else selectorX = -100.0f;

            selector.setPosition({ selectorX, 11.0f });
            window.draw(selector);

            // Фон для уровня
            window.draw(levelBgSprite);

            sf::Text levelText(font);
            levelText.setString("Lvl " + std::to_string(currentLevel));
            levelText.setCharacterSize(30);
            levelText.setFillColor(sf::Color::Yellow);
            levelText.setStyle(sf::Text::Bold);
            // Получаем размеры
            sf::FloatRect textBounds = levelText.getLocalBounds();
            sf::FloatRect bgBounds = levelBgSprite.getGlobalBounds();

            // Центрируем текст
            levelText.setOrigin({
                textBounds.size.x / 2,
                textBounds.size.y / 2
                });

            levelText.setPosition({
                bgBounds.position.x + bgBounds.size.x / 2,
                bgBounds.position.y + bgBounds.size.y / 2
                });
            window.draw(levelText);
            //лопата
            shovelSprite.setScale({ 0.08f, 0.07f });
            shovelSprite.setPosition({ 460, -12 });

            window.draw(shovelSprite);

            window.draw(backButtonSprite);
            window.draw(saveButtonSprite);
            window.draw(pauseButtonSprite);

            // Кнопка PAUSE с подсветкой
            sf::Sprite pauseBtnToDraw = pauseButtonSprite;
            if (pauseButtonSprite.getGlobalBounds().contains(mousePos)) {
                pauseBtnToDraw.setTexture(ButtonDarkTex);
            }
            window.draw(pauseBtnToDraw);

            // Текст на кнопке паузы
            sf::Text pauseButtonText(font);
            pauseButtonText.setString("ii");
            pauseButtonText.setCharacterSize(24);
            pauseButtonText.setFillColor(sf::Color(60, 60, 60));

            sf::FloatRect pauseBtnBounds = pauseButtonSprite.getGlobalBounds();
            pauseButtonText.setOrigin({ pauseButtonText.getLocalBounds().size.x / 2, pauseButtonText.getLocalBounds().size.y / 2 });
            pauseButtonText.setPosition({
                pauseBtnBounds.position.x + pauseBtnBounds.size.x / 2,
                pauseBtnBounds.position.y + pauseBtnBounds.size.y / 2
                });
            window.draw(pauseButtonText);

            // Кнопка BACK с подсветкой
            sf::Sprite backBtnToDraw = backButtonSprite;
            if (backButtonSprite.getGlobalBounds().contains(mousePos)) {
                backBtnToDraw.setTexture(ButtonDarkTex);
                backBtnToDraw.setScale({ 0.3f, 0.3f });
            }
            window.draw(backBtnToDraw);

            sf::Text backText(font);
            backText.setString("BACK");
            backText.setCharacterSize(20);
            backText.setFillColor(sf::Color(60, 60, 60));

            sf::FloatRect btnBounds = backButtonSprite.getGlobalBounds();
            backText.setOrigin({ backText.getLocalBounds().size.x / 2, backText.getLocalBounds().size.y / 2 });
            backText.setPosition({
                btnBounds.position.x + btnBounds.size.x / 2,
                btnBounds.position.y + btnBounds.size.y / 2
                });
            window.draw(backText);

            // Кнопка SAVE с подсветкой
            sf::Sprite saveBtnToDraw = saveButtonSprite;
            if (saveButtonSprite.getGlobalBounds().contains(mousePos)) {
                saveBtnToDraw.setTexture(ButtonDarkTex);
            }
            window.draw(saveBtnToDraw);


            sf::Text saveText(font);
            saveText.setString("SAVE");
            saveText.setCharacterSize(20);
            saveText.setFillColor(sf::Color(60, 60, 60));

            sf::FloatRect saveBounds = saveButtonSprite.getGlobalBounds();
            saveText.setOrigin({ saveText.getLocalBounds().size.x / 2, saveText.getLocalBounds().size.y / 2 });
            saveText.setPosition({
                saveBounds.position.x + saveBounds.size.x / 2,
                saveBounds.position.y + saveBounds.size.y / 2
                });
            window.draw(saveText);

            sf::Text modeText(font);
            modeText.setCharacterSize(16);
            modeText.setFillColor(sf::Color::White);
            modeText.setPosition({ 10.0f, 670.0f });

            if (isShovelActive) {
                modeText.setString("Mode SHOVEL");
                modeText.setFillColor(sf::Color::Yellow);
            }
            else if (selectedPlant != PlantType::NONE) {
                std::string plantName;
                if (selectedPlant == PlantType::SHOOTER) plantName = "SHOOTER";
                else if (selectedPlant == PlantType::WALL) plantName = "WALL";
                else if (selectedPlant == PlantType::SLOWER) plantName = "SLOWER";
                modeText.setString("Mode " + plantName);
                modeText.setFillColor(sf::Color::Cyan);
            }
            else {
                modeText.setString("Mode NONE");
                modeText.setFillColor(sf::Color(128, 128, 128));
            }
            window.draw(modeText);

            if (gameOver) {
                sf::RectangleShape overlay(sf::Vector2f(400.0f, 150.0f));
                overlay.setFillColor(sf::Color(0, 0, 0, 220));
                overlay.setPosition({ 300.0f, 280.0f });
                window.draw(overlay);

                sf::Text gameOverText(font);
                gameOverText.setString("GAME OVER");
                gameOverText.setCharacterSize(40);
                gameOverText.setFillColor(sf::Color::Red);
                gameOverText.setOrigin({ gameOverText.getLocalBounds().size.x / 2, gameOverText.getLocalBounds().size.y / 2 });
                gameOverText.setPosition({ 500, 320 });
                window.draw(gameOverText);

                sf::Text restartText(font);
                restartText.setString("Press R to restart");
                restartText.setCharacterSize(20);
                restartText.setFillColor(sf::Color::White);
                restartText.setOrigin({ restartText.getLocalBounds().size.x / 2, restartText.getLocalBounds().size.y / 2 });
                restartText.setPosition({ 500, 380 });
                window.draw(restartText);
            }

            if (levelComplete) {
                sf::RectangleShape overlay(sf::Vector2f(400.0f, 150.0f));
                overlay.setFillColor(sf::Color(0, 0, 0, 220));
                overlay.setPosition({ 300.0f, 280.0f });
                window.draw(overlay);

                sf::Text completeText(font);
                completeText.setString("LEVEL COMPLETE!");
                completeText.setCharacterSize(35);
                completeText.setFillColor(sf::Color::Green);
                completeText.setOrigin({ completeText.getLocalBounds().size.x / 2, completeText.getLocalBounds().size.y / 2 });
                completeText.setPosition({ 500, 310 });
                window.draw(completeText);

                if (currentLevel < 5) {
                    sf::Text nextText(font);
                    nextText.setString("Press N for next level");
                    nextText.setCharacterSize(20);
                    nextText.setFillColor(sf::Color::White);
                    nextText.setOrigin({ nextText.getLocalBounds().size.x / 2, nextText.getLocalBounds().size.y / 2 });
                    nextText.setPosition({ 500, 370 });
                    window.draw(nextText);
                }
                else {
                    sf::Text winText(font);
                    winText.setString("YOU WIN! Press BACK");
                    winText.setCharacterSize(20);
                    winText.setFillColor(sf::Color::Yellow);
                    winText.setOrigin({ winText.getLocalBounds().size.x / 2, winText.getLocalBounds().size.y / 2 });
                    winText.setPosition({ 500, 370 });
                    window.draw(winText);
                }
            }
            if (isPaused) {
                sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
                
                pauseBgSprite.setScale({ 0.6f, 0.6f });
                pauseBgSprite.setPosition({ 165.0f, 130.0f });
                window.draw(pauseBgSprite);

                sf::FloatRect bgBounds = pauseBgSprite.getGlobalBounds();

                // Кнопки
                float buttonWidth = 154.0f;
                float buttonHeight = 60.0f;
                float spacing = 80.0f;
                float totalWidth = buttonWidth * 2 + spacing;
                float startX = bgBounds.position.x + (bgBounds.size.x - totalWidth) / 2;
                float startY = bgBounds.position.y + 180.0f;

                pauseResumeBtn.setScale({ 0.5f, 0.5f });
                pauseResumeBtn.setPosition({ startX, startY });
                window.draw(pauseResumeBtn);

                sf::Sprite resumeBtnToDraw = pauseResumeBtn;
                if (pauseResumeBtn.getGlobalBounds().contains(mousePos)) {
                    resumeBtnToDraw.setTexture(ButtonDarkTex);
                }
                window.draw(resumeBtnToDraw);
                // Иконка на кнопке PLAY
                sf::FloatRect resumeBtnBounds = pauseResumeBtn.getGlobalBounds();
                float playIconSize = 100.0f; 
                sf::Vector2u playIconTexSize = playIconTex.getSize();
                float playIconScaleX = playIconSize / playIconTexSize.x;
                float playIconScaleY = playIconSize / playIconTexSize.y;
                playIcon.setScale({ playIconScaleX, playIconScaleY });
                playIcon.setPosition({
                    resumeBtnBounds.position.x + resumeBtnBounds.size.x / 2 - playIconSize / 2,
                    resumeBtnBounds.position.y + resumeBtnBounds.size.y / 2 - playIconSize / 2
                    });
                window.draw(playIcon);

                pauseRestartBtn.setScale({ 0.5f, 0.5f });
                pauseRestartBtn.setPosition({ startX + buttonWidth + spacing, startY });
                window.draw(pauseRestartBtn);

                sf::Sprite restartBtnToDraw = pauseRestartBtn;
                if (pauseRestartBtn.getGlobalBounds().contains(mousePos)) {
                    restartBtnToDraw.setTexture(ButtonDarkTex);
                }
                window.draw(restartBtnToDraw);
                // Иконка на кнопке RESTART
                sf::FloatRect restartBtnBounds = pauseRestartBtn.getGlobalBounds();
                float restartIconSize = 100.0f; 
                sf::Vector2u restartIconTexSize = restartIconTex.getSize();

                // Вычисляем масштаб с сохранением пропорций
                float restartIconScale = std::min(
                    restartIconSize / restartIconTexSize.x,
                    restartIconSize / restartIconTexSize.y
                );

                restartIcon.setScale({ restartIconScale, restartIconScale });
                restartIcon.setPosition({
                    restartBtnBounds.position.x + restartBtnBounds.size.x / 2 - (restartIconTexSize.x * restartIconScale) / 2,
                    restartBtnBounds.position.y + restartBtnBounds.size.y / 2 - (restartIconTexSize.y * restartIconScale) / 2
                    });
                window.draw(restartIcon);

                sf::Text backToLevelsText(font);
                backToLevelsText.setString("BACK TO LEVELS");
                backToLevelsText.setCharacterSize(20);
                backToLevelsText.setStyle(sf::Text::Bold);

                sf::FloatRect backToLevelsBounds = backToLevelsText.getLocalBounds(); 
                backToLevelsText.setOrigin({ backToLevelsBounds.size.x / 2, backToLevelsBounds.size.y / 2 });
                float textX = bgBounds.position.x + bgBounds.size.x / 2 + 10.0f;
                float textY = bgBounds.position.y + bgBounds.size.y - 80.0f;
                backToLevelsText.setPosition({ textX, textY });

                // Проверяем наведение
                sf::FloatRect backToLevelsRect;
                backToLevelsRect.position = { textX - backToLevelsBounds.size.x / 2, textY - backToLevelsBounds.size.y / 2 };
                backToLevelsRect.size = { backToLevelsBounds.size.x, backToLevelsBounds.size.y };

                if (backToLevelsRect.contains(mousePos)) {
                    backToLevelsText.setFillColor(sf::Color(180, 180, 180)); 
                }
                else {
                    backToLevelsText.setFillColor(sf::Color::White); 
                }

                window.draw(backToLevelsText);

                sf::FloatRect backBounds = backToLevelsText.getLocalBounds();
                backToLevelsText.setOrigin({ backBounds.size.x / 2, backBounds.size.y / 2 });
                backToLevelsText.setPosition({
                    bgBounds.position.x + bgBounds.size.x / 2 + 10.0f,
                    bgBounds.position.y + bgBounds.size.y - 80.0f
                    });
                window.draw(backToLevelsText);

                // Заголовок PAUSED 
                sf::Text pauseTitle(font);
                pauseTitle.setString("PAUSED");
                pauseTitle.setCharacterSize(40);
                pauseTitle.setFillColor(sf::Color::Yellow);
                pauseTitle.setStyle(sf::Text::Bold);

                sf::FloatRect titleBounds = pauseTitle.getLocalBounds();
                pauseTitle.setOrigin({ titleBounds.size.x / 2, titleBounds.size.y / 2 });
                pauseTitle.setPosition({
                    bgBounds.position.x + bgBounds.size.x / 2 + 10.0f,
                    bgBounds.position.y + 140.0f
                    });
                window.draw(pauseTitle);
            }
        }

        if (currentState == GameState::MENU) {
            window.draw(menuBgSprite);
            sf::Text menuText(font);
            menuText.setString("MAIN MENU");
            menuText.setCharacterSize(60);
            menuText.setFillColor(sf::Color::White);

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

            float positionsX[5] = { 150, 400, 650, 275, 525 };
            float positionsY[5] = { 200, 200, 200, 350, 350 };

            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            for (int i = 0; i < 5; i++) {
                // Если все уровни пройдены - все разблокированы
                bool isLocked;
                if (allLevelsCompleted) {
                    isLocked = false; 
                }
                else {
                    isLocked = save.levelsCompleted[i]; // Иначе проверяем пройден ли
                }

                std::unique_ptr<sf::Sprite> button;
                if (isLocked) {
                    button = std::make_unique<sf::Sprite>(levelLockedTex);
                }
                else {
                    button = std::make_unique<sf::Sprite>(levelButtonTex);
                }
                button->setScale({ 0.13f, 0.13f });
                button->setPosition({ positionsX[i], positionsY[i] });

                if (!isLocked && button->getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                    button = std::make_unique<sf::Sprite>(levelButtonDarkTex);
                    button->setScale({ 0.145f, 0.145f });
                    button->setPosition({ positionsX[i] - 11.4f, positionsY[i] - 10.0f });
                }
                window.draw(*button);

                if (!isLocked) {
                    sf::Text levelNum(font);
                    levelNum.setString(std::to_string(i + 1));
                    levelNum.setCharacterSize(30);
                    levelNum.setFillColor(sf::Color::White);
                    sf::FloatRect btnBounds = button->getGlobalBounds();
                    levelNum.setOrigin({ levelNum.getLocalBounds().size.x / 2, levelNum.getLocalBounds().size.y / 2 });
                    levelNum.setPosition({ btnBounds.position.x + btnBounds.size.x / 2, btnBounds.position.y + btnBounds.size.y / 2 });
                    window.draw(levelNum);
                }
                else {

                    sf::Text lockText(font);
                    lockText.setString("passed");
                    lockText.setCharacterSize(24);
                    lockText.setFillColor(sf::Color::Red);
                    sf::FloatRect btnBounds = button->getGlobalBounds();
                    lockText.setOrigin({ lockText.getLocalBounds().size.x / 2, lockText.getLocalBounds().size.y / 2 });
                    lockText.setPosition({ btnBounds.position.x + btnBounds.size.x / 2, btnBounds.position.y + btnBounds.size.y / 2 });
                    window.draw(lockText);
                }
            }

            // Кнопка BACK
            sf::Text backText(font);
            backText.setString("BACK");
            backText.setCharacterSize(30);
            backText.setOrigin({ backText.getLocalBounds().size.x / 2, backText.getLocalBounds().size.y / 2 });
            backText.setPosition({ 500, 600 });

            sf::FloatRect backTextRect;
            backTextRect.position = { 500 - backText.getLocalBounds().size.x / 2, 600 - backText.getLocalBounds().size.y / 2 };
            backTextRect.size = { backText.getLocalBounds().size.x, backText.getLocalBounds().size.y };

            // Преобразуем mousePos в float для проверки
            if (backTextRect.contains(static_cast<sf::Vector2f>(mousePos))) {
                backText.setFillColor(sf::Color(180, 180, 180));
            }
            else {
                backText.setFillColor(sf::Color::White);
            }

            window.draw(backText);

            loadButtonSprite.setPosition({ 750.0f, 500.0f });
            window.draw(loadButtonSprite);

            sf::Text loadText(font);
            loadText.setString("LOAD");
            loadText.setCharacterSize(24);
            loadText.setFillColor(sf::Color::White);
            loadText.setPosition({ 810.f, 550.f });

            window.draw(loadText);
        }

        if (showSaveSlots) {
            sf::RectangleShape overlay(sf::Vector2f(1000.f, 700.f));
            overlay.setFillColor(sf::Color(0, 0, 0, 180));
            window.draw(overlay);

            sf::Text title(font);
            title.setString("SAVE GAME");
            title.setCharacterSize(30);
            title.setFillColor(sf::Color::White);
            title.setOrigin({ title.getLocalBounds().size.x / 2, title.getLocalBounds().size.y / 2 });
            title.setPosition({ 500, 150 });
            window.draw(title);

            sf::Vector2f slotPositions[3] = { {300.f, 200.f}, {300.f, 300.f}, {300.f, 400.f} };
            sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));

            for (int i = 0; i < 3; i++) {
                sf::FloatRect slotRect;
                slotRect.position = slotPositions[i];
                slotRect.size = { 200.f, 60.f };

                sf::RectangleShape slot(sf::Vector2f(200.f, 60.f));
                if (slotRect.contains(mousePos)) {
                    slot.setFillColor(sf::Color(80, 80, 80, 200));
                }
                else {
                    slot.setFillColor(sf::Color(50, 50, 50, 200));
                }
                slot.setOutlineColor(sf::Color::White);
                slot.setOutlineThickness(2.f);
                slot.setPosition(slotPositions[i]);
                window.draw(slot);

                sf::Text slotText(font);
                slotText.setString("Slot " + std::to_string(i + 1));
                slotText.setCharacterSize(20);
                slotText.setFillColor(sf::Color::White);
                slotText.setOrigin({ slotText.getLocalBounds().size.x / 2, slotText.getLocalBounds().size.y / 2 });
                slotText.setPosition({ slotPositions[i].x + 100, slotPositions[i].y + 30 });
                window.draw(slotText);
            }
        }

        if (showLoadSlots) {
            sf::RectangleShape overlay(sf::Vector2f(1000.f, 700.f));
            overlay.setFillColor(sf::Color(0, 0, 0, 180));
            window.draw(overlay);

            sf::Text title(font);
            title.setString("LOAD GAME");
            title.setCharacterSize(30);
            title.setFillColor(sf::Color::White);
            title.setOrigin({ title.getLocalBounds().size.x / 2, title.getLocalBounds().size.y / 2 });
            title.setPosition({ 500, 150 });
            window.draw(title);

            sf::Vector2f slotPositions[3] = { {300.f, 200.f}, {300.f, 300.f}, {300.f, 400.f} };
            sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));

            for (int i = 0; i < 3; i++) {
                sf::FloatRect slotRect;
                slotRect.position = slotPositions[i];
                slotRect.size = { 200.f, 60.f };

                sf::RectangleShape slot(sf::Vector2f(200.f, 60.f));
                if (slotRect.contains(mousePos)) {
                    slot.setFillColor(sf::Color(80, 80, 80, 200));
                }
                else {
                    slot.setFillColor(sf::Color(50, 50, 50, 200));
                }
                slot.setOutlineColor(sf::Color::White);
                slot.setOutlineThickness(2.f);
                slot.setPosition(slotPositions[i]);
                window.draw(slot);

                sf::Text slotText(font);
                slotText.setString("Slot " + std::to_string(i + 1));
                slotText.setCharacterSize(20);
                slotText.setFillColor(sf::Color::White);
                slotText.setOrigin({ slotText.getLocalBounds().size.x / 2, slotText.getLocalBounds().size.y / 2 });
                slotText.setPosition({ slotPositions[i].x + 100, slotPositions[i].y + 30 });
                window.draw(slotText);
            }
        }

        window.display();
    }

    return 0;
}