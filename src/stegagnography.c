#include <err.h>
#include <stdio.h>
#include <stdlib.h>

/*#define W 1028*/
#define W 8
#define H 720
#define STEG_SIZE 3

int* get_key(char* path, int* key_size)
{
    FILE *key_file;
    int* key;
    int size;
    char read;
    
    key_file = fopen(path, "r");
    if (key_file != NULL)
    {
        fseek(key_file, 0, SEEK_END);
        size = ftell(key_file);
        fseek(key_file, 0, SEEK_SET);

        key = malloc(sizeof(int)*size);
        while (fscanf(key_file, "%c", &read) != EOF)
        {
            key[*key_size] = read - '0';
            (*key_size)++;
        }
        fclose(key_file);
    }
    else
    {
        errx(1, "It was not possible to open the key file");
    }
    return key;
}


char* get_steg_byts(char* path, int* key, int key_size, int steg_size)
{
    int i = 0, j = 0, row = 0, col = 0, steg_position = 0, file_position = 0, key_col = 0, key_row = 0;
    char read;
    char* bytes;
    FILE *steg_file;

    steg_file = fopen(path, "r");

    if (steg_file == NULL)
    {
        errx(1, "It was not possible to open the steg file");
    }

    bytes = (char*) malloc(sizeof(char)*steg_size + 1);

    for (i = 0, j = 0; i < steg_size; ++i, ++j)
    {
        steg_position = key[j%key_size];
        
        if (steg_position)
        {
            key_col = 0;
            if (steg_position >=4 && steg_position <= 6)
            {
                key_col = 1;
            }
            else if (steg_position >=7 && steg_position <= 9)
            {
                key_col = 2;
            }

            key_row = (steg_position-1) % 3;
            file_position = (row + key_row) * W + col + key_col;

            fseek(steg_file, file_position, SEEK_SET);
            fscanf(steg_file, "%c", &read);

            bytes[i] = read;
        }
        else
        {
            i--;
        }
        
        col += 3;

        if(col + 3 > W)
        {
            col = 0;
            row += 3;
        }
    }
    bytes[steg_size] = '\0';
    return bytes;
}

int main(int argc, char* argv[])
{
    int* key;
    int key_size = 0;
    char *steg;
    switch (argc)
    {
        case 3:
            key = get_key(argv[2], &key_size);
            steg = get_steg_byts(argv[1], key, key_size, STEG_SIZE);
            break;
        default:
            errx(1, "invalid arguments");
    }

    free(key);
    free(steg);
    return 0;
}
