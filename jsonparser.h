#ifndef JSONPARSER

#define JSONPARSER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define keyoffset TOKEN_FALSE

// token types
enum token_type {
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, COLON,
    TOKEN_STRING, TOKEN_NUMBER,
    TOKEN_FALSE, TOKEN_TRUE,
    TOKEN_NULL, TOKEN_EOF
};

// value types
enum value_type {
    OBJECT, ARRAY,
    VALUE_STRING, VALUE_NUMBER,
    VALUE_TRUE, VALUE_FALSE,
    VALUE_NULL
};

// token
typedef struct {
    int type;
    int line;
    char *lexeme;
} token;

// scanner state
typedef struct {
    int start;
    int current;
    int line;
    int capacity;
    int size;
    char *source;
    token *tokens;
} scanner;

// parser state
typedef struct {
    int current;
    token *tokens;
} parser;

typedef struct value value;

// member is a pair of key and value
typedef struct {
    char *string;
    value *value;
} member;

// object is a collection of members
typedef struct {
    int capacity;
    int size;
    member *members;
} object;

// array is a collection of elements
typedef struct {
    int capacity;
    int size;
    value *elements;
} array;

// value can have many types
struct value {
    int type;
    union {
        char *string;
        float number;
        object object;
        array array;
    };
};

// string is a dynamic array of characters
typedef struct {
    int capacity;
    int size;
    char *string;
} string;

// read the file
char *file_read(const char *filename);
// parse json string and store result in value
void parse_json(const char *source, value *value);
// convert value to string, set ind to 0
void value_string(value *value, string *string, int ind);
// print string to standard output
void print_string(string *string);
// free data from the value
void free_value(value *value);
// free data from the string
void free_string(string *string);

#endif