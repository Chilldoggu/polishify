#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
#include <wchar.h>
#include <stddef.h>

#pragma GCC diagnostic ignored "-Wmultichar"

#define MAX_WORD_SIZ       1024

/* GLOBALS */
struct trans{
    char    *ascii;
    wchar_t *unicode;
} TRANSLATION;

/* PROTOTYPES */
void error(char *, ...);
wchar_t fgetww(wchar_t *, int, FILE *);
int compare_trans_sizes(char [], wchar_t []);
void init_translation();
void print_translation_set();
int wstrncmp(wchar_t *, wchar_t *, int);
wchar_t skip_snippet(FILE *);
int is_ascii(wchar_t *);
void translation_loop();

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

    return 0;
}

void translation_loop()
{
    wchar_t c;
    wchar_t word[MAX_WORD_SIZ];
    char *dic_name = "notes.md";
    char *out_name = "out.txt";
    FILE *dic_ptr;
    FILE *out_ptr;

    if ((dic_ptr = fopen(dic_name, "r")) == NULL)
        error("Can't open %s", dic_name);
    if ((out_ptr = fopen(out_name, "w")) == NULL)
        error("Can't open %s", out_name);

    do {
        c = fgetww(word, MAX_WORD_SIZ, dic_ptr);
        if (*word) {
            printf("is ascii?: %d | %ls\n", is_ascii(word), word);
        }
    } while (c != WEOF);

    fclose(dic_ptr);
    fclose(out_ptr);
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
fgetww: copy word from file stream to s. Returns last character read from file stream.
        There is an off by one error when reading last word, when it's right before WEOF or not word character(s),
        so you need (so far) make sure that this function doesn't return empty word.
*/
wchar_t fgetww(wchar_t *s, int n, FILE *fp)
{
    wchar_t c;
    wchar_t *base = s;

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

    if (wstrncmp(base, L"`", 1)) { // Not code snippet so must be code block
        wchar_t *p = base;
        int cnt = 0;

        while (p != s)
            if (*p++ == L'`') cnt++;

        switch (cnt) {
        case 1:
            while ((c = fgetwc(fp)) != '`');
            break;
        
        case 2:
            break;

        default:
            error("inside fgetww() - word contains more than more than two '`'");
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

    if ((trans_size = compare_trans_sizes(ascii, unicode)) == -1) {
        error("Translation ascii and unicode arrays do not overlap!");
    }

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