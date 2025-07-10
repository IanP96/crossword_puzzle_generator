#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#include <gdfontl.h>
#include <gdfontg.h>

// todo clean up code
// todo add comments
// todo make output files go to separate folder

// Number of columns in grid
#define GRID_WIDTH 13
// Number of rows in grid
#define GRID_HEIGHT GRID_WIDTH
#define ALPHABET_SIZE 26
// Length of each word
#define WORD_SIZE 5

// File name of text file containing all words to use
char *ALL_WORDS_FILE_NAME = "all_words.txt";
// Folder name to store output files
char *OUTPUT_FOLDER_NAME = "output";
// Total number of words in text file
int NUM_WORDS = 3103;
// Placeholder for grid cell with no letter in it
char NO_LETTER = '_';
char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
char FIRST_LETTER = 'a';
// Set to true to print debug messages
bool DEBUG = true;
// Placeholder for grid cell with no code number in it
int8_t NO_NUMBER = -1;
// Hide all letters in the key before this threshold. higher value -> harder. Should be between 0 and 26
int8_t BASE_THRESHOLD = ALPHABET_SIZE / 2 + 2;

// Represents a cell in the puzzle grid
typedef struct Cell
{
    char letter;
    int8_t number;
} Cell;

// A direction to place a word in (across or down)
typedef enum Direction
{
    DIR_ACROSS = 0,
    DIR_DOWN
} Direction;

typedef Cell Grid[GRID_HEIGHT][GRID_WIDTH];

/**
 * @brief Reads all words from the text file and puts them in the dest array, no nullterminators used
 *
 * @param dest 2D array to put all of the words in, no null terminators
 */
void load_words(char dest[NUM_WORDS][WORD_SIZE])
{
    FILE *file = fopen(ALL_WORDS_FILE_NAME, "r");
    for (int i = 0; i < NUM_WORDS; i++)
    {
        fread(dest[i], WORD_SIZE, sizeof(char), file);
        fgetc(file); // to get and dispose of newline character
    }
}

/**
 * @brief Places a word in a grid and updates data accordingly
 *
 * @param grid grid to place word in
 * @param x x coordinate of word starting letter
 * @param y y coordinate of word starting letter
 * @param word_to_place word to place in the grid, no null terminator
 * @param remaining_words array of bools with each element corresponding to an element of all_words. Each element is true if the word is still usable (has an unused letter), false otherwise. Array updated in place
 * @param num_remaining_words pointer to number of remaining words, updated in place
 * @param remaining_letters array of 26 booleans. Each element is true if the corresponding letter is used, false otherwise. Array updated in place
 * @param num_remaining_letters pointer to number of remaining letters, updated in place
 * @param all_words array of all words from the text file
 * @param across_or_down which direction to place the word in
 */
void place_word(Grid grid, int x, int y, char word_to_place[WORD_SIZE], bool remaining_words[NUM_WORDS], int *num_remaining_words, bool remaining_letters[ALPHABET_SIZE], int *num_remaining_letters, char all_words[NUM_WORDS][WORD_SIZE], Direction across_or_down)
{

    // insert word into grid
    for (int l = 0; l < WORD_SIZE; l++)
    {
        grid[y][x].letter = word_to_place[l];
        if (across_or_down == DIR_ACROSS)
        {
            x++;
        }
        else
        {
            y++;
        }
    }

    // update remaining letters
    for (int i = 0; i < WORD_SIZE; i++)
    {
        int letter = word_to_place[i] - FIRST_LETTER;
        if (remaining_letters[letter])
        {
            (*num_remaining_letters)--;
            remaining_letters[letter] = false;
        }
    }

    // if there are no remaining letters, we're done here
    if (*num_remaining_letters == 0)
    {
        return;
    }

    // update remaining words, based on which words contain an unused letter
    for (int i = 0; i < NUM_WORDS; i++)
    {
        if (!remaining_words[i])
        {
            continue;
        }
        char *word = all_words[i];
        bool keep = false;
        for (int j = 0; j < WORD_SIZE; j++)
        {
            if (remaining_letters[word[j] - FIRST_LETTER])
            {
                keep = true;
                break;
            }
        }
        if (!keep)
        {
            remaining_words[i] = false;
            (*num_remaining_words)--;
        }
    }
}

