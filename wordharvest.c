/********************************************
 *   Searches for files and extract words
 * from them to create a list of keywords
 * that can be used in a dictionary attack
 ********************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>

#define MAX_EXTENSIONS 5
#define MAX_LENGTH_NAME 6

char extensions[MAX_EXTENSIONS][MAX_LENGTH_NAME + 1];
FILE* output = NULL;

int search_files(const char *root, const char *ext);

int can_handle_pdf()
{
    return access("/usr/local/bin/pdftotext", F_OK | X_OK)
            || access("/usr/bin/pdftotext", F_OK | X_OK)
            || access("/bin/pdftotext", F_OK | X_OK)
            || access("/usr/local/sbin/pdftotext", F_OK | X_OK)
            || access("/usr/sbin/pdftotext", F_OK | X_OK);
}

int can_handle_doc()
{
    return 0; /* TODO */
}

int init_allowed_extensions()
{
    int i;
    /* these extensions are always allowed */
    int allowed = 3;
    strcpy(extensions[0], "txt");
    strcpy(extensions[1], "text");
    strcpy(extensions[2], "asc");
    if (can_handle_pdf())
        strcpy(extensions[allowed++], "pdf");
    if (can_handle_doc())
        strcpy(extensions[allowed++], "doc");
    for (i = allowed; i < MAX_EXTENSIONS; i++)
        extensions[i][0] = '\0';
    return allowed;
}

int is_this_extension_allowed(char* extension)
{
    int i;
    for (i = 0; i < MAX_EXTENSIONS; i++)
        if (0 == strcmp(extension, extensions[i]))
            return 1;
    return 0;
}

int parse_e_option(char* opt_value)
{
    char extension[MAX_LENGTH_NAME];
    int i = 0;
    int k;

    if (NULL == opt_value)
        return 0;
    while (opt_value[i] != '\0') {
        for (k = 0;
                k < MAX_LENGTH_NAME && opt_value[i + k] != '\0'
                        && opt_value[i + k] != ':'; k++)
            extension[k] = opt_value[i + k];
        extension[k] = '\0';

        if (!is_this_extension_allowed(extension)) {
            printf("ERROR: Extension ''%s'' not allowed.\n", extension);
            return 0;;
        }

        if (':' == opt_value[i + k])
            i = i + k + 1;
        else
            i = i + k;

    }
    return 1;
}

/* check if directory exist */
int parse_d_option(const char* optarg)
{
    DIR* dir = opendir(optarg);
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}

int parse_o_option(char* optarg)
{
    output = fopen(optarg, "w");
    if (NULL == output)
        return 0;
    return 1;
}

void print_allowed_extensions()
{
    int i;
    printf("Allowed extensions: ");
    for (i = 0; i < MAX_EXTENSIONS; i++) {
        printf("%s ", extensions[i]);
    }
    printf("\n");
}

int parse_options(int argc, char **argv)
{
    int eflag = 0;
    int dflag = 0;
    int oflag = 0;
    char *value = NULL;
    int index;
    int ret = 0;
    int c;

    opterr = 0;
    /* wordharvest -e txt:text:asc -d /tmp/ -o words_tmp */
    while ((c = getopt(argc, argv, "e:d:o:")) != -1) {
        switch (c) {
        case 'e':
            eflag = parse_e_option(optarg);
            break;
        case 'd':
            dflag = parse_d_option(optarg);
            break;
        case 'o':
            oflag = parse_o_option(optarg);
            break;
        case '?':
            fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            return 1;
        default:
            ret = -1;
        }

    }

    if (3 != eflag + dflag + oflag) {
        printf("Usage: wordharvest -e <extension0>:<extension1>:..:"
                "<extensionN> -d <dir> -o <output file>\n");
        print_allowed_extensions();
        printf("Check if the directory passed with -d option exists\n");
        ret = -1;
    } else {
        printf("options OK\n");
        ret = 0;
    }

    return ret;
}

int main(int argc, char **argv)
{
    int number_allowed_extensions;
    char buffer_extensions[MAX_LENGTH_NAME];
    number_allowed_extensions = init_allowed_extensions();
    parse_options(argc, argv);

    fclose(output);
    return 0;
}

int search_files(const char *root, const char *ext)
{
    DIR *dp;
    regex_t reg;
    struct dirent *ep;
    char *full_dir;
    char *reg_ext;

    dp = opendir(root);

    asprintf(&reg_ext, "^.*\\.(%s)$", ext);

    if (regcomp(&reg, reg_ext, REG_EXTENDED | REG_NOSUB) != 0)
        return -2;

    free(reg_ext);

    if (dp != NULL) {
        while (ep = readdir(dp)) {
            asprintf(&full_dir, "%s/%s", root, ep->d_name);

            if (ep->d_type == DT_DIR) {

                if ((strcmp(ep->d_name, ".") != 0)
                        && (strcmp(ep->d_name, "..") != 0)) {
                    printf("DIR: %s\n", full_dir);
                    search_files(full_dir, ext);
                }
            } else {
                if ((regexec(&reg, ep->d_name, 0, (regmatch_t *) NULL, 0))
                        == 0) {
                    printf("FILE MATCH: %s\n", full_dir);
                } else
                    printf("FILE: %s\n", full_dir);
            }

            free(full_dir);
        }

        (void) closedir(dp);
    } else
        fprintf(stderr, "Couldn't open the directory: %s\n", root);

    return 0;
}

