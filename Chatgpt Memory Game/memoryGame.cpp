#include <SDL.h> //header file for the window of the sdl 
#include <SDL_image.h> //header file for the image 
#include <SDL_ttf.h> //header file for the costume fonts
#include <vector> //vector for the position 
#include <algorithm> //for the randomizer
#include <iostream> // standard input output header file for the console 
#include <fstream> //for the reading and writting of the files
#include <cstdlib>
#include <ctime>
#include <string>
#include <random>

const int WIDTH = 1024;
const int HEIGHT = 700;
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

struct CardPair {
    std::string planetImage;
    std::string satelliteImage;
};

struct HighScore {
    int moves;
    Uint32 time; 
};

//caller for the file in the txt
HighScore loadHighScore(const std::string& filename) {
    HighScore highScore = { 0, UINT32_MAX }; // Default high score
    std::ifstream inFile(filename);
    if (inFile.is_open()) {
        inFile >> highScore.moves >> highScore.time;
        inFile.close();
    }
    return highScore;
}

//void updator for the game for new high score
void saveHighScore(const std::string& filename, const HighScore& highScore) {
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << highScore.moves << " " << highScore.time;
        outFile.close();
    }
}

// foor the file names / png for the game
const std::vector<CardPair> cardPairs = {
    {"Parker - Sun.png", "Sun - Parker.png"},
    {"Venus - venera 7.png", "venera 7 - venus .png"},
    {"mercury - mariner .png", "mariner - mercury.png"},
    {"Saturn - Cassini.png", "Cassini - saturn.png"},
    {"neptune - voyager 2.png", "voyager 2 - neptune.png"},
    {"Mars- curiosity.png", "Curiosity - Mars.png"},
    {"Jupiter (Juno) .png", "Juno - Jupiter.png"},
    {"Moon - Apollo.png", "Apollo - moon.png"}
};

// Function declarations
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

        SDL_Surface* planetSurface = IMG_Load(pair.planetImage.c_str());
        SDL_Surface* satelliteSurface = IMG_Load(pair.satelliteImage.c_str());

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

    std::random_device rd; // Random device to seed the random number generator
    std::mt19937 g(rd()); // Mersenne Twister random number generator
    std::shuffle(cards.begin(), cards.end(), g); // Shuffle the cards
}

void drawCards(SDL_Renderer* renderer, const std::vector<Card>& cards, bool showAll) {
    int totalCardWidth = (4 * CARD_SIZE) + (3 * GAP); // 4 cards in a row with gaps
    int totalCardHeight = (2 * CARD_SIZE) + GAP; // 2 rows of cards with a gap

    int startX = (WIDTH - totalCardWidth) / 2; // Center X
    int startY = (HEIGHT - totalCardHeight) / 5; // Center Y   
    startY = std::max(startY, 0);

    for (size_t i = 0; i < cards.size(); ++i) {
        int x = startX + (i % 4) * (CARD_SIZE + GAP);
        int y = startY + (i / 4) * (CARD_SIZE + GAP);

        if (showAll || cards[i].isFlipped || cards[i].isMatched) {
            SDL_Rect cardRect = { x, y, CARD_SIZE, CARD_SIZE };
            SDL_RenderCopy(renderer, cards[i].texture, nullptr, &cardRect);
        }
        else {
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Grey for unflipped cards
            SDL_Rect cardRect = { x, y, CARD_SIZE, CARD_SIZE };
            SDL_RenderFillRect(renderer, &cardRect); // Fill with grey for unflipped cards
        }
    }
}

void displayResult(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, Uint32 elapsedTime, int moves, TTF_Font* font) {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr); // Draw background

    // Center the text horizontally, but move it slightly to the left
    int textX = (WIDTH / 2) - 400; // Adjust -50 to your desired left offset
    int textY = HEIGHT / 2 - 100; // Vertical center offset (you can adjust this as needed)

    std::string resultMessage = "You solved it in " + std::to_string(elapsedTime) + " seconds and " + std::to_string(moves) + " moves!";
    SDL_Color textColor = { 255, 255, 255 }; // White color for text

    // Render the result message at the adjusted position
    renderText(renderer, font, resultMessage, textColor, textX, textY);

    SDL_RenderPresent(renderer);
  //  SDL_Delay(3000); // Show result for 3 seconds   

   // system("pause");
}