/**
 * @brief Returns the index of the nth true element in arr (n starting at 0)
 */
int index_of_nth_true_element(int n, bool arr[])
{
    int found = 0;
    int i;
    for (i = 0; found <= n; i++)
    {
        if (arr[i])
        {
            found++;
        }
    }
    i--;
    return i;
}

/**
 * @brief Return the index of a random true element in an array of bools
 *
 * @param bools the array of bools to find a random true element in
 * @param num_true_elems number of elements that are true in bools
 * @return int index of a random true element in bools
 */
int pick_random_true_elem(bool bools[NUM_WORDS], int num_true_elems)
{
    int choice = arc4random_uniform(num_true_elems);
    // if (DEBUG)
    // {
    //     printf("Random choice was %d\n", choice);
    // }
    return index_of_nth_true_element(choice, bools);
}

/**
 * @brief Checks if a word can be placed in a grid
 *
 * @param grid grid to check
 * @param word_to_place word to test, no null terminator
 * @param dest_x pointer to x coord of first letter. If the word can be placed, this is updated in place
 * @param dest_y pointer to y coord of first letter. If the word can be placed, this is updated in place
 * @param placed_across_or_down pointer to potential direction. If the word can be placed, this is updated in place
 * @return true if word can be placed
 * @return false otherwise
 */
