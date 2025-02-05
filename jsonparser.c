#include "jsonparser.h"

const char *keywords[] = { "false", "true", "null" };

int file_read(const char *filename, char **buffer) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        return 0;
    }
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    *buffer = malloc((size + 1) * sizeof(char));
    rewind(f);
    int ret = fread(*buffer, sizeof(char), size, f);
    (*buffer)[size] = 0;
    fclose(f);
    return ret;
}

char scanner_advance(scanner *scanner) {
    return scanner->source[scanner->current++];
}

int scanner_is_at_end(scanner *scanner) {
    return scanner->current >= strlen(scanner->source);
}

char scanner_peek(scanner *scanner) {
    if (scanner_is_at_end(scanner)) {
        return '\0';
    }
    return scanner->source[scanner->current];
}

char *substring(char *s, int start, int end) {
    int size = end - start;
    char *t = malloc(size + 1);
    strncpy(t, s + start, size);
    t[size] = 0;
    return t;
}

void scanner_error(int line, char *msg) {
    fprintf(stderr, "[line %d]: %s\n", line, msg);
    exit(1);
}

void add_token(scanner *scanner, int token_type) {
    char *text = substring(
        scanner->source, scanner->start, scanner->current
    );
    token token = {
        .type = token_type,
        .lexeme = text,
        .line = scanner->line,
    };
    if (scanner->size >= scanner->capacity) {
        scanner->capacity *= 2;
        scanner->tokens = realloc(
            scanner->tokens, scanner->capacity * sizeof(token)
        );
    }
    scanner->tokens[scanner->size++] = token;
}

void scanner_string(scanner *scanner) {
    while (scanner_peek(scanner) != '"' && !scanner_is_at_end(scanner)) {
        if (scanner_peek(scanner) == '\n') {
            scanner->line++;
        }
        scanner_advance(scanner);
    }
    if (scanner_is_at_end(scanner)) {
        scanner_error(scanner->line, "unterminated string");
    }
    scanner_advance(scanner);
    scanner->start++;
    scanner->current--;
    add_token(scanner, TOKEN_STRING);
    scanner->start--;
    scanner->current++;
}

int isdigit(char c) {
    return c >= '0' && c <= '9';
}

int isalpha(char c) {
    return  (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c == '_');
}

int isalphanum(char c) {
    return isdigit(c) || isalpha(c);
}

char scanner_peeknext(scanner *scanner) {
    if (scanner->current + 1 >= strlen(scanner->source)) {
        return '\0';
    }
    return scanner->source[scanner->current + 1];
}

void number(scanner *scanner) {
    while (isdigit(scanner_peek(scanner))) {
        scanner_advance(scanner);
    }
    if (scanner_peek(scanner) == '.' && isdigit(scanner_peeknext(scanner))) {
        scanner_advance(scanner);
        while (isdigit(scanner_peek(scanner))) {
            scanner_advance(scanner);
        }
    }
    add_token(scanner, TOKEN_NUMBER);
}

int get_keytype(char *key) {
    int nkeys = sizeof(keywords) / sizeof(char *);
    for (int i = 0; i < nkeys; i++) {
        if (!strcmp(key, keywords[i])) {
            return keyoffset + i;
        }
    }
    return -1;
}

void identifier(scanner *scanner) {
    while (isalphanum(scanner_peek(scanner))) {
        scanner_advance(scanner);
    }
    char *text = substring(
        scanner->source, scanner->start, scanner->current
    );
    int type = get_keytype(text);
    if (type == -1) {
        char line[128];
        sprintf(line, "unexpected identifier: %s", text);
        scanner_error(scanner->line, line);
    }
    add_token(scanner, type);
    free(text);
}

void scan_token(scanner *scanner) {
    char c = scanner_advance(scanner);
    switch(c) {
    case '{':
        add_token(scanner, LEFT_BRACE);
        break;
    case '}':
        add_token(scanner, RIGHT_BRACE);
        break;
    case '[':
        add_token(scanner, LEFT_BRACKET);
        break;
    case ']':
        add_token(scanner, RIGHT_BRACKET);
        break;
    case ',':
        add_token(scanner, COMMA);
        break;
    case ':':
        add_token(scanner, COLON);
        break;
    case ' ':
    case '\r':
    case '\t':
        break;
    case '\n':
        scanner->line++;
        break;
    case '"':
        scanner_string(scanner);
        break;
    default:
        if (isdigit(c)) {
            number(scanner);
        } else if (isalpha(c)) {
            identifier(scanner);
        } else {
            char msg[128];
            sprintf(msg, "unexpected character %c", c);
            scanner_error(scanner->line, msg);
        }
        break;
    }
}