void playGame(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, TTF_Font* font, TTF_Font* resultFont) {
    std::vector<Card> cards;
    initializeCards(cards, renderer);

    // Load high score
    HighScore highScore = loadHighScore("highscore.txt");

    // Show all cards for 3 seconds
    Uint32 startTicks = SDL_GetTicks();
    bool showAll = true;

    // Variables for the game logic
    int firstCardIndex = -1;
    int secondCardIndex = -1;
    bool waitingForSecondCard = false;
    Uint32 lastFlipTime = 0;

    // Score and timer management
    int moves = 0;
    Uint32 startTime = SDL_GetTicks();

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;

                // Check if mouse is within card boundaries
                for (size_t i = 0; i < cards.size(); ++i) {
                    int x = (i % 4) * (CARD_SIZE + GAP) + (WIDTH - (4 * CARD_SIZE + 3 * GAP)) / 2;
                    int y = (i / 4) * (CARD_SIZE + GAP) + (HEIGHT - (2 * CARD_SIZE + GAP)) / 5; //for the hitbox aka the corresponding event box of the tile

                    if (mouseX >= x && mouseX <= x + CARD_SIZE &&
                        mouseY >= y && mouseY <= y + CARD_SIZE) {
                        // Only flip if the card is not matched or already flipped
                        if (!cards[i].isMatched && !cards[i].isFlipped && !waitingForSecondCard) {
                            cards[i].isFlipped = true;
                            moves++; // incremaent for the move counter 

                            // Check if it's the first card clicked
                            if (firstCardIndex == -1) {
                                firstCardIndex = i;
                            }
                            else {
                                secondCardIndex = i;
                                waitingForSecondCard = true;
                                lastFlipTime = SDL_GetTicks();

                                // Check for match
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

        // Show all cards for 6 seconds
        Uint32 elapsedTicks = SDL_GetTicks() - startTicks;
        if (elapsedTicks >= 5000) {
            showAll = false;
        }

        // Handle the delay after flipping two cards
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

        // Render cards
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
        drawCards(renderer, cards, showAll);

        // Render the timer
        Uint32 elapsedTime = (SDL_GetTicks() - startTime) / 1000;
        SDL_Color textColor = { 255, 255, 255 };
        std::string timerMessage = "TIME: " + std::to_string(elapsedTime);
        renderText(renderer, font, timerMessage, textColor, WIDTH / 2 - 50, 30);

        // Render move count
        std::string moveMessage = "MOVES: " + std::to_string(moves);
        renderText(renderer, font, moveMessage, textColor, WIDTH / 2 +350 , 10);

        // Render high scores in the top left corner
        std::string HighScore = "HIGH SCORE: " ;
        renderText(renderer, font, HighScore, textColor, 10, 10);

        std::string highScoreMessage = "MOVES: " + std::to_string(highScore.moves);
        renderText(renderer, font, highScoreMessage, textColor, 10, 40); // Positioned in the top left corner

      

        SDL_RenderPresent(renderer);
        SDL_Delay(100);

        // Check for game over condition
        bool allMatched = std::all_of(cards.begin(), cards.end(), [](const Card& card) { return card.isMatched; });
        if (allMatched) {
            if (moves < highScore.moves || (moves == highScore.moves && elapsedTime < highScore.time)) {
                highScore.moves = moves;
                highScore.time = elapsedTime;
                saveHighScore("highscore.txt", highScore);
            }
            displayResult(renderer, backgroundTexture, elapsedTime, moves, resultFont);
        }
    }

    cleanupCards(cards);
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
    srand(static_cast<unsigned>(time(0))); // Seed random number generator

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Memory Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("8bitOperatorPlus-Bold.ttf", 24);
    TTF_Font* resultFont = TTF_OpenFont("8bitOperatorPlus-Bold.ttf", 32);

    SDL_Surface* backgroundSurface = IMG_Load("background.png");
    SDL_Texture* backgroundTexture = SDL_CreateTextureFromSurface(renderer, backgroundSurface);
    SDL_FreeSurface(backgroundSurface);
    std::vector<Card> cards;
    playGame(renderer, backgroundTexture, font, resultFont);

    cleanupCards(cards);
    cleanup(backgroundTexture);
    TTF_CloseFont(font);
    TTF_CloseFont(resultFont);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}


