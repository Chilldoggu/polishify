#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <stddef.h>

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
void make_upper_again(wchar_t *, wchar_t *);
void init_translation();
void translation_loop(char *, char *);
void print_translation_set();
int wstrncmp(wchar_t *, wchar_t *, int);
int is_ascii(wchar_t *);
int compare_trans_sizes(char [], wchar_t []);
int translate_word(wchar_t *, double);
int get_word_size(wchar_t *);
int get_word_byte_size(wchar_t *);
unsigned count_lines(FILE *);
wchar_t fgetww(wchar_t *, int, FILE *);
wchar_t skip_snippet(FILE *);

/* FUNCTION DEFINITIONS */
int main(int argc, char **argv)
{
    char c;
    char *output_name = NULL;
    char *input_name  = NULL;
    char tmp[MAX_WORD_SIZ] = "out_";

    while (--argc > 0 && (*++argv)[0] == '-') {
        char *argument = argv[0];
        while (c = *++argument) {
            switch (c) {
            case 'o':
                output_name = strdup(*++argv);
                --argc;
                break;
            default:
                error("polishify: illegal option %c\n", c);
                argc = 0;
                break;
            }
        }
    }

    if (argc == 1) {
        input_name = strdup(*argv);
    } else {
        printf("Usage: polishify [ -o output_name ] input_file\n");
        return 1;
    }

    if (output_name == NULL)
        output_name = strcat(tmp, input_name);
    
    setlocale(LC_ALL, "C.utf8");

    init_translation();
    // print_translation_set();

    translation_loop(input_name, output_name);

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

void translation_loop(char *in_name, char *out_name)
{
    wchar_t c;
    wchar_t word[MAX_WORD_SIZ];
    wchar_t original[MAX_WORD_SIZ];
    char *dic_name = "input/pl_dict.txt";
    FILE *in_ptr;
    FILE *in_ptr2;
    FILE *out_ptr;
    FILE *dic_ptr;
    double threshold = 0.95;
    int word_pos = 0;
    int word_siz = 0;

    if ((in_ptr = fopen(in_name, "r")) == NULL)
        error("Can't open %s", in_name);
    if ((in_ptr2 = fopen(in_name, "r")) == NULL)
        error("Can't open %s", in_name);
    if ((out_ptr = fopen(out_name, "w")) == NULL)
        error("Can't open %s", out_name);
    if ((dic_ptr = fopen(dic_name, "r")) == NULL)
        error("Can't open %s", dic_name);

    dic_init(dic_ptr);

    // printf("        Before translation | After translation\n");
    // printf("        --------------------------------------\n");
    do {
        c = fgetww(word, MAX_WORD_SIZ, in_ptr);
        // When word is at the end of the file then fgetww sometimes returns '\0', so we check for that. (Bug)
        if (*word) {
            // printf("%26ls | ", word);
            word_siz = get_word_byte_size(word);

            // Translate only ascii words (point of the program)
            if (is_ascii(word)) {
                wstrdup(original, word);
                to_lower(word);
                translate_word(word, threshold);
                make_upper_again(original, word);
            }

            if (c != WEOF) {
                word_pos = ftell(in_ptr) - word_siz - 1;
            } else
                word_pos = ftell(in_ptr) - word_siz;

            while (ftell(in_ptr2) < word_pos) {
                fputwc(fgetwc(in_ptr2), out_ptr);
            }
            fputws(word, out_ptr);
            fseek(in_ptr2, word_siz, SEEK_CUR);
            // printf("%ls\n", word);
        }
    } while (c != WEOF);

    while (ftell(in_ptr2) < ftell(in_ptr)) {
        fputwc(fgetwc(in_ptr2), out_ptr);
    }

    fclose(in_ptr);
    fclose(out_ptr);
    fclose(dic_ptr);
}


void make_upper_again(wchar_t *original, wchar_t *word) {
    for (size_t i = 0; original[i]; i++) {
        if (original[i] >= 'A' && original[i] <= 'Z') {
            if (word[i] > (wchar_t)127) {
                for (size_t j = 0; TRANSLATION.ascii[j]; j++) {
                    if (TRANSLATION.ascii[j] == original[i]) {
                        word[i] = TRANSLATION.unicode[j];
                    }
                }
            } else {
                word[i] -= 'a' - 'A';
            }
        }
    }
}

/* get_word_byte_size: get amount of bytes that it takes to write a word to a file in UTF-8 encoding by comparing UNICODEs. */
int get_word_byte_size(wchar_t *word)
{
    short byte_shift = 8;

    wchar_t c;
    int bytes_siz = 0;
    while (c = *word++) {
        // Outside of known UNICODE codes (at this date and time)
        if ((c >> (byte_shift * 3)) || \
            (c >> (byte_shift * 2)) > 0x10) {
            error("get_word_byte_size(): character '%lc' out of bounds. whole: %0.8x, 4byte: %x, 3byte: %x, test: %0.8x", c, c, c >> (byte_shift * 3), c >> (byte_shift * 2), 0x70 >> 4);
        // Inside 4 byte UTF-8 encoding
        } else if (c >> (byte_shift * 2) <= 0x10 && \
                   c >> (byte_shift * 2) >= 0x01) {
            // printf("4byte: %lc\n", c);
            bytes_siz += 4;
        // Inside 3 byte UTF-8 encoding
        } else if (c >> (byte_shift * 1) <= 0xFF && \
                   c >> (byte_shift * 1) >= 0x08) {
            // printf("3byte: %lc\n", c);
            bytes_siz += 3;
        // Inside 2 byte UTF-8 encoding
        } else if (c >> (byte_shift * 1) <= 0x07 && \
                   c >> (byte_shift * 0) >= 0x80) {
            // printf("2byte: %lc\n", c);
            bytes_siz += 2;
        } else {
            // printf("1byte: %lc\n", c);
            bytes_siz += 1;
        }
    }

    return bytes_siz;
}

void to_lower(wchar_t *word)
{
    for (size_t i = 0; word[i]; i++) {
        if (word[i] >= 'A' && word[i] <= 'Z')
            word[i] += 'a' - 'A';
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