void scan_tokens(scanner *scanner) {
    while (!scanner_is_at_end(scanner)) {
        scanner->start = scanner->current;
        scan_token(scanner);
    }
    scanner->start = scanner->current;
    add_token(scanner, TOKEN_EOF);
}

void print_tokens(scanner *scanner) {
    for (int i = 0; i < scanner->size; i++) {
        token token = scanner->tokens[i];
        printf("type: %d\n", token.type);
        printf("lexeme: %s\n", token.lexeme);
        printf("line: %d\n", token.line);
    }
}

token parser_peek(parser *parser) {
    return parser->tokens[parser->current];
}

int parser_is_at_end(parser *parser) {
    return parser_peek(parser).type == TOKEN_EOF;
}

int check(parser *parser, int type) {
    if (parser_is_at_end(parser)) {
        return 0;
    }
    return parser_peek(parser).type == type;
}

token previous(parser *parser) {
    return parser->tokens[parser->current - 1];
}

token parser_advance(parser *parser) {
    if (!parser_is_at_end(parser)) {
        parser->current++;
    }
    return previous(parser);
}

void parser_error(token token, char *msg) {
    fprintf(stderr, "[line %d] at '%s': %s\n", token.line, token.lexeme, msg);
    exit(1);
}

token consume(parser *parser, int type, char *msg) {
    if (check(parser, type)) {
        return parser_advance(parser);
    }
    parser_error(parser_peek(parser), msg);
    return (token){0};
}

void array_add_value(array *array, value *value) {
    if (array->size >= array->capacity) {
        array->capacity *= 2;
        array->elements = realloc(
            array->elements, array->capacity * sizeof(*value)
        );
    }
    array->elements[array->size++] = *value;
}

void object_add_member(object *object, member *member) {
    if (object->size >= object->capacity) {
        object->capacity *= 2;
        object->members = realloc(
            object->members, object->capacity * sizeof(*member)
        );
    }
    object->members[object->size++] = *member;
}

void parse_value(parser *, value *);

void parse_elements(parser *parser, array *array) {
    if (parser_peek(parser).type == RIGHT_BRACKET) {
        return;
    }
    array->capacity = 4;
    array->elements = malloc(4 * sizeof(value));
    value value = {0};
    parse_value(parser, &value);
    array_add_value(array, &value);
    while (parser_peek(parser).type == COMMA) {
        parser_advance(parser);
        parse_value(parser, &value);
        array_add_value(array, &value);
    }
}

void parse_member(parser *parser, member *member) {
    member->value = malloc(sizeof(value));
    token string = consume(parser, TOKEN_STRING, "expected string");
    member->string = strdup(string.lexeme);
    consume(parser, COLON, "expected colon");
    value val = {0};
    parse_value(parser, &val);
    memcpy(member->value, &val, sizeof(value));
}

void parse_members(parser *parser, object *object) {
    if (parser_peek(parser).type == RIGHT_BRACE) {
        return;
    }
    object->capacity = 4;
    object->members = malloc(4 * sizeof(member));
    member member = {0};
    parse_member(parser, &member);
    object_add_member(object, &member);
    while (parser_peek(parser).type == COMMA) {
        parser_advance(parser);
        parse_member(parser, &member);
        object_add_member(object, &member);
    }
}

void parse_object(parser *parser, value *value) {
    value->type = OBJECT;
    object object = {0};
    consume(parser, LEFT_BRACE, "expected left brace");
    parse_members(parser, &object);
    consume(parser, RIGHT_BRACE, "expected right brace");
    value->object = object;
}

void parse_array(parser *parser, value *value) {
    value->type = ARRAY;
    array array = {0};
    consume(parser, LEFT_BRACKET, "expected left bracket");
    parse_elements(parser, &array);
    consume(parser, RIGHT_BRACKET, "expected right bracket");
    value->array = array;
}

