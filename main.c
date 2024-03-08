#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
#include <wchar.h>
#include <stddef.h>

#pragma GCC diagnostic ignored "-Wmultichar"

#define MAX_WORD_SIZ    1024
#define DEBUG           1

/* GLOBALS */
struct trans {
    char    *ascii;
    wchar_t *unicode;
} TRANSLATION;

// typedef struct word_similarity {
//     wchar_t dic_word;
//     double similarity;
// } word_sim;

struct dic {
    wchar_t **dic;
    long unsigned lines;
} DIC = { NULL, 0 };



/* PROTOTYPES */
void error(char *, ...);
void wstrdup(wchar_t *, wchar_t *);
void dic_init(FILE *);
void to_lower(wchar_t *);
void print_dic();
void turn_ascii(char *, int, wchar_t *);
void init_translation();
void translation_loop();
void print_translation_set();
int wstrncmp(wchar_t *, wchar_t *, int);
int is_ascii(wchar_t *);
int compare_trans_sizes(char [], wchar_t []);
int translate_word(wchar_t *, double);
int get_word_size(wchar_t *);
unsigned count_lines(FILE *);
wchar_t fgetww(wchar_t *, int, FILE *);
wchar_t skip_snippet(FILE *);

/* FUNCTION DEFINITIONS */
int main(int argc, char **argv)
{
    setlocale(LC_ALL, "C.utf8");

    init_translation();
    // print_translation_set();

    translation_loop();

    /* CLEAN */
    free(TRANSLATION.ascii);
    free(TRANSLATION.unicode);
    for (size_t i = 0; i < DIC.lines; i++) {
        free(DIC.dic[i]);
    }
    free(DIC.dic);
    
    return 0;
}

void print_dic()
{
    for (size_t i = 0; i < DIC.lines; i++) {
        printf("%d. %ls\n", i+1, DIC.dic[i]);
    }
}

void translation_loop()
{
    wchar_t c;
    wchar_t word[MAX_WORD_SIZ];
    char *in_name = "input/notes.md";
    char *out_name = "out.txt";
    char *dic_name = "input/pl_dict.txt";
    FILE *in_ptr;
    FILE *out_ptr;
    FILE *dic_ptr;
    double threshold = 0.95;

    if ((in_ptr = fopen(in_name, "r")) == NULL)
        error("Can't open %s", in_name);
    if ((out_ptr = fopen(out_name, "w")) == NULL)
        error("Can't open %s", out_name);
    if ((dic_ptr = fopen(dic_name, "r")) == NULL)
        error("Can't open %s", dic_name);

    dic_init(dic_ptr);
    // print_dic();

    printf("        Before translation | After translation\n");
    printf("        --------------------------------------\n");
    do {
        c = fgetww(word, MAX_WORD_SIZ, in_ptr);
        // When word is at the end of the file then fgetww sometimes returns '\0', so we check for that. (Bug)
        if (*word) {
            // printf("is ascii?: %d | %ls\n", is_ascii(word), word);

            printf("%26ls | ", word);
            // Translate only ascii words (point of the program)
            if (is_ascii(word)) {
                to_lower(word);
                translate_word(word, threshold);
            }
            printf("%ls\n", word);
        }
    } while (c != WEOF);

    fclose(in_ptr);
    fclose(out_ptr);
    fclose(dic_ptr);
}

void to_lower(wchar_t *word)
{
    for (size_t i = 0; word[i]; i++)
    {
        if (*word >= 'A' && *word <= 'Z')
            *word += 'a' - 'A';
    }
    
}

unsigned count_lines(FILE *fp)
{
    wchar_t c;
    unsigned lines = 0;
    while ((c = fgetwc(fp)) != WEOF)
        if (c == '\n')
            lines++; 
    lines++; // for WEOF

    if (fseek(fp, 0L, SEEK_SET) != 0)
        error("Fseek error");

    return lines;
}

void wstrdup(wchar_t *dst, wchar_t *src)
{
    while (*src) *dst++ = *src++;
    *dst = '\0';
}

