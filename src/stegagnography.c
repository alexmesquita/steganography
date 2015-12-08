#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define W 1280
#define H 720
#define STEG_BYTES 2073600 /*Quantos bytes modificados no arquivo*/
#define STEG_BITS 4 /*Quantos bits modificados cada byte tem*/

int k = 0, row = 0, col = 0, key_col = 0, key_row = 0;

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

char* get_steg_bytes(char* path, int* key, int key_size, int steg_bytes)
{
    int i = 0, file_position = 0, steg_position = 0;
    char read;
    char* bytes;
    FILE *steg_file;

    steg_file = fopen(path, "r");

    if (steg_file == NULL)
    {
        errx(1, "It was not possible to open the steg file");
    }

    bytes = (char*) malloc(sizeof(char)*steg_bytes + 1);

    for (i = 0; i < steg_bytes; ++i, ++k)
    {
        steg_position = key[k%key_size];
        
        if (steg_position)
        {
            key_col = (steg_position - 1) / 3;

            key_row = (steg_position - 1) % 3;
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
    return bytes;
}

char* generate_steg(char* steg_bytes, int bytes_size, int steg_bits)
{
    int i, j, position, bit;
    char* result_steg;
    char aux = 0;

    result_steg = (char*) malloc((sizeof(char) * bytes_size));

    for (i = 0; i < bytes_size; ++i)
    {
        position = 1;
        aux = 0;
        for (j = 0; j < steg_bits; ++j)
        {
            bit = steg_bytes[i] & position;
            position <<= 1;

            switch (j % 5)
            {
                case 0:
                    if(bit)
                        aux |= 128;
                    break;
                case 1:
                    if(bit)
                        aux |= 64;
                    break;
                case 2:
                    if(bit)
                        aux |= 32;
                    break;
                case 3:
                    if(bit)
                        aux |= 16;
                    break;
                case 4:
                    if(bit)
                        aux |= 8;
                    break;
                default:
                    errx(1, "Error when calculating the bits");
                    break;
            }
        }
        result_steg[i] = aux;
    }
    return result_steg;
}


char* generate_hash(char* hash_bytes, int bytes_size, int steg_bits)
{
    int i, j, h = 0, position, bit;
    char* hash_steg;
    char aux = 0;

    hash_steg = (char*) malloc(sizeof(char) * 16);

    for (i = 0; i < bytes_size; ++i)
    {
        position = 1;
        for (j = 0; j < steg_bits && h < 16; ++j)
        {
            bit = hash_bytes[i] & position;
            position <<= 1;

            switch ((i*steg_bits + j) % 8)
            {
                case 0:
                    if(bit)
                        aux |= 128;
                    break;
                case 1:
                    if(bit)
                        aux |= 64;
                    break;
                case 2:
                    if(bit)
                        aux |= 32;
                    break;
                case 3:
                    if(bit)
                        aux |= 16;
                    break;
                case 4:
                    if(bit)
                        aux |= 8;
                    break;
                case 5:
                    if(bit)
                        aux |= 4;
                    break;
                case 6:
                    if(bit)
                        aux |= 2;
                    break;
                case 7:
                    if(bit)
                        aux |= 1;
                    hash_steg[h] = aux;
                    h++;
                    aux = 0;
                    break;
                default:
                    errx(1, "Error when calculating the bits");
                    break;
            }
        }
    }
    return hash_steg;
}


void save_steg(char* steg, char* file, int size)
{
    FILE *steg_file;
    steg_file = fopen(file, "w");
    if (steg_file != NULL)
    {
        fwrite(steg , 1 , size, steg_file);
    }
    else
    {
        errx(1, "It was not possible to open the result file");
    }
}


int main(int argc, char* argv[])
{
    int* key;
    int key_size = 0;
    char *steg_bytes, *result_steg, *hash_bytes, *hash_steg;
    switch (argc)
    {
        case 3:
            key = get_key(argv[2], &key_size);
            steg_bytes = get_steg_bytes(argv[1], key, key_size, STEG_BYTES);
            result_steg = generate_steg(steg_bytes, STEG_BYTES, STEG_BITS);
            save_steg(result_steg, "res/result.y", STEG_BYTES);
            hash_bytes = get_steg_bytes(argv[1], key, key_size, ceil(128.0 / STEG_BITS));
            hash_steg = generate_hash(hash_bytes, ceil(128.0 / STEG_BITS), STEG_BITS);
            save_steg(hash_steg, "res/hash", 16);
            break;
        default:
            errx(1, "invalid arguments");
    }

    free(key);
    free(steg_bytes);
    free(result_steg);
    return 0;
}