void parse_value(parser *parser, value *value) {
    token token = parser_peek(parser);
    enum token_type type = token.type;
    switch (type) {
    case LEFT_BRACE:
        parse_object(parser, value);
        break;
    case LEFT_BRACKET:
        parse_array(parser, value);
        break;
    case TOKEN_STRING:
        parser_advance(parser);
        value->type = VALUE_STRING;
        value->string = strdup(token.lexeme);
        break;
    case TOKEN_NUMBER:
        parser_advance(parser);
        value->type = VALUE_NUMBER;
        value->number = atof(token.lexeme);
        break;
    case TOKEN_TRUE:
        parser_advance(parser);
        value->type = VALUE_TRUE;
        break;
    case TOKEN_FALSE:
        parser_advance(parser);
        value->type = VALUE_FALSE;
        break;
    case TOKEN_NULL:
        value->type = VALUE_NULL;
        break;
    default:
        printf("unexpected token: %s\n", token.lexeme);
        exit(1);
    }
}

void parser_free(parser *parser) {
    for (int i = 0; i < parser->current + 1; i++) {
        free(parser->tokens[i].lexeme);
    }
    free(parser->tokens);
}

void free_value(value *value) {
    switch(value->type) {
    case ARRAY:
        for (int i = 0; i < value->array.size; i++) {
            free_value(&value->array.elements[i]);
        }
        free(value->array.elements);
        break;
    case OBJECT:
        for (int i = 0; i < value->object.size; i++) {
            free(value->object.members[i].string);
            free_value(value->object.members[i].value);
            free(value->object.members[i].value);
        }
        free(value->object.members);
        break;
    case VALUE_STRING:
        free(value->string);
        break;
    }
}

void parse_json(const char *buffer, value *value) {
    scanner scanner = {
        .line = 1,
        .source = (char *) buffer,
        .capacity = 4,
        .tokens = malloc(4 * sizeof(token))
    };
    scan_tokens(&scanner);
    parser parser = {.tokens = scanner.tokens};
    parse_value(&parser, value);
    parser_free(&parser);
}

void string_cat(string *string, char *msg) {
    int size = strlen(msg);
    if (string->size + size >= string->capacity) {
        string->capacity += size;
        string->capacity *= 2;
        string->string = realloc(
            string->string, string->capacity * sizeof(char)
        );
    }
    string->string[string->size] = 0;
    string->size += size;
    strcat(string->string, msg);
}

void print_string(string *string) {
    puts(string->string);
}

void free_string(string *string) {
    free(string->string);
}

void value_string(value *value, string *string, int ind);

void array_string(array *array, string *string, int ind) {
    string_cat(string, "[ ");
    for (int i = 0; i < array->size; i++) {
        value value = array->elements[i];
        value_string(&value, string, ind);
        if (i < array->size - 1) {
            string_cat(string, ", ");
        }
    }
    string_cat(string, " ]");
}

void object_string(object *object, string *string, int ind) {
    char keystr[64] = {0};
    string_cat(string, "{ ");
    if (object->size > 0) {
        string_cat(string, "\n");
    }
    for (int i = 0; i < object->size; i++) {
        member member = object->members[i];
        for (int i = 0; i < ind + 1; i++) {
            string_cat(string, "    ");
        }
        sprintf(keystr, "%s: ", member.string);
        string_cat(string, keystr);
        value_string(member.value, string, ind + 1);
        if (i < object->size - 1) {
            string_cat(string, ",");
        }
        string_cat(string, "\n");
    }
    for (int i = 0; i < ind; i++) {
        string_cat(string, "    ");
    }
    string_cat(string, "}");
}

void value_string(value *value, string *string, int ind) {
    char valstr[64] = {0};
    switch (value->type) {
    case ARRAY:
        array_string(&value->array, string, ind);
        break;
    case OBJECT:
        object_string(&value->object, string, ind);
        break;
    case VALUE_NUMBER:
        sprintf(valstr, "%f", value->number);
        string_cat(string, valstr);
        break;
    case VALUE_STRING:
        sprintf(valstr, "\"%s\"", value->string);
        string_cat(string, valstr);
        break;
    case VALUE_FALSE:
        string_cat(string, "false");
        break;
    case VALUE_TRUE:
        string_cat(string, "true");
        break;
    case VALUE_NULL:
        string_cat(string, "null");
        break;
    }
}