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

#define GRID_WIDTH 13
#define GRID_HEIGHT GRID_WIDTH
#define ALPHABET_SIZE 26
#define WORD_SIZE 5

char *ALL_WORDS_FILE_NAME = "all_words.txt";
char *PUZZLE_FILE_NAME = "puzzle.txt";
char *SOLUTION_FILE_NAME = "solution.txt";
// total number of words in text file
int NUM_WORDS = 3103;

char NO_LETTER = '_';
char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
char FIRST_LETTER = 'a';
bool DEBUG = false;
int8_t NO_NUMBER = -1;
// todo tweak threshold
// hide all letters in key before this threshold. higher value -> harder
int THRESHOLD = ALPHABET_SIZE / 2 + 1;

typedef struct Cell
{
    char letter;
    int8_t number;
} Cell;

typedef Cell Grid[GRID_HEIGHT][GRID_WIDTH];

void load_words(char dest[NUM_WORDS][WORD_SIZE])
{
    FILE *file = fopen(ALL_WORDS_FILE_NAME, "r");
    for (int i = 0; i < NUM_WORDS; i++)
    {
        fread(dest[i], WORD_SIZE, sizeof(char), file);
        fgetc(file); // to get and dispose of newline character
    }
}

void place_word(Grid grid, int x, int y, char word_to_place[WORD_SIZE], bool remaining_words[NUM_WORDS], int *num_remaining_words, bool remaining_letters[ALPHABET_SIZE], int *num_remaining_letters, char all_words[NUM_WORDS][WORD_SIZE], uint8_t across_or_down)
{

    // insert word into grid
    for (int l = 0; l < WORD_SIZE; l++)
    {
        grid[y][x].letter = word_to_place[l];
        if (across_or_down == 0)
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

int pick_random_true_elem(bool bools[NUM_WORDS], int num_true_elems)
{
    int choice = arc4random_uniform(num_true_elems);
    // if (DEBUG)
    // {
    //     printf("Random choice was %d\n", choice);
    // }
    return index_of_nth_true_element(choice, bools);
}

bool can_word_be_placed(Grid grid, char word_to_place[WORD_SIZE], int *dest_x, int *dest_y, uint8_t *placed_across_or_down)
{

    for (int8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (int8_t x = 0; x < GRID_WIDTH; x++)
        {
            for (uint8_t across_or_down = 0; across_or_down <= 1; across_or_down++)
            {
                bool valid = true;
                bool anchored = false;
                for (int l = 0; l < WORD_SIZE && valid; l++)
                {
                    int letter_y = across_or_down == 0 ? y : y + l;
                    if (letter_y >= GRID_HEIGHT)
                    {
                        valid = false;
                        break;
                    }
                    int letter_x = across_or_down == 0 ? x + l : x;
                    if (letter_x >= GRID_WIDTH)
                    {
                        valid = false;
                        break;
                    }

                    // check head and tail
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
                        // check surrounding cells
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
                    *dest_x = x;
                    *dest_y = y;
                    *placed_across_or_down = across_or_down;
                    return true;
                }
            }
        }
    }

    return false;
}

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

bool create_grid(Grid grid)
{
    /*
    initial word
    filter words
    place next word
    filter words again
    repeat until all letters used
    */

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

    for (int y = 0; y < GRID_HEIGHT; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            grid[y][x].letter = NO_LETTER;
            grid[y][x].number = NO_NUMBER;
        }
    }

    // initial word
    if (DEBUG)
    {
        printf("Placing initial word\n");
    }
    int initial_x = GRID_WIDTH / 2 - WORD_SIZE / 2;
    int initial_y = GRID_HEIGHT / 2;
    int initial_word = pick_random_true_elem(remaining_words, num_remaining_words);
    place_word(grid, initial_x, initial_y, all_words[initial_word], remaining_words, &num_remaining_words, remaining_letters, &num_remaining_letters, all_words, 0);
    if (DEBUG)
    {
        print_grid_letters(grid);
    }

    // more words
    while (num_remaining_letters)
    {
        if (DEBUG)
        {
            printf("Looking for next word to that can be placed\n");
        }
        int candidate_word = pick_random_true_elem(remaining_words, num_remaining_words);
        int x;
        int y;
        uint8_t across_or_down;
        int num_words_checked = 0;
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

void encode_grid(Grid grid, char key[ALPHABET_SIZE])
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
            for (int8_t letter_num = 0; letter_num < THRESHOLD; letter_num++)
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

void print_puzzle_to_file(FILE *file, Grid grid, char key[ALPHABET_SIZE], bool show_solutions)
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
        bool show_letter = i >= THRESHOLD || show_solutions;
        fprintf(file, "%c\n", show_letter ? key[i] : '?');
    }
}

void create_puzzle(Grid grid, char key[ALPHABET_SIZE])
{
    while (!create_grid(grid))
        ;
    if (DEBUG)
    {
        printf("\nFinal grid:\n");
        print_grid_letters(grid);
    }
    encode_grid(grid, key);
}

void write_puzzle_to_text_files(Grid grid, char key[ALPHABET_SIZE])
{
    FILE *puzzle_file = fopen(PUZZLE_FILE_NAME, "w");
    FILE *solution_file = fopen(SOLUTION_FILE_NAME, "w");

    if (DEBUG)
    {
        printf("Printing results to files\n");
    }
    print_puzzle_to_file(puzzle_file, grid, key, false);
    print_puzzle_to_file(solution_file, grid, key, true);
    fclose(puzzle_file);
    fclose(solution_file);
}

void run_game_with_text_files(void)
{
    Grid grid;
    char key[ALPHABET_SIZE];
    create_puzzle(grid, key);

    write_puzzle_to_text_files(grid, key);
}

void run_game_with_image(void)
{
    Grid grid;
    char key[ALPHABET_SIZE];
    create_puzzle(grid, key);

    write_puzzle_to_text_files(grid, key);

    // cell 51 x 51
    int CELL_PIXEL_SIZE = 51;

    /* Declare the image */
    gdImagePtr im;
    /* Declare output files */
    FILE *pngout;
    /* Declare color index */
    int black;
    int light_grey;

    // 8 x 16
    gdFontPtr largeFont = gdFontGetLarge();

    // 9 x 15
    gdFontPtr giantFont = gdFontGetGiant();

    /* Allocate the image: pixel sizes */
    // 3 extra rows for the key
    im = gdImageCreate(CELL_PIXEL_SIZE * GRID_WIDTH, CELL_PIXEL_SIZE * (GRID_HEIGHT + 3));
    int width_pixels = CELL_PIXEL_SIZE * GRID_WIDTH;
    int height_pixels = CELL_PIXEL_SIZE * (GRID_HEIGHT + 3);

    /* Allocate the color white
    Since this is the first color in a new image, it will
    be the background color. */
    gdImageColorAllocate(im, 255, 255, 255);

    /* Allocate the color black. */
    black = gdImageColorAllocate(im, 0, 0, 0);

    int light_grey_brightness = 250;
    light_grey = gdImageColorAllocate(im, light_grey_brightness, light_grey_brightness, light_grey_brightness);

    // draw grid lines
    for (int8_t x = 0; x < GRID_WIDTH; x++)
    {
        gdImageLine(im, x * CELL_PIXEL_SIZE, height_pixels - CELL_PIXEL_SIZE * 2, x * CELL_PIXEL_SIZE, height_pixels - 1, black);
    }
    for (int8_t y = GRID_HEIGHT + 1; y < GRID_HEIGHT + 3; y++)
    {
        gdImageLine(im, 0, y * CELL_PIXEL_SIZE, width_pixels - 1, y * CELL_PIXEL_SIZE, black);
    }

    // draw main grid
    int SUBDIVISION_PIXEL_SIZE = CELL_PIXEL_SIZE / 3;
    for (int8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (int8_t x = 0; x < GRID_WIDTH; x++)
        {
            Cell cell = grid[y][x];
            int draw_x, draw_y;
            int corner_x = x * CELL_PIXEL_SIZE, corner_y = y * CELL_PIXEL_SIZE;
            if (cell.number != NO_NUMBER)
            {
                gdImageRectangle(im, corner_x, corner_y, corner_x + CELL_PIXEL_SIZE, corner_y + CELL_PIXEL_SIZE, black);
                // draw number
                draw_y = corner_y + SUBDIVISION_PIXEL_SIZE * 2;
                int number = cell.number;
                if (number >= 10)
                {
                    draw_x = corner_x + SUBDIVISION_PIXEL_SIZE;
                    gdImageChar(im, largeFont, draw_x, draw_y, number / 10 + '0', black);
                    draw_x += SUBDIVISION_PIXEL_SIZE;
                    gdImageChar(im, largeFont, draw_x, draw_y, number % 10 + '0', black);
                }
                else
                {
                    draw_x = corner_x + SUBDIVISION_PIXEL_SIZE * 2;
                    gdImageChar(im, largeFont, draw_x, draw_y, number + '0', black);
                }
            }
            else if (cell.letter != NO_LETTER)
            {
                gdImageRectangle(im, corner_x, corner_y, corner_x + CELL_PIXEL_SIZE, corner_y + CELL_PIXEL_SIZE, black);
                // draw letter
                draw_x = corner_x + SUBDIVISION_PIXEL_SIZE;
                draw_y = corner_y + SUBDIVISION_PIXEL_SIZE;
                gdImageChar(im, giantFont, draw_x, draw_y, cell.letter, black);
            }
            else
            {
                // empty
                // gdImageLine(im, corner_x, corner_y, corner_x + CELL_PIXEL_SIZE, corner_y + CELL_PIXEL_SIZE, black);
                 gdImageFilledRectangle(im, corner_x + 1, corner_y + 1, corner_x + CELL_PIXEL_SIZE, corner_y + CELL_PIXEL_SIZE, light_grey);
            }
        }
    }

    // draw key
    for (int8_t i = 0; i < ALPHABET_SIZE; i++)
    {
        int corner_x = (i % 13) * CELL_PIXEL_SIZE;
        int corner_y = (GRID_HEIGHT + 1 + (i >= 13 ? 1 : 0)) * CELL_PIXEL_SIZE;
        int draw_x, draw_y;
        if (i < THRESHOLD)
        {
            // hidden
            // draw number
            draw_y = corner_y + SUBDIVISION_PIXEL_SIZE * 2;
            int number = i;
            if (number >= 10)
            {
                draw_x = corner_x + SUBDIVISION_PIXEL_SIZE;
                gdImageChar(im, largeFont, draw_x, draw_y, number / 10 + '0', black);
                draw_x += SUBDIVISION_PIXEL_SIZE;
                gdImageChar(im, largeFont, draw_x, draw_y, number % 10 + '0', black);
            }
            else
            {
                draw_x = corner_x + SUBDIVISION_PIXEL_SIZE * 2;
                gdImageChar(im, largeFont, draw_x, draw_y, number + '0', black);
            }
        }
        else
        {
            // show letter
            // draw letter
            draw_x = corner_x + SUBDIVISION_PIXEL_SIZE;
            draw_y = corner_y + SUBDIVISION_PIXEL_SIZE;
            gdImageChar(im, giantFont, draw_x, draw_y, key[i], black);
        }
    }

    /* Open a file for writing. "wb" means "write binary", important
      under MSDOS, harmless under Unix. */
    pngout = fopen("puzzle.png", "wb");

    /* Output the image to the disk file in PNG format. */
    gdImagePng(im, pngout);

    /* Close the files. */
    fclose(pngout);

    /* Destroy the image in memory. */
    gdImageDestroy(im);
}

int main()
{
    run_game_with_image();
    printf("Image and text files created\n");
}