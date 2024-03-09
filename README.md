Make you text files written in ASCII to UNICODE.

```bash
# Usage
polishify [ -o output_filename ] input_filename
```

If you want to translate an array of files you could use for example:
```bash
#!/bin/bash

# Define the array of filenames
filenames=('wide char, locale, internalizacja.md' 'Ellipsis.md' 'Zpredefiniowane nazwy.md' 'Union.md' 'malloc.md' 'fsize - rekursywne wyswietlanie rozmiarow kartoteki.md' 'Low-level funkcje.md' 'flushing buffer.md')

# Iterate through each filename in the array
for file in "${filenames[@]}"; do
    # Check if the file exists
    if [ -e "$file" ]; then
        echo "Starting translation of file '$file'"
        # If the file exists, pass it to the polishify program
        polishify "$file"
    else
        # If the file doesn't exist, print an error message
        echo "Error: File '$file' not found."
    fi
done
```
