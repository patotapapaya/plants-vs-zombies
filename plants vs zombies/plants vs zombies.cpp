#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
// Типы
enum class ZombieType { NORMAL, FAST, TOUGH };
enum class PlantType { SHOOTER, WALL, SLOWER };

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
        sprite.setPosition({ startX, startY });
        sprite.setScale({ 0.5f, 0.5f });

        switch (t) {
        case ZombieType::NORMAL:
            health = 3.0f;
            speed = -35.0f;
            originalSpeed = -35.0f;
            sprite.setColor(sf::Color(180, 180, 180));
            break;
        case ZombieType::FAST:
            health = 2.0f;
            speed = -65.0f;
            originalSpeed = -65.0f;
            sprite.setColor(sf::Color(255, 80, 80));
            break;
        case ZombieType::TOUGH:
            health = 8.0f;
            speed = -20.0f;
            originalSpeed = -20.0f;
            sprite.setColor(sf::Color(160, 100, 40));
            sprite.setScale({ 0.6f, 0.6f });
            break;
        }
    }

    void update(float deltaTime) {
        if (!isDead) {
            if (slowTimer > 0) {
                speed = originalSpeed * 0.5f;
                slowTimer -= deltaTime;
                sprite.setColor(sf::Color(100, 200, 255));
            }
            else {
                speed = originalSpeed;
                switch (type) {
                case ZombieType::NORMAL: sprite.setColor(sf::Color(180, 180, 180)); break;
                case ZombieType::FAST: sprite.setColor(sf::Color(255, 80, 80)); break;
                case ZombieType::TOUGH: sprite.setColor(sf::Color(160, 100, 40)); break;
                }
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
            sprite.setScale({ 0.12f, 0.12f });
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
        sprite.setPosition({ startX, startY });
        sprite.setScale({ 0.3f, 0.3f });
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

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    sf::RenderWindow window(sf::VideoMode({ 1000, 700 }), "Zombies vs Plants");
    window.setFramerateLimit(60);

    // Текстуры растений
    sf::Texture shooterTex = createTexture(100, 100, sf::Color(50, 200, 50));
    sf::Texture wallTex = createTexture(100, 100, sf::Color(80, 80, 255));
    sf::Texture slowerTex = createTexture(100, 100, sf::Color(255, 255, 80));

    // Текстуры зомби
    sf::Texture zombieNormTex = createTexture(64, 64, sf::Color(180, 180, 180));
    sf::Texture zombieFastTex = createTexture(64, 64, sf::Color(255, 80, 80));
    sf::Texture zombieToughTex = createTexture(80, 80, sf::Color(160, 100, 40));

    // Текстура пули
    sf::Texture bulletTex = createTexture(16, 16, sf::Color(255, 255, 0));

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
    PlantType selectedPlant = PlantType::SHOOTER;

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

        // События
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
                if (mouse->button == sf::Mouse::Button::Left && !gameOver && !levelComplete && waveActive) {
                    sf::Vector2i pos = sf::Mouse::getPosition(window);
                    int gx = (pos.x / 100) * 100;
                    int gy = (pos.y / 100) * 100;

                    if (gx >= 100 && gx < 900 && gy >= 100 && gy < 600) {
                        bool occupied = false;
                        for (const auto& p : plants) {
                            if (std::abs(p.getX() - static_cast<float>(gx)) < 50 &&
                                std::abs(p.getY() - static_cast<float>(gy)) < 50) {
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

        // Логика
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
                        if (std::abs(z.getY() - p.gridY) < 50) {                        
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

        // Отрисовка
        window.clear(sf::Color(34, 139, 34));

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

        window.display();
    }

    return 0;
}