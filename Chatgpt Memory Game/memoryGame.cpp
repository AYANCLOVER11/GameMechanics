#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <random>

const int WIDTH = 1024;
const int HEIGHT = 768;
const int CARD_SIZE = 120;
const int GAP = 26;
const int NUM_IMAGES = 8;
const int FLIP_DELAY = 1000;

struct Card {
    SDL_Texture* texture;
    bool isFlipped;
    bool isMatched;
    int pairId;
};

struct HighScore {
    std::string playerName;
    int moves;
    Uint32 time;
};

std::vector<HighScore> loadLeaderboard(const std::string& filename) {
    std::vector<HighScore> leaderboard;
    std::ifstream inFile(filename);
    if (inFile.is_open()) {
        HighScore score;
        while (inFile >> score.playerName >> score.moves >> score.time) {
            leaderboard.push_back(score);
        }
        inFile.close();
    }
    return leaderboard;
}


void saveLeaderboard(const std::string& filename, const std::vector<HighScore>& leaderboard) {
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        for (const auto& score : leaderboard) {
            outFile << score.playerName << " " << score.moves << " " << score.time << "\n";
        }
        outFile.close();
    }
}

const std::vector<std::pair<std::string, std::string>> cardPairs = {
    {"blue - earth.png", "earth - blue.png"},
    {"hell - venus.png", "Venus - hell.png"},
    {"uranus - tilted.png", "tilted - ur anus.png"},
    {"Saturn - icy rings.png", "icy rings - saturn.png"},
    {"caloris - mercury.png", "mercury - caloris .png"},
    {"Jupiter - red spot.png", "red spot - jupiter.png"},
    {"Mars- olympus.png", "olympus - mars.png"},
    {"neptune - dark spot.png", "dark spot - neptune.png"}
};

void initializeCards(std::vector<Card>& cards, SDL_Renderer* renderer);
void drawCards(SDL_Renderer* renderer, const std::vector<Card>& cards, bool showAll);
void displayResult(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, Uint32 elapsedTime, int moves, TTF_Font* font);
void playGame(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, TTF_Font* font, TTF_Font* resultFont);
void cleanupCards(std::vector<Card>& cards);
void cleanup(SDL_Texture* backgroundTexture);
void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y);