bool can_word_be_placed(Grid grid, char word_to_place[WORD_SIZE], int *dest_x, int *dest_y, Direction *placed_across_or_down)
{
    for (int8_t start_y = 0; start_y < GRID_HEIGHT; start_y++)
    {
        for (int8_t start_x = 0; start_x < GRID_WIDTH; start_x++)
        {
            for (Direction across_or_down = DIR_ACROSS; across_or_down <= DIR_DOWN; across_or_down++)
            {
                // Whether the word matches any existing letters and doesn't touch any other words in the grid (other than words it is intersecting)
                bool valid = true;
                // Whether the word would be tethered to an existing word in the grid
                bool anchored = false;
                for (int l = 0; l < WORD_SIZE && valid; l++)
                {

                    // Get coordinates of this letter
                    int letter_y = across_or_down == 0 ? start_y : start_y + l;
                    if (letter_y >= GRID_HEIGHT)
                    {
                        valid = false;
                        break;
                    }
                    int letter_x = across_or_down == 0 ? start_x + l : start_x;
                    if (letter_x >= GRID_WIDTH)
                    {
                        valid = false;
                        break;
                    }

                    // Check head and tail
                    if (l == 0 || l == WORD_SIZE - 1)
                    {
                        int8_t end_x, end_y;
                        int8_t delta = l == 0 ? -1 : 1;
                        end_x = across_or_down == 0 ? letter_x + delta : letter_x;
                        end_y = across_or_down == 0 ? letter_y : letter_y + delta;
                        bool out_of_bounds = (end_x < 0 || end_x >= GRID_WIDTH || end_y < 0 || end_y >= GRID_HEIGHT);
                        if (!out_of_bounds && grid[end_y][end_x].letter != NO_LETTER)
                        {
                            valid = false;
                            break;
                        }
                    }

                    Cell cell = grid[letter_y][letter_x];
                    if (cell.letter != NO_LETTER)
                    {
                        anchored = true;
                        // if there is already a letter here, new letter must match
                        if (cell.letter != word_to_place[l])
                        {
                            valid = false;
                            break;
                        }
                    }
                    else
                    {
                        // Check adjacent cells
                        for (int8_t delta = -1; delta <= 1; delta += 2)
                        {
                            int8_t adj_x = across_or_down == 0 ? letter_x : letter_x + delta;
                            int8_t adj_y = across_or_down == 0 ? letter_y + delta : letter_y;
                            if (adj_x < 0 || adj_x >= GRID_WIDTH || adj_y < 0 || adj_y >= GRID_HEIGHT)
                            {
                                continue;
                            }
                            if (grid[adj_y][adj_x].letter != NO_LETTER)
                            {
                                valid = false;
                                break;
                            }
                        }
                    }
                }

                if (valid && anchored)
                {
                    *dest_x = start_x;
                    *dest_y = start_y;
                    *placed_across_or_down = across_or_down;
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * @brief Print all letters in a grid
 */
void print_grid_letters(Grid grid)
{
    for (int y = 0; y < GRID_HEIGHT; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            putchar(grid[y][x].letter);
            putchar(' ');
        }
        putchar('\n');
    }
}

/**
 * @brief Creates a crossword grid of letters
 *
 * @param grid grid to populate with letters
 * @return true if successful
 * @return false otherwise
 */
bool create_grid(Grid grid)
{
    if (DEBUG)
    {
        printf("Loading words\n");
    }
    char all_words[NUM_WORDS][WORD_SIZE];
    load_words(all_words);
    if (DEBUG)
    {
        fwrite(all_words[0], sizeof(char), WORD_SIZE, stdout);
        putchar('\n');
        fwrite(all_words[NUM_WORDS - 1], sizeof(char), WORD_SIZE, stdout);
        putchar('\n');
    }

    int num_remaining_words = NUM_WORDS;
    bool remaining_words[NUM_WORDS];
    memset(remaining_words, true, sizeof(remaining_words));
    int num_remaining_letters = ALPHABET_SIZE;
    bool remaining_letters[ALPHABET_SIZE];
    memset(remaining_letters, true, sizeof(remaining_letters));

    // Set initial cell values
    for (int y = 0; y < GRID_HEIGHT; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            grid[y][x].letter = NO_LETTER;
            grid[y][x].number = NO_NUMBER;
        }
    }

    // First word in grid. Pick a random word and place horizontally in middle
    if (DEBUG)
    {
        printf("Placing initial word\n");
    }
    int initial_x = GRID_WIDTH / 2 - WORD_SIZE / 2;
    int initial_y = GRID_HEIGHT / 2;
    int initial_word = pick_random_true_elem(remaining_words, num_remaining_words);
    place_word(grid, initial_x, initial_y, all_words[initial_word], remaining_words, &num_remaining_words, remaining_letters, &num_remaining_letters, all_words, DIR_ACROSS);
    if (DEBUG)
    {
        print_grid_letters(grid);
    }

    // Place remaining words until all letters are used
    while (num_remaining_letters)
    {
        if (DEBUG)
        {
            printf("Looking for next word to that can be placed\n");
        }
        int candidate_word = pick_random_true_elem(remaining_words, num_remaining_words);
        int x;
        int y;
        Direction across_or_down;
        int num_words_checked = 0;
        // Loop until we find a word that can be placed
        while (!remaining_words[candidate_word] || !can_word_be_placed(grid, all_words[candidate_word], &x, &y, &across_or_down))
        {
            candidate_word++;
            if (candidate_word == NUM_WORDS)
            {
                candidate_word = 0;
            }
            num_words_checked++;
            if (num_words_checked > NUM_WORDS)
            {
                if (DEBUG)
                {
                    printf("No more words can be added, this grid creation failed\n");
                }
                return false;
            }
        }

        if (DEBUG)
        {
            printf("Placing word: ");
            fwrite(all_words[candidate_word], sizeof(char), WORD_SIZE, stdout);
            putchar('\n');
        }
        place_word(grid, x, y, all_words[candidate_word], remaining_words, &num_remaining_words, remaining_letters, &num_remaining_letters, all_words, across_or_down);
        if (DEBUG)
        {
            print_grid_letters(grid);
        }
    }

    return true;
}

/**
 * @brief Makes a random sequence of lowercase letters
 *
 * @param letters array to put random letter sequence in
 */
void random_letter_sequence(char letters[ALPHABET_SIZE])
{
    bool remaining[ALPHABET_SIZE];
    memset(remaining, true, sizeof(remaining));
    for (int8_t i = 0; i < ALPHABET_SIZE; i++)
    {
        int choice = pick_random_true_elem(remaining, ALPHABET_SIZE - i);
        char letter = choice + FIRST_LETTER;
        letters[i] = letter;
        remaining[choice] = false;
    }
}

/**
 * @brief Encodes a grid with numbers in some cells
 *
 * @param grid grid to encode
 * @param key letter key
 * @param threshold number of letters to encode in grid. Should be between 0 and 26. Higher value means harder difficulty
 */
void encode_grid(Grid grid, char key[ALPHABET_SIZE], int8_t threshold)
{
    random_letter_sequence(key);
    for (int8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (int8_t x = 0; x < GRID_WIDTH; x++)
        {
            Cell *cell = &(grid[y][x]);
            if ((*cell).letter == NO_LETTER)
            {
                continue;
            }
            for (int8_t letter_num = 0; letter_num < threshold; letter_num++)
            {
                char letter = key[letter_num];
                if (letter == (*cell).letter)
                {
                    (*cell).number = letter_num;
                    break;
                }
            }
        }
    }
}

/**
 * @brief Write completed puzzle to a text file (grid and key)
 *
 * @param file pointer to file to write to
 * @param grid grid of encoded letters
 * @param key key of letters
 * @param show_solutions true to show solutions, false to hide some letter in the key and hide letters in grid
 * @param threshold number of letters encoded. Should be between 0 and 26. Higher value means harder difficulty
 */
void print_puzzle_to_file(FILE *file, Grid grid, char key[ALPHABET_SIZE], bool show_solutions, int8_t threshold)
{
    for (int8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (int8_t x = 0; x < GRID_WIDTH; x++)
        {
            Cell cell = grid[y][x];
            bool show_letter = show_solutions || cell.number == NO_NUMBER;
            if (show_letter)
            {
                fprintf(file, "%3c", cell.letter);
            }
            else
            {
                fprintf(file, "%3d", cell.number);
            }
        }
        fputc('\n', file);
    }
    fputc('\n', file);

    // print key
    for (int8_t i = 0; i < ALPHABET_SIZE; i++)
    {
        fprintf(file, "%2d: ", i);
        bool show_letter = i >= threshold || show_solutions;
        fprintf(file, "%c\n", show_letter ? key[i] : '?');
    }
}

/**
 * @brief Create grid and encode it
 *
 * @param grid grid to populate, updated in place
 * @param key letter key, updated in place
 * @param threshold number of letters to encode. Should be between 0 and 26. Higher value means harder difficulty
 */
void create_puzzle(Grid grid, char key[ALPHABET_SIZE], int8_t threshold)
{
    while (!create_grid(grid))
        ;
    if (DEBUG)
    {
        printf("\nFinal grid:\n");
        print_grid_letters(grid);
    }
    encode_grid(grid, key, threshold);
}

/**
 * @brief Write a finished puzzle to text files
 *
 * @param grid encoded puzzle grid
 * @param key key of letters
 * @param threshold difficulty threshold
 * @param file_name_suffix character to put at end of each filename
 * @param write_puzzle_file true to also write a puzzle file, false to just write a solution file
 */
void write_puzzle_to_text_files(Grid grid, char key[ALPHABET_SIZE], int8_t threshold, char file_name_suffix, bool write_puzzle_file)
{
    if (DEBUG)
    {
        printf("Printing results to files\n");
    }
    char final_solution_name[100];
    sprintf(final_solution_name, "%s/solution%c.txt", OUTPUT_FOLDER_NAME, file_name_suffix);
    FILE *solution_file = fopen(final_solution_name, "w");
    print_puzzle_to_file(solution_file, grid, key, true, threshold);
    fclose(solution_file);

    if (write_puzzle_file)
    {
        char final_puzzle_name[100];
        sprintf(final_puzzle_name, "%s/puzzle%c.txt", OUTPUT_FOLDER_NAME, file_name_suffix);
        FILE *puzzle_file = fopen(final_puzzle_name, "w");
        print_puzzle_to_file(puzzle_file, grid, key, false, threshold);
        fclose(puzzle_file);
    }
    if (DEBUG)
    {
        printf("Writing to text files completed\n");
    }
    
}

/**
 * @brief Create a puzzle and write the puzzle and solution to text files
 *
 * @param threshold difficulty threshold
 * @param file_name_suffix character to put at end of each filename
 */
void run_game_with_text_files(int8_t threshold, char file_name_suffix)
{
    Grid grid;
    char key[ALPHABET_SIZE];
    create_puzzle(grid, key, threshold);

    write_puzzle_to_text_files(grid, key, threshold, file_name_suffix, true);
}

void run_game_with_image(int threshold, char file_name_suffix)
{
    Grid grid;
    char key[ALPHABET_SIZE];
    create_puzzle(grid, key, threshold);

    write_puzzle_to_text_files(grid, key, threshold, file_name_suffix, false);

    FILE *pngout;

    // 8 x 16
    gdFontPtr large_font = gdFontGetLarge();
    // 9 x 15
    gdFontPtr giant_font = gdFontGetGiant();

    int font_width = 10;
    int font_height = 17;

    // Size of each grid cell in pixels
    int CELL_PIXEL_SIZE = 40;

    // Width of grid in pixels
    int width_pixels = CELL_PIXEL_SIZE * GRID_WIDTH;
    // Height of grid in pixels, 3 extra rows for the key
    int height_pixels = CELL_PIXEL_SIZE * (GRID_HEIGHT + 3);
    // Column width of RHS grid for remaining letters
    int missing_letter_column_width = CELL_PIXEL_SIZE * 2 / 3;
    if (DEBUG)
    {
        printf("Creating image\n");
    }
    gdImagePtr img = gdImageCreate(width_pixels + missing_letter_column_width * 3, height_pixels);

    // Set white background. Since this is the first color in a new image, it will be the background colour
    gdImageColorAllocate(img, 255, 255, 255);

    int black = gdImageColorAllocate(img, 0, 0, 0);

    int light_grey_brightness = 250;
    int light_grey = gdImageColorAllocate(img, light_grey_brightness, light_grey_brightness, light_grey_brightness);

    // Draw main grid
    int letter_offset_x = CELL_PIXEL_SIZE / 2 - font_width / 2;
    int letter_offset_y = CELL_PIXEL_SIZE / 2 - font_height / 2;
    int digit_offset_right_x = CELL_PIXEL_SIZE - font_width;
    int digit_offset_y = CELL_PIXEL_SIZE - font_height;
    int digit_offset_left_x = digit_offset_right_x - font_width;
    for (int8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (int8_t x = 0; x < GRID_WIDTH; x++)
        {
            Cell cell = grid[y][x];
            int corner_x = x * CELL_PIXEL_SIZE;
            int corner_y = y * CELL_PIXEL_SIZE;
            int draw_x, draw_y;
            bool draw_cell_outline;
            if (cell.number != NO_NUMBER)
            {
                draw_cell_outline = true;
                // Draw number
                draw_y = corner_y + digit_offset_y;
                int number = cell.number;
                // Draw RHS digit
                draw_x = corner_x + digit_offset_right_x;
                gdImageChar(img, large_font, draw_x, draw_y, number % 10 + '0', black);
                if (number >= 10)
                {
                    // Draw LHS digit
                    draw_x = corner_x + digit_offset_left_x;
                    gdImageChar(img, large_font, draw_x, draw_y, number / 10 + '0', black);
                }
            }
            else if (cell.letter != NO_LETTER)
            {
                draw_cell_outline = true;
                // Draw letter
                draw_x = corner_x + letter_offset_x;
                draw_y = corner_y + letter_offset_y;
                gdImageChar(img, giant_font, draw_x, draw_y, cell.letter, black);
            }
            else
            {
                // Empty cell
                draw_cell_outline = false;
                gdImageFilledRectangle(img, corner_x + 1, corner_y + 1, corner_x + CELL_PIXEL_SIZE, corner_y + CELL_PIXEL_SIZE, light_grey);
            }
            if (draw_cell_outline)
            {
                gdImageRectangle(img, corner_x, corner_y, corner_x + CELL_PIXEL_SIZE, corner_y + CELL_PIXEL_SIZE, black);
            }
        }
    }

    // Draw key
    for (int8_t i = 0; i < ALPHABET_SIZE; i++)
    {
        int corner_x = (i % 13) * CELL_PIXEL_SIZE;
        int corner_y = (GRID_HEIGHT + (i >= 13 ? 2 : 0.5)) * CELL_PIXEL_SIZE;
        gdImageRectangle(img, corner_x, corner_y, corner_x + CELL_PIXEL_SIZE, corner_y + CELL_PIXEL_SIZE, black);
        int draw_x, draw_y;
        if (i < threshold)
        {
            // Hidden
            // Draw number
            draw_y = corner_y + digit_offset_y;
            int number = i;
            // Draw RHS digit
            draw_x = corner_x + digit_offset_right_x;
            gdImageChar(img, large_font, draw_x, draw_y, number % 10 + '0', black);
            if (number >= 10)
            {
                // Draw LHS digit
                draw_x = corner_x + digit_offset_left_x;
                gdImageChar(img, large_font, draw_x, draw_y, number / 10 + '0', black);
            }
        }
        else
        {
            // Show letter
            // Draw letter
            draw_x = corner_x + letter_offset_x;
            draw_y = corner_y + letter_offset_y;
            gdImageChar(img, giant_font, draw_x, draw_y, key[i], black);
        }
    }

    // Draw missing letter grid
    int draw_x = width_pixels + missing_letter_column_width;
    int draw_y = missing_letter_column_width;
    // 0 for left column, 1 for right column
    int8_t column = 0;
    for (char letter = 'a'; letter <= 'z'; letter++)
    {
        bool missing = false;
        for (int8_t i = 0; i < threshold; i++)
        {
            if (key[i] == letter)
            {
                missing = true;
                break;
            }
        }
        if (!missing)
        {
            continue;
        }
        gdImageChar(img, giant_font, draw_x, draw_y, letter, black);
        if (column == 0)
        {
            column = 1;
            draw_x += missing_letter_column_width;
        }
        else
        {
            column = 0;
            draw_y += missing_letter_column_width;
            draw_x -= missing_letter_column_width;
        }
    }

    if (DEBUG)
    {
        printf("Creating final png\n");
    }
    // Open a file for writing. "wb" means "write binary", important under MSDOS, harmless under Unix
    char image_file_name[100];
    sprintf(image_file_name, "%s/puzzle%c.png", OUTPUT_FOLDER_NAME, file_name_suffix);
    pngout = fopen(image_file_name, "wb");

    // Output the image to the disk file in PNG format.
    gdImagePng(img, pngout);
    // Close the file
    fclose(pngout);
    // Destroy the image in memory
    gdImageDestroy(img);
}

int main(void)
{
    char suffixes[] = {'0', '1', '2', '3'};
    int deltas[] = {-1, 0, 0, 1};
    for (int8_t i = 0; i < 4; i++)
    {
        run_game_with_image(BASE_THRESHOLD + deltas[i], suffixes[i]);
    }
    printf("Images and text files created\n");
}
