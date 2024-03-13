#define PATH_MAX 2048
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>


int compare(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}

void dirwalk(const char *dirPath, int depth, bool l, bool d, bool f, bool s) {
    struct dirent **namelist;
    int numEntries;

    if (s == true) {
        numEntries = scandir(dirPath, &namelist, NULL, compare);
    } else {
        numEntries = scandir(dirPath, &namelist, NULL, NULL);
    }

    if (numEntries == -1) {
        perror("scandir");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numEntries; i++) {
        struct dirent *entry = namelist[i];
        char filePath[PATH_MAX];

        snprintf(filePath, PATH_MAX, "%s/%s", dirPath, entry->d_name);

        struct stat sb;
        if (lstat(filePath, &sb) == -1) {
            perror("lstat");
            continue;
        }

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ) {
            continue;
        }

        else if ((l && S_ISLNK(sb.st_mode))) {
            printf("Link: %s\n", entry->d_name);
        }
        else if ((d && S_ISDIR(sb.st_mode))) {
            printf("Dir: %s\n", entry->d_name);
            char* new_path = calloc(strlen(dirPath) + strlen(entry->d_name) + 2,sizeof(char));
            strcpy(new_path, dirPath);
            strcat(new_path, "/");
            strcat(new_path, entry->d_name);
            if (s == true) {
                dirwalk(new_path, depth, l, d, f, s);
            } else {
                dirwalk(new_path, depth++, l, d, f, NULL);
            }
        }
        else if ((f && S_ISREG(sb.st_mode))) {
            printf("File: %s\n", entry->d_name);
        }
        else if (!l && !d && !f) {
            printf("%s\n", entry->d_name);
        }

        free(entry);
    }
    free(namelist);
}

int main(int argc, char *argv[]) {
    int opt;
    bool l = false, d = false, f = false, s = false;
    const char *dirPath = "./";
    setlocale(LC_COLLATE, "");

    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l':
                l = true;
                break;
            case 'd':
                d = true;
                break;
            case 'f':
                f = true;
                break;
            case 's':
                s = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [dir] [options]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        dirPath = argv[optind];
    }

    printf("l = %d \n", l);
    printf("d = %d \n", d);
    printf("f = %d \n", f);
    printf("s = %d \n", s);

    dirwalk(dirPath, 0, l, d, f, s);

    return 0;
}