void initializeCards(std::vector<Card>& cards, SDL_Renderer* renderer) {
    for (size_t i = 0; i < cardPairs.size(); ++i) {
        const auto& pair = cardPairs[i];

        SDL_Surface* planetSurface = IMG_Load(pair.first.c_str());
        SDL_Surface* satelliteSurface = IMG_Load(pair.second.c_str());

        if (planetSurface == nullptr || satelliteSurface == nullptr) {
            std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
            continue;
        }

        SDL_Texture* planetTexture = SDL_CreateTextureFromSurface(renderer, planetSurface);
        SDL_Texture* satelliteTexture = SDL_CreateTextureFromSurface(renderer, satelliteSurface);
        SDL_FreeSurface(planetSurface);
        SDL_FreeSurface(satelliteSurface);

        if (planetTexture == nullptr || satelliteTexture == nullptr) {
            std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
            continue;
        }

        cards.push_back({ planetTexture, false, false, static_cast<int>(i) }); // Planet card
        cards.push_back({ satelliteTexture, false, false, static_cast<int>(i) }); // Satellite card
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(cards.begin(), cards.end(), g);
}

void drawCards(SDL_Renderer* renderer, const std::vector<Card>& cards, bool showAll) {
    int totalCardWidth = (4 * CARD_SIZE) + (3 * GAP);
    int totalCardHeight = (2 * CARD_SIZE) + GAP;

    int startX = (WIDTH - totalCardWidth) / 2;
    int startY = (HEIGHT - totalCardHeight) / 5;
    startY = std::max(startY, 0);

    for (size_t i = 0; i < cards.size(); ++i) {
        int x = startX + (i % 4) * (CARD_SIZE + GAP);
        int y = startY + (i / 4) * (CARD_SIZE + GAP);

        if (showAll || cards[i].isFlipped || cards[i].isMatched) {
            SDL_Rect cardRect = { x, y, CARD_SIZE, CARD_SIZE };
            SDL_RenderCopy(renderer, cards[i].texture, nullptr, &cardRect);
        }
        else {
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_Rect cardRect = { x, y, CARD_SIZE, CARD_SIZE };
            SDL_RenderFillRect(renderer, &cardRect);
        }
    }
}

void displayResult(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, Uint32 elapsedTime, int moves, TTF_Font* font) {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
    std::vector<HighScore> leaderboard = loadLeaderboard("leaderboard.txt");

    int textX = (WIDTH / 2) - 350;
    int textY = HEIGHT / 2 - 40;
    SDL_Color textColor = { 255, 255, 255 };
    int yOffset = HEIGHT / 2 - 40;
    std::string leaderboardTOP = "LEADERBOARD";
    renderText(renderer, font, leaderboardTOP, textColor, textX + 250 , textY - 300);
    for (const auto& score : leaderboard) {
        std::string leaderboardEntry = score.playerName + ": " + std::to_string(score.moves) + " moves, " + std::to_string(score.time) + " seconds";
        renderText(renderer, font, leaderboardEntry, textColor, textX + 20, yOffset - 230  );
        yOffset += 30;
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(5000);
}

bool playAgain() {
    std::string response;
    while (true) {
        std::cout << "Do you want to play again? (y/n): ";
        std::cin >> response;

        if (response == "y" || response == "Y") {
            return true;
        }
        else if (response == "n" || response == "N") {
            return false;
        }
        else {
            std::cout << "Invalid input. Please enter 'y' or 'n'." << std::endl;
        }
    }
}

std::string getInputText(SDL_Renderer* renderer, TTF_Font* font, SDL_Texture* backgroundTexture) {
    std::string inputText;
    bool inputActive = true;
    SDL_StartTextInput(); // Start text input

    while (inputActive) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                inputActive = false;
                break;
            }
            if (event.type == SDL_TEXTINPUT) {
                inputText += event.text.text; // Add new text
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && !inputText.empty()) {
                    inputText.pop_back(); // Remove last character
                }
                if (event.key.keysym.sym == SDLK_RETURN) {
                    inputActive = false; // Finish input on Enter
                }
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr); // Use your background
        SDL_Color textColor = { 255, 255, 255 };
        renderText(renderer, font, "Enter your name:", textColor, WIDTH / 2 - 100, HEIGHT / 2 - 120);
        renderText(renderer, font, inputText, textColor, WIDTH / 2 - 50, HEIGHT / 2);

        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    SDL_StopTextInput(); // Stop text input
    return inputText;
}


