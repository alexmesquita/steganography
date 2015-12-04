#include <err.h>
#include <stdio.h>
#include <stdlib.h>

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

/*
vector<bitset> get_steg_bits(string path, string key, int steg_size)
{
    vector<bitset> steg_bits;

    bitset<8> byte;
}
*/

int main(int argc, char* argv[])
{
    int* key;
    int key_size = 0;
    int i = 0;
    switch (argc)
    {
        case 3:
            key = get_key(argv[2], &key_size);
            break;
        default:
            errx(1, "invalid arguments");
    }

    for (i = 0; i < key_size; ++i)
    {
        printf("%d\n", key[i]);
    }

    free(key);
    return 0;
}
