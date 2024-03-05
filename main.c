#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
#include <wchar.h>
#include <stddef.h>

#pragma GCC diagnostic ignored "-Wmultichar"

#define TRANSLATION_SIZE   18
#define MAX_WORD_SIZ       1024

struct {
    char    ascii_char[TRANSLATION_SIZE];
    wchar_t polish_char[TRANSLATION_SIZE];
} TRANSLATION = {
};

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

/* fgetww: copy word from file stream to s. Returns last character read from file stream. */
wchar_t fgetww(wchar_t *s, int n, FILE *fp)
{
    wchar_t c;
    while (n-- || (c = fgetwc(fp)) != ' ' || c != WEOF || c != '\n' || c != ',' || c != '-' || c != '\'' || c != '.')
        *s++ = c;
    *s = '\0';

    return c;
}

void init_translation()
{
    char ascii[] = {'a', 'c', 'e', 'l', 'n', 'o', 's', 'z', 'z', 'A', 'C', 'E', 'L', 'N', 'O', 'S', 'Z', 'Z'};
    wchar_t utf8[] = {'ą', 'ć', 'ę', 'ł', 'ń', 'ó', 'ś', 'ź', 'ż', 'Ą', 'Ć', 'Ę', 'Ł', 'Ń', 'Ó', 'Ś', 'Ź', 'Ż'};
    for (size_t i = 0; i < TRANSLATION_SIZE; i++) {
        TRANSLATION.ascii_char[i] = ascii[i];
        TRANSLATION.polish_char[i] = utf8[i];
    } 
}

int main(int argc, char **argv)
{
    char *dic_name = "polish_pasta.txt";
    FILE *dic_ptr;
    wchar_t word[MAX_WORD_SIZ];

    setlocale(LC_ALL, "C.utf8");

    if ((dic_ptr = fopen(dic_name, "r")) == NULL)
        error("Can't open %s", dic_name);
    
    init_translation();
    // for (int i = 0; i < TRANSLATION_SIZE; i++) {
    //     printf("%c ", TRANSLATION.ascii_char[i]);
    // }
    // printf("\n");
    for (int i = 0; i < TRANSLATION_SIZE; i++) {
        wprintf(L"%lc", TRANSLATION.polish_char[i]);
    }
    // while (fgetww(word, MAX_WORD_SIZ, dic_ptr) != WEOF) {
    //     fwprintf(stdout, word);
    // }

    fclose(dic_ptr);

    return 0;
}