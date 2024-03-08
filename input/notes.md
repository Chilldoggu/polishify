#### Wstep
Wide char'y sa uzywane do zapisu symboli, ktore wychodza poza tablice ASCII w pojedynczym znaku. Aby program wiedzial w jaki sposob ma interpretowac znaki (do jakiego jezyka przynaleza) musi miec  ustawiony odpowiedni **locale**.

**locale** sa to konwencje, ktore pozwalaja uzytkownikom/programistom na wykorzystanie swojego ulubionego jezyka w programie C. Ogolna nazwa tworzenia oprogramowania, ktore dopasowuje sie do regionu z ktorego pochodza uzytkownicy, nazywa sie **internalizacja**.

W C dany **locale** moze byc ustawiony poprzez modyfikacje zmiennej srodowiskowej. Domyslnie jest ustawiony locale **POSIX**, ktory wynika z amerykanskiego standardu ANSI i korzysta z tablicy ASCII.

---
#### Problem I/O dla wchar'ów
**`wchar_t`** jest typem danych o wielkosci **4 bajtow**. Jego celem jest przechowywanie znakow z calej tablicy Unicode, a **jego wartosc wynosi kod Unicode odpowiadajacy znakowi**. Przy wykonywaniu I/O, funkcje standardowych bibliotek automatycznie mapuja kody Unicode na wartosci UTF-8, dzieki czemu nie musimy przejmowac sie mapowaniem calych zbiorow znakow.

---
#### Jakie sa kowencje danej locale
Locale ma za zadanie definicje m.in.:
1. Ktore wartosci wielobajtowych znakow sa dopuszczalne i w jaki sposob sa intepretowane.
2. Jakie znaki sa alfabetyczne, wielkie i male dla celow konwersji.
3. Jak formatowane sa cyfry oraz waluty.
4. W jakim jezyku wyswietlane sa bledy.
5. Jakiego jezyka uzyc do odpowiedzi tak/nie.

Zadaniem locale nie jest tlumaczenie jednego jezyka na drugi, ale jest to mozliwe w niektorych bibliotekach C.

---
#### Konfiguracja locale
To jakie sa nazwy locale zainstalowanych na twoim systemie mozesz sprawdzic poprzez:
```bash
locale -a
```

Standardowo kazdy system (UNIX) ma zainstalowany locale C lub POSIX (jedno i to samo).

Zmiana locale moze zajsc po przypisaniu wartosci do zmiennej srodowiskowej **`LANG`**. Locale skonfigurowany w taki sposob bedzie uzyty do **wszystkich** celow. Analogicznie mozna tworzyc mieszanke systemowych locale, wykorzystujac jedno locale do formatowania czasu i dat, druga do formatowania waluty i trzecia do uzywanego jezyka. Locale moze byc przypisane do wykonywania pracy dla kategorii:
1. `LC_COLLATE` - zestawianie liter w pojedyncze znaki. Dla przykladu polskie `sz` lub `cz` jest traktowane jako jeden znak.
2. `LC_TYPE` - Konwersja znakow.
3. `LC_MONETARY` - formatowanie waluty.
4. `LC_NUMERIC` - formatowanie wartosci numerycznych ktore nie sa waluta.
5. `LC_TIME` - formatowanie czasu i dat.
6. `LC_MESSAGES` - wiadomosci wyswietlane w interfejsie uzytkownika.
7. `LC_ALL` - uzyj do wszystkich powyzszych kategorii.
8. `LANG` - uzyj do wszystkich kategorii ale moze byc nadpisane przez powyzsze kategorie.

W jezyku C wszystkie zmienne srodowiskowe jak i zainstalowane locale systemowe sa dziedziczone, ale nie uzywane przez funkcje bibliotek poniewaz standard domyslnie uzywa locale `C` (`POSIX`). Z tego powodu wymagane jest manulana konfiguracja zmiennej srodowiskowej uzywanej przez program C przez funkcje **`char *setlocale(int category, char *locale)`**, gdzie rozne `category` sa zdefiniowane w pliku naglowkowym **`<locale.h>`**. `setlocale` zwraca nazwe ustawionej locale.
- Uzycie `setlocale(LC_ALL, "")` spowoduje ustawienie wszystkich kategorii locale programu, na te ktore sa skonfigurowane na systemie.

W celu wypisania specjalnych znakow zdefiniowanych w danym locale musisz uzyc w formatowalnym lancuchu znakow `printf`, znaku `%ls`, ktory przewiduje podanie `wchar_t *`. 

---
#### Przyklady
Przyklad zmiany locale oraz przywrocenie wczesniejszej locale po wykonanej pracy.
```c
#include <stddef.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

void with_other_locale (char *new_locale,
	                    void (*subroutine) (int),
	                    int argument)
{
  char *old_locale, *saved_locale;

  /* Get the name of the current locale.  */
  old_locale = setlocale(LC_ALL, NULL);

  /* Copy the name so it won’t be clobbered by setlocale. */
  saved_locale = strdup(old_locale);
  if (saved_locale == NULL)
    fatal ("Out of memory");

  /* Now change the locale and do some stuff with it. */
  setlocale(LC_ALL, new_locale);
  (*subroutine)(argument);

  /* Restore the original locale. */
  setlocale(LC_ALL, saved_locale);
  free(saved_locale);
}
```

Wyswietlenie wszystkich wielo-bajtowych znakow z tablicy utf-32:
```c
#include <stdio.h>
#include <stddef.h>
#include <locale.h>

int main(int argc, char **argv)
{
    char *new_locale;
    wchar_t c;
    new_locale = setlocale(LC_ALL, "");
    // Mozna rowniez setlocale(LC_ALL, "C.utf8").
    // Pamietaj o sprawdzeniu dostepnych locale komenda `locale -a`.
    wchar_t max = ~(wchar_t)0; 
    for (int i = 0; i < (unsigned)max; i++) {
        c = i;
        (i % 15) ? printf("%lc, ", c) : printf("\n%d. %lc, ", i, c);
    }
    printf("\n");
    return 0;
}
```

Przyklad printowania znakow `wchar_t` z locale `POSIX` i `systemowego`:
```c
#include <stdio.h>
#include <stddef.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char *new_locale, *old_locale;
    wchar_t c;
    
    old_locale = setlocale(LC_ALL, NULL);
    old_locale = strdup(old_locale);
    for (int i = 0; i < 1000; i++) {
        c = i;
        (i % 15) ? printf("%lc, ", c) : printf("\n%d. %lc, ", i, c);
    }
    printf("\n");

    new_locale = setlocale(LC_ALL, "");
    // wchar_t max = ~(wchar_t)0; 
    for (int i = 0; i < 1000; i++) {
        c = i;
        (i % 15) ? printf("%lc, ", c) : printf("\n%d. %lc, ", i, c);
    }
    printf("\n");

    setlocale(LC_ALL, old_locale);
    free(old_locale);
    return 0;
}
```