void playGame(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, TTF_Font* font, TTF_Font* resultFont) {
    std::vector<HighScore> leaderboard = loadLeaderboard("leaderboard.txt");
    bool gameRunning = true;

    while (gameRunning) {
        std::vector<Card> cards;
        initializeCards(cards, renderer);

        // Show all cards for 5 seconds before starting the game
        Uint32 showAllStart = SDL_GetTicks();
        bool showAll = true;

        while (showAll) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    gameRunning = false;
                    showAll = false;
                }
            }

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
            drawCards(renderer, cards, true);
            SDL_RenderPresent(renderer);

            // Stop showing cards after 5 seconds
            if (SDL_GetTicks() - showAllStart >= 5000) {
                showAll = false;
            }
        }

        // Now start the main game loop
        Uint32 startTime = SDL_GetTicks();
        bool timerStopped = false;
        int moves = 0;

        // Declare indices for the selected cards here
        int firstCardIndex = -1;
        int secondCardIndex = -1;
        bool waitingForSecondCard = false;
        Uint32 lastFlipTime = 0;

        while (true) {
            SDL_Event event;
            bool running = true;

            while (running) {
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        running = false;
                        gameRunning = false;
                    }

                    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                        int mouseX = event.button.x;
                        int mouseY = event.button.y;

                        for (size_t i = 0; i < cards.size(); ++i) {
                            int x = (i % 4) * (CARD_SIZE + GAP) + (WIDTH - (4 * CARD_SIZE + 3 * GAP)) / 2;
                            int y = (i / 4) * (CARD_SIZE + GAP) + (HEIGHT - (2 * CARD_SIZE + GAP)) / 5;

                            if (mouseX >= x && mouseX <= x + CARD_SIZE &&
                                mouseY >= y && mouseY <= y + CARD_SIZE) {
                                if (!cards[i].isMatched && !cards[i].isFlipped && !waitingForSecondCard) {
                                    cards[i].isFlipped = true;
                                    moves++;

                                    if (firstCardIndex == -1) {
                                        firstCardIndex = i;
                                    }
                                    else {
                                        secondCardIndex = i;
                                        waitingForSecondCard = true;
                                        lastFlipTime = SDL_GetTicks();

                                        if (cards[firstCardIndex].pairId == cards[secondCardIndex].pairId) {
                                            cards[firstCardIndex].isMatched = true;
                                            cards[secondCardIndex].isMatched = true;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }

                // Handle waiting for second card logic
                if (waitingForSecondCard) {
                    if (SDL_GetTicks() - lastFlipTime >= FLIP_DELAY) {
                        if (!cards[firstCardIndex].isMatched) {
                            cards[firstCardIndex].isFlipped = false;
                        }
                        if (!cards[secondCardIndex].isMatched) {
                            cards[secondCardIndex].isFlipped = false;
                        }
                        firstCardIndex = -1;
                        secondCardIndex = -1;
                        waitingForSecondCard = false;
                    }
                }

                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
                drawCards(renderer, cards, false); // Show cards based on their state

                Uint32 elapsedTime = (SDL_GetTicks() - startTime) / 1000;
                SDL_Color textColor = { 255, 255, 255 };
                std::string timerMessage = "TIME: " + std::to_string(elapsedTime);
                renderText(renderer, font, timerMessage, textColor, WIDTH / 2 - 50, 30);

                std::string moveMessage = "MOVES: " + std::to_string(moves);
                renderText(renderer, font, moveMessage, { 255, 255, 255 }, WIDTH / 2 + 350, 10);

                SDL_RenderPresent(renderer);
                SDL_Delay(100);

                // Check if all cards are matched
                bool allMatched = std::all_of(cards.begin(), cards.end(), [](const Card& card) { return card.isMatched; });
                if (allMatched) {
                    if (!timerStopped) {
                        timerStopped = true;
                    }

                    // Get player name using text input
                    std::string playerName = getInputText(renderer, font, backgroundTexture);


                    HighScore newScore = { playerName, moves, elapsedTime };
                    leaderboard.push_back(newScore);

                    std::sort(leaderboard.begin(), leaderboard.end(), [](const HighScore& a, const HighScore& b) {
                        return a.moves < b.moves || (a.moves == b.moves && a.time < b.time);
                        });
                    saveLeaderboard("leaderboard.txt", leaderboard);
                    displayResult(renderer, backgroundTexture, elapsedTime, moves, resultFont);

                    if (!playAgain()) {
                        gameRunning = false;
                    }
                    break;
                }
            }

            cleanupCards(cards);
        }
    }
}

void cleanupCards(std::vector<Card>& cards) {
    for (auto& card : cards) {
        SDL_DestroyTexture(card.texture);
    }
}

void cleanup(SDL_Texture* backgroundTexture) {
    SDL_DestroyTexture(backgroundTexture);
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    SDL_Rect dstRect = { x, y, width, height };
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
    SDL_DestroyTexture(texture);
}



int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(0)));

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Memory Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("8bitOperatorPlus-Bold.ttf", 24);
    TTF_Font* resultFont = TTF_OpenFont("8bitOperatorPlus-Bold.ttf", 32);

    SDL_Surface* backgroundSurface = IMG_Load("welcome bg.png");
    SDL_Texture* backgroundTexture = SDL_CreateTextureFromSurface(renderer, backgroundSurface);
    SDL_FreeSurface(backgroundSurface);

    playGame(renderer, backgroundTexture, font, resultFont);

    cleanup(backgroundTexture);
    TTF_CloseFont(font);
    TTF_CloseFont(resultFont);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
