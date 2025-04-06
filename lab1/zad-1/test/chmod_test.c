#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

int main() {
    FILE *file;

    if (chmod("test/test.txt", S_IRUSR) == -1) {
        perror("Nie udało się zmienić uprawnień");
        return 1;
    }
    
    // Próbujemy otworzyć plik do zapisu
    file = fopen("test/test.txt", "w");
    if (file == NULL) {
        if (errno == EACCES) {
            printf("Brak uprawnień. Zmieniam uprawnienia pliku...\n");

            // Ustawienie pełnych uprawnień dla właściciela
            if (chmod("test/test.txt", S_IRUSR | S_IWUSR | S_IXUSR) == -1) {
                perror("Nie udało się zmienić uprawnień");
                return 1;
            }

            // Ponowna próba otwarcia pliku
            file = fopen("test/test.txt", "w");
            if (file == NULL) {
                perror("Nadal brak dostępu do pliku");
                return 1;
            }
        } else {
            perror("Inny błąd przy otwieraniu pliku");
            return 1;
        }
    }

    fprintf(file, "Hello, World!\n");
    fclose(file);

    return 0;
}
