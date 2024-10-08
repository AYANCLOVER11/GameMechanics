#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

const int WIDTH = 1000;
const int HEIGHT = 600;
const int CARD_SIZE = 110;
const int GAP = 20;
const int NUM_IMAGES = 8; // Total number of unique images
const int FLIP_DELAY = 1000; // Time to show the second card in milliseconds

struct Card {
    SDL_Texture* texture;
    bool isFlipped;
    bool isMatched;
};

// Function declarations
void initializeCards(std::vector<Card>& cards, SDL_Renderer* renderer);
void drawCards(SDL_Renderer* renderer, const std::vector<Card>& cards, bool showAll);
void displayResult(SDL_Renderer* renderer, Uint32 elapsedTime, int moves);
void playGame(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, TTF_Font* font);
void cleanupCards(std::vector<Card>& cards);
void cleanup(SDL_Texture* backgroundTexture);
void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y);

void initializeCards(std::vector<Card>& cards, SDL_Renderer* renderer) {
    // Load images
    std::vector<std::string> imagePaths = {
        "Apollo - moon.png",
        "Cassini - saturn.png",
        "Ceres - dawn.png",
        "Jupiter (Juno) .png",
        "Mars- curiosity.png",
        "neptune - voyager 2.png",
        "Saturn - Cassini.png",
        "Sun - Parker.png"
    };

    // Create pairs of cards
    for (int i = 0; i < NUM_IMAGES; ++i) {
        SDL_Surface* surface = IMG_Load(imagePaths[i].c_str());
        if (surface == nullptr) {
            std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface); // Free the surface after creating the texture

        if (texture == nullptr) {
            std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
            continue;
        }

        cards.push_back({ texture, false, false });
        cards.push_back({ texture, false, false });
    }

    // Shuffle the cards
    std::random_shuffle(cards.begin(), cards.end());
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

void displayResult(SDL_Renderer* renderer, Uint32 elapsedTime, int moves) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    std::string resultMessage = "You solved it in " + std::to_string(elapsedTime) + " seconds and " + std::to_string(moves) + " moves!";

    // Display the result message
    std::cout << resultMessage << std::endl;

    SDL_RenderPresent(renderer);
    SDL_Delay(3000); // Show result for 3 seconds
}

void playGame(SDL_Renderer* renderer, SDL_Texture* backgroundTexture, TTF_Font* font) {
    std::vector<Card> cards;
    initializeCards(cards, renderer);

    // Show all cards for 3 seconds
    Uint32 startTicks = SDL_GetTicks();
    bool showAll = true;

    // Variables for the game logic
    int firstCardIndex = -1; // To store the index of the first flipped card
    int secondCardIndex = -1; // To store the index of the second flipped card
    bool waitingForSecondCard = false; // To prevent flipping more than two cards
    Uint32 lastFlipTime = 0; // Time when the last flip occurred

    // Score and timer management
    int moves = 0; // Track the number of moves
    Uint32 startTime = SDL_GetTicks(); // Start time for the timer

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
                    int y = (i / 4) * (CARD_SIZE + GAP) + (HEIGHT - (2 * CARD_SIZE + GAP)) / 5;

                    if (mouseX >= x && mouseX <= x + CARD_SIZE &&
                        mouseY >= y && mouseY <= y + CARD_SIZE) {
                        // Only flip if the card is not matched or already flipped
                        if (!cards[i].isMatched && !cards[i].isFlipped && !waitingForSecondCard) {
                            cards[i].isFlipped = true;
                            moves++; // Increment moves on each valid flip

                            // Check if it's the first card clicked
                            if (firstCardIndex == -1) {
                                firstCardIndex = i; // Store the index of the first card
                            }
                            else {
                                secondCardIndex = i; // Store the index of the second card
                                waitingForSecondCard = true; // Set waiting for the second card
                                lastFlipTime = SDL_GetTicks(); // Record the time for delay

                                // Check for match
                                if (cards[firstCardIndex].texture == cards[i].texture) { // Compare textures
                                    // It's a match!
                                    cards[firstCardIndex].isMatched = true;
                                    cards[i].isMatched = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Clear the screen and render the background
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);

        // Show all cards for the first 3 seconds
        if (showAll) {
            drawCards(renderer, cards, true);
            if (SDL_GetTicks() - startTicks > 3000) { // Wait for 3 seconds
                showAll = false;
            }
        }
        else {
            drawCards(renderer, cards, false);

            // Handle the flip delay for the second card
            if (waitingForSecondCard && (SDL_GetTicks() - lastFlipTime > FLIP_DELAY)) {
                // Flip back the cards if they don't match
                if (!cards[firstCardIndex].isMatched && !cards[secondCardIndex].isMatched) {
                    cards[firstCardIndex].isFlipped = false; // Flip back the first card
                    cards[secondCardIndex].isFlipped = false; // Flip back the second card
                }
                firstCardIndex = -1; // Reset first card index
                secondCardIndex = -1; // Reset second card index
                waitingForSecondCard = false; // Reset wait state
            }
        }

        // Render the timer
        Uint32 elapsedTime = (SDL_GetTicks() - startTime) / 1000; // Convert to seconds
        SDL_Color textColor = { 255, 255, 255 }; // White color for text
        std::string timerMessage = "TIME: " + std::to_string(elapsedTime) ;
        renderText(renderer, font, timerMessage, textColor, WIDTH / 2 - 50, 20);

        SDL_RenderPresent(renderer);
        SDL_Delay(100);

        // Check for game over condition (all cards matched)
        bool allMatched = std::all_of(cards.begin(), cards.end(), [](const Card& card) {
            return card.isMatched;
            });

        if (allMatched) {
            // Display the final time
            displayResult(renderer, elapsedTime, moves);
        }
    }

    cleanupCards(cards); // Cleanup card textures after the game ends
}

void cleanupCards(std::vector<Card>& cards) {
    for (auto& card : cards) {
        if (card.texture) {
            SDL_DestroyTexture(card.texture);
        }
    }
}

void cleanup(SDL_Texture* backgroundTexture) {
    if (backgroundTexture) {
        SDL_DestroyTexture(backgroundTexture);
    }
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, int x, int y) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface); // Free the surface after creating the texture

    int textWidth = 0, textHeight = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &textWidth, &textHeight);

    SDL_Rect dstRect = { x, y, textWidth, textHeight };
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
    SDL_DestroyTexture(texture); // Clean up texture
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Initialize SDL_image
    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG))) {
        std::cerr << "Failed to initialize SDL_image: " << IMG_GetError() << std::endl;
        return -1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Memory Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Load background texture
    SDL_Texture* backgroundTexture = IMG_LoadTexture(renderer, "space.png");
    if (!backgroundTexture) {
        std::cerr << "Failed to load background texture: " << IMG_GetError() << std::endl;
        return -1;
    }

    // Load font
    TTF_Font* font = TTF_OpenFont("VCR_OSD_MONO.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return -1;
    }

    // Start the game
    playGame(renderer, backgroundTexture, font);

    // Cleanup
    cleanup(backgroundTexture);
    TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
