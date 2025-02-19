#include "jsonparser.h"

int main(int argc, char **argv) {
    // check usage
    if (argc != 2) {
        printf("usage: %s [file.json]\n", argv[0]);
        return 1;
    }
    // read the file from the first argument
    char *source = file_read(argv[1]);
    if (!source) {
        return 1;
    }
    // parse the json file and store it as a value
    value value = {0};
    parse_json(source, &value);
    // convert value to string
    string string = {
        .capacity = 64,
        .string = malloc(64)
    };
    value_string(&value, &string, 0);
    // print the string to the standard output
    print_string(&string);
    // free all data
    free_value(&value);
    free_string(&string);
    free(source);
    return 0;
}