void dic_init(FILE *dic_ptr)
{
    wchar_t max_line[MAX_WORD_SIZ];
    wchar_t *p;
    unsigned word_cnt = 0;

    DIC.lines = count_lines(dic_ptr);
    DIC.dic = (wchar_t**)malloc(sizeof(wchar_t*)*DIC.lines);
    
    for (size_t i = 0; i < DIC.lines; i++) {
        fgetws(max_line, MAX_WORD_SIZ, dic_ptr);
        p = max_line;
        word_cnt = 0;

        // Count line len and delete trailing '\n'
        while (*p != '\n' && *p != '\0') {
            word_cnt++;
            p++;
        }
        *p = '\0';

        DIC.dic[i] = (wchar_t *)malloc(sizeof(wchar_t)*(word_cnt+1)); // word_cnt+1 to make space for '\0'
        wstrdup(DIC.dic[i], max_line);
    }

    if (fseek(dic_ptr, 0L, SEEK_SET) != 0)
        error("Fseek error");
}

int get_word_size(wchar_t *word)
{
    int size = 0;
    for (size_t i = 0; word[i]; i++)
        size++; 
    return size;
}

/* translate_word: translate ascii word only if it's likelihood is inside threshold. Translated word overwrites `word` argument, returns -1 otherwise */
int translate_word(wchar_t *word, double threshold)
{
    wchar_t result[MAX_WORD_SIZ];
    char dic_word[MAX_WORD_SIZ];
    int word_size = get_word_size(word);
    double max_similarity = 0;
    double word_similarity = 0;

    // Iterate through all dictionary and compare word to each entry.
    for (size_t i = 0; i < DIC.lines; i++) {
        // Don't even bother if dictionary entry isn't the same size as word of interest.
        if (get_word_size(DIC.dic[i]) != word_size)
            continue;

        turn_ascii(dic_word, MAX_WORD_SIZ, DIC.dic[i]);

        // Calculate how simmilar the word is to ascii dictionary entry.
        int occurence = 0;
        for (size_t i = 0; word[i]; i++) {
            if (word[i] == dic_word[i])
                occurence++;
        }
        word_similarity = occurence/word_size;

        if (word_similarity > max_similarity) {
            max_similarity = word_similarity;
            wstrdup(result, DIC.dic[i]); // Copy original dictionary entry into result.
        } 

        // Nothing will be larger than 100% similarity
        if (max_similarity == 1) {
            break;
        }
    }

    // If word is unlikely to be translated
    if (max_similarity < threshold)
        return -1;

    // printf("%ls ", result);
    wstrdup(word, result);
    return 1;
}

void turn_ascii(char *buf, int n, wchar_t *w_word)
{
    if (n < 0)
        error("turn_ascii() n < 0");
    
    while (n-- && *w_word) {
        // If not ascii then translate
        if (*w_word > (wchar_t)127) {
            // Check wheter the character is in translation set, if yes then translate
            size_t i = 0;
            for (; TRANSLATION.unicode[i]; i++) {
                if (TRANSLATION.unicode[i] == *w_word) {
                    *buf = TRANSLATION.ascii[i];
                    break;
                }
            }
            if (TRANSLATION.unicode[i] == '\0')
                error("turn_ascii() character not in translation set.");
        } else
            *buf = *w_word;

        buf++;
        w_word++;
    }
    if (n == 0)
        error("turn_ascii() word out of bounds");
    *buf = '\0'; // Make sure to check our buf is in bounds before zero terminating
}

int is_ascii(wchar_t *word)
{
    while (*word) {
        if (*word < (wchar_t)0 || *word > (wchar_t)127)
            return 0;
        word++;
    }
    return 1;
}

void error(char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "[ERROR] ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");

    va_end(args);

	exit(1);
}

wchar_t skip_snippet(FILE *fp)
{
    wchar_t c;
    int cnt;
    while ((c = fgetwc(fp)) != WEOF) {
        if (c == L'`') cnt++;
        else cnt = 0;
        if (cnt == 3) break;
    }
    return c;
}

/* wstrncmp: compare first N characters from wchar_t string. */
int wstrncmp(wchar_t *s1, wchar_t *s2, int n)
{
    if (n < 1) {
        error("wstrncmp(wchar_t *, wchar_t *, int) doesn't accept n < 1.");
    }

    while (--n && *s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }

    if (*s1 != *s2) {
        return 0; // false
    }

    return 1; // true
}

/* 
fgetww: Copy word from file stream to s. Returns last character read from file stream. It also ignores code blocks that are often part of the markdown notes.
        There is an off by one error when reading last word, when it's right before WEOF or not word character(s), so you need (so far) make sure that this function doesn't return empty word.
        (Too complex should be rewritten)
*/
wchar_t fgetww(wchar_t *s, int n, FILE *fp)
{
    wchar_t c;
    wchar_t *base = s;

    // Make sure character is either alphabetic or ` or unicode (very bad, should be improved).
    while (n-- && (c = fgetwc(fp)) != ' ' && c != WEOF && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '`' || c > 127))
        *s++ = c;
    if (base == s && c != WEOF) // Ensure there are no empty words.
        c = fgetww(base, MAX_WORD_SIZ, fp);
    else 
        *s = '\0';

    if (wstrncmp(base, L"```", 3)) { // Must be snippet
        skip_snippet(fp);
        return fgetww(base, MAX_WORD_SIZ, fp);
    }

    if (wstrncmp(base, L"`", 1)) { // Not code snippet so must be code block (too complex, improve)
        wchar_t *p = base;
        int cnt = 0;

        while (p != s)
            if (*p++ == L'`') 
                cnt++;

        switch (cnt) {
        case 1:
            while ((c = fgetwc(fp)) != '`');
            break;
        
        case 2:
            break;

        default:
            error("inside fgetww() - word contains more than two '`'");
            break;
        }

        return fgetww(base, MAX_WORD_SIZ, fp);
    }

    return c;
}

int compare_trans_sizes(char ascii[], wchar_t unicode[])
{
    char *ptr1 = ascii;
    wchar_t *ptr2 = unicode;
    int size = 0;

    while (*ptr1 != '\0' && *ptr2 != '\0') {
        ptr1++;
        ptr2++;
        size++;
    }
    
    if (*ptr1 != '\0' || *ptr2 != '\0') {
        return -1;
    }

    return size;
}

void init_translation()
{
    int trans_size;
    char ascii[] = { 'a', 'c', 'e', 'l', 'n', 'o', 's', 'z', 'z', 'A', 'C', 'E', 'L', 'N', 'O', 'S', 'Z', 'Z', '\0' };
    wchar_t unicode[] = { L'ą', L'ć', L'ę', L'ł', L'ń', L'ó', L'ś', L'ź', L'ż', L'Ą', L'Ć', L'Ę', L'Ł', L'Ń', L'Ó', L'Ś', L'Ź', L'Ż', '\0'};

    if ((trans_size = compare_trans_sizes(ascii, unicode)) == -1)
        error("Translation ascii and unicode arrays do not overlap!");

    TRANSLATION.ascii = (char *)malloc(trans_size);
    TRANSLATION.unicode  = (wchar_t *)malloc(sizeof(wchar_t)*trans_size);
    for (int i = 0; i < trans_size; i++) {
        TRANSLATION.ascii[i] = ascii[i];
        TRANSLATION.unicode[i]  = unicode[i];
    }
}

void print_translation_set()
{
    printf("%10s  |%12s\n", "ASCII", "UNICODE");
    printf("-------------------------\n");
    for (int i = 0; TRANSLATION.ascii[i]; ++i) {
        printf("%lc : 0x%04x  |  %lc : 0x%04x\n", TRANSLATION.ascii[i], (unsigned int)TRANSLATION.ascii[i], TRANSLATION.unicode[i], (unsigned int)TRANSLATION.unicode[i]);
    }
}

/*
================= GRAVEYARD ================= 

unsigned dic_max_word(FILE *dic_ptr)
{
    wchar_t max_line[MAX_WORD_SIZ];
    long unsigned lines = 0;
    while (fgetws(max_line, MAX_WORD_SIZ, dic_ptr)) {
        lines++;
    }
    return 0;
}

*/