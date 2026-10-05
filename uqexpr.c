#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tinyexpr.h>

#define MAX_VAR_NAME_LENGTH 24 // Maximum length for variable name
#define DEFAULT_SIG_FIGURES 3 // Default significant figures
#define MIN_SIG_FIGURES 2 // Min significant figures
#define MAX_SIG_FIGURES 9 // Max significant figures
#define EXIT_CODE_17 17 // error(17)
#define EXIT_CODE_6 6 // error(6)
#define EXIT_CODE_9 9 // error(9)
#define MAX_BUFFER 1000 // Max buffer size of inputs
#define RANGE_INPUT 7 // @range command's expression position

// Structure for variable
typedef struct {
    char name[MAX_VAR_NAME_LENGTH];
    double value;
} Variable;

// Structure for loop variable
typedef struct {
    char name[MAX_VAR_NAME_LENGTH];
    double value;
    double start, increment, end;
} LoopVariable;

// Initialise global variables
typedef struct {
    Variable* variables;
    LoopVariable* loopVariables;
    int numVariables;
    int numLoopVariables;
    int significantFigures;
    char* file;
} GlobalVar;

// Initialise function first to ensure everything's loaded without error.
void parse_arguments(int argc, char* argv[], GlobalVar* gv);
void usage_error_message(void);
void startup_message(void);
void invalid_variables(void);
void duplicate_variables(void);
void handle_initialise(GlobalVar* gv, char* arg);
void handle_looping(GlobalVar* gv, char* arg);
void handle_significant_figures(GlobalVar* gv, char* arg);
int is_valid_variable_name(const char* name);
int is_valid_value(const char* value);
int is_duplicate_variable(GlobalVar* gv, const char* name);
int is_duplicate_loop_variable(GlobalVar* gv, const char* name);
void print_variables(GlobalVar* gv);
void print_loop_variables(GlobalVar* gv);
void print_thank_you(void);
void free_memory(GlobalVar* gv);
void handle_user_input(GlobalVar* gv);
void remove_variable(GlobalVar* gv, const char* name);
void validate_range(const char* name, const char* tempStart,
        const char* tempIncrement, const char* tempEnd);
void handle_range_command(GlobalVar* gv, char* rangeArg);
void update_or_create_loop(GlobalVar* gv, const char* name, double start,
        double increment, double end, int loopVarIndex);
void process_expression(GlobalVar* gv, char* expression);
char* extract_variable_name(char* expression, char** remain);
int find_variable_index(GlobalVar* gv, const char* varName);
int find_loop_variable_index(GlobalVar* gv, const char* varName);
void add_variable(GlobalVar* gv, const char* varName);
void add_te_variables(GlobalVar* gv, te_variable* teVars, double* teValues,
        char teNames[][MAX_VAR_NAME_LENGTH]);
void evaluate(GlobalVar* gv, char* expression, char* varName, int storeResult);
void handle_file(GlobalVar* gv);

/* parse_arguments()
 *
 * This function processes command-line arguments for:
 * --initilise
 * --looping
 * --signficant figures
 * Reading from files
 *
 * Exits the program if arguments are invalid
 *
 * REF: this code references the output i got from gemini
 */
void parse_arguments(int argc, char* argv[], GlobalVar* gv)
{
    // Checks for empty arguments
    if (argc == 2 && strcmp(argv[1], "") == 0) {
        usage_error_message();
    }

    // Checks for arguments and handle each accordingly
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--initialise") == 0) {
            if (i + 1 < argc) {
                handle_initialise(gv, argv[++i]);
            } else {
                usage_error_message();
            }
        } else if (strcmp(argv[i], "--looping") == 0) {
            if (i + 1 < argc) {
                handle_looping(gv, argv[++i]);
            } else {
                usage_error_message();
            }
        } else if (strcmp(argv[i], "--significantfigures") == 0) {
            if (i + 1 < argc) {
                handle_significant_figures(gv, argv[++i]);
            } else {
                usage_error_message();
            }
        } else if (argv[i][0] != '-') {
            // Handle input file, and check if its the last argument
            if (i == argc - 1) {
                gv->file = argv[i];

                FILE* file = fopen(gv->file, "r");
                if (file == NULL) {
                    fprintf(stderr, "uqexpr: can't read from file \"%s\"\n",
                            gv->file);
                    free_memory(gv);
                    free(gv);
                    exit(1);
                }
            } else {
                usage_error_message();
            }
        } else if (argv[i][0] == '-') {
            usage_error_message();
        } else {
            return;
        }
    }
}

/* usage_error_message()
 *
 * Display usage error and exit with error(17)
 */
void usage_error_message()
{
    fprintf(stderr,
            "Usage: ./uqexpr [--significantfigures 2..9] [--looping string] "
            "[--initialise string] [inputfilename]\n");
    exit(EXIT_CODE_17);
}

/* startup_message()
 *
 * Prints a start up message
 */
void startup_message()
{
    printf("Welcome to uqexpr.\ns4938368 wrote this program.\n");
}

/* invalid_variables()
 *
 *  Display invalid variable error and exit with error(6)
 */
void invalid_variables()
{
    fprintf(stderr, "uqexpr: invalid variable(s) were specified\n");
    exit(EXIT_CODE_6);
}

/* duplicate_variables()
 *
 * Display duplicate variable name error and exit with error(9)
 */
void duplicate_variables()
{
    fprintf(stderr, "uqexpr: duplicate variables were specified\n");
    exit(EXIT_CODE_9);
}

/* handle_initialise()
 *
 * Parses and validates a variables initialization argument
 * Creates a new variable with the given name and value
 */
void handle_initialise(GlobalVar* gv, char* arg)
{
    // Count number of '=' present in argument
    // REF: this portion of code is references from my interaction with gemini
    int numEquals = 0;
    for (int i = 0; arg[i] != '\0'; i++) {
        if (arg[i] == '=') {
            numEquals++;
        }
    }

    // Ensure only one '=' is present
    if (numEquals > 1) {
        invalid_variables();
    }
    char* name = strtok(arg, "=");
    char* tempValue = strtok(NULL, "=");

    // Check for duplicates during parsing
    if (is_duplicate_variable(gv, name)
            || is_duplicate_loop_variable(gv, name)) {
        duplicate_variables();
    }

    // Validation
    if (!name || !tempValue || !is_valid_value(tempValue)
            || !is_valid_variable_name(name)) {
        invalid_variables();
    }
    double value = atof(tempValue);

    gv->variables
            = realloc(gv->variables, (gv->numVariables + 1) * sizeof(Variable));
    if (gv->variables == NULL) {
        fprintf(stderr, "memory allocation failed\n");
        exit(1);
    }
    // Storing variable
    strcpy(gv->variables[gv->numVariables].name, name);
    gv->variables[gv->numVariables].value = value;
    gv->numVariables++;
}

/* handle_looping()
 *
 * Parses and validates a loop variable initialization argument
 * Creates a new loop variable with given values
 */
void handle_looping(GlobalVar* gv, char* arg)
{
    // parses the argument for : name, start, increment, and end
    char* name = strtok(arg, ",");
    char* tempStart = strtok(NULL, ",");
    char* tempIncrement = strtok(NULL, ",");
    char* tempEnd = strtok(NULL, ",");

    // Checks for duplicate variable name
    if (is_duplicate_variable(gv, name)
            || is_duplicate_loop_variable(gv, name)) {
        duplicate_variables();
    }

    // Validation
    if (!name || !tempStart || !tempIncrement || !tempEnd
            || !is_valid_value(tempStart) || !is_valid_value(tempIncrement)
            || !is_valid_value(tempEnd) || !is_valid_variable_name(name)) {
        invalid_variables();
    }

    // Convert argument into numbers
    double start = atof(tempStart);
    double increment = atof(tempIncrement);
    double end = atof(tempEnd);

    if ((start < end && increment <= 0) || (start > end && increment >= 0)) {
        invalid_variables();
    }

    gv->loopVariables = realloc(gv->loopVariables,
            (gv->numLoopVariables + 1) * sizeof(LoopVariable));

    // Storing loop variables
    strcpy(gv->loopVariables[gv->numLoopVariables].name, name);
    gv->loopVariables[gv->numLoopVariables].start = start;
    gv->loopVariables[gv->numLoopVariables].increment = increment;
    gv->loopVariables[gv->numLoopVariables].end = end;
    gv->loopVariables[gv->numLoopVariables].value = start;
    gv->numLoopVariables++;
}

/* handle_significant_figures()
 *
 * Validates and sets the significant figures
 * Exits with usage error if value is out of range
 */
void handle_significant_figures(GlobalVar* gv, char* arg)
{
    // Reject arguments starting with '0'
    if (arg[0] == '0' && arg[1] != '\0') {
        usage_error_message();
    }

    // Convert argument into int
    gv->significantFigures = atoi(arg);

    // Validation
    if (gv->significantFigures < MIN_SIG_FIGURES
            || gv->significantFigures > MAX_SIG_FIGURES) {
        usage_error_message();
    }
}

/* is_valid_variable_name()
 *
 * Checks if the variable name:
 * - is between 1 and MAX_VAR_NAME_LENGTH - 1
 * - contains only alphabets
 *
 * returns 1 if valid, 0 if invalid
 */
int is_valid_variable_name(const char* name)
{
    int length = strlen(name);
    // Check length limit
    if (length < 1 || length > MAX_VAR_NAME_LENGTH - 1) {
        return 0;
    }
    // Check if all characters are alphabetic
    for (int i = 0; i < length; i++) {
        if (!isalpha(name[i])) {
            return 0;
        }
    }
    return 1;
}

/* is_valid_value()
 *
 * Uses strtod to validate if the entire string can be converted into
 * a number
 *
 * returns 1 if valid, 0 if invalid
 *
 * REF: The following code references from
 * www.tutorialspoint.com/c_standard_library/c_function_strtod.htm
 */
int is_valid_value(const char* value)
{
    char* i;
    strtod(value, &i);

    // Check if the pointer of the endpoint is the same as value or if the
    // endpoint is not null.
    if (i == value || *i != '\0') {
        return 0; // Not a number
    }

    return 1; // Valid number
}

/* is_duplicate_variable()
 *
 * searches through existing variables to find duplicates
 *
 * returns 1 if found, 0 if not found
 */
int is_duplicate_variable(GlobalVar* gv, const char* name)
{
    for (int i = 0; i < gv->numVariables; i++) {
        if (strcmp(gv->variables[i].name, name) == 0) {
            return 1; // Duplicate found
        }
    }
    return 0;
}

/* is_duplicate_loop_variable()
 *
 * searches through existing loop variables to find duplicates
 *
 * returns 1 if found, 0 if not found
 */
int is_duplicate_loop_variable(GlobalVar* gv, const char* name)
{
    for (int i = 0; i < gv->numLoopVariables; i++) {
        if (strcmp(gv->loopVariables[i].name, name) == 0) {
            return 1; // Duplicate found
        }
    }
    return 0;
}

/* print_variables()
 *
 * Print all non-loop variables
 * Displays the name and value of each variable
 * Uses the significant figures for output
 */
void print_variables(GlobalVar* gv)
{
    if (gv->numVariables > 0) {
        printf("Variables:\n");
        for (int i = 0; i < gv->numVariables; i++) {
            printf("%s = %.*g\n", gv->variables[i].name, gv->significantFigures,
                    gv->variables[i].value);
        }
    } else {
        printf("No variables were defined.\n");
    }
}

/* print_loop_variables()
 *
 * Print all loop variables
 * Displays the name and value of each loop variable
 * Uses the significant figures for output
 */
void print_loop_variables(GlobalVar* gv)
{
    if (gv->numLoopVariables > 0) {
        printf("Loop variables:\n");
        for (int i = 0; i < gv->numLoopVariables; i++) {
            printf("%s = %.*g (%.*g, %.*g, %.*g)\n", gv->loopVariables[i].name,
                    gv->significantFigures, gv->loopVariables[i].value,
                    gv->significantFigures, gv->loopVariables[i].start,
                    gv->significantFigures, gv->loopVariables[i].increment,
                    gv->significantFigures, gv->loopVariables[i].end);
        }
    } else {
        printf("No loop variables were specified.\n");
    }
}

/* print_thank_you()
 *
 * Print a thank you message when program ends
 */
void print_thank_you()
{
    printf("Thank you for using uqexpr.\n");
}

/* free_memory()
 *
 * Free dynamically allocated memory for variable and loop variable
 */
void free_memory(GlobalVar* gv)
{
    free(gv->variables);
    free(gv->loopVariables);
}

/* handle_user_input()
 *
 * Reads expression and assignment operations from stdin
 * Process each line until user exits program
 */
void handle_user_input(GlobalVar* gv)
{
    printf("Please enter your expressions and assignment operations to be "
           "evaluated.\n");

    char input[MAX_BUFFER];
    while (1) {
        // read input from stdin
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // end of file detected
            break;
        }

        // remove newline from input
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
            len--;
        }

        // checks if empty or a comment
        if (len == 0 || input[0] == '#') {
            continue;
        }

        // Process the input
        process_expression(gv, input);
    }
}

/* remove_variable()
 *
 * Finds and remove a variables with specified name
 * Reallocates memory to adjust the variables array
 */
void remove_variable(GlobalVar* gv, const char* name)
{
    // find matching name
    for (int i = 0; i < gv->numVariables; i++) {
        if (strcmp(gv->variables[i].name, name) == 0) {
            // Shift remaining variables
            for (int j = i; j < gv->numVariables - 1; j++) {
                gv->variables[j] = gv->variables[j + 1];
            }
            gv->numVariables--;

            // Reallocate memory
            gv->variables = realloc(
                    gv->variables, gv->numVariables * sizeof(Variable));
            return;
        }
    }
}

/* validate_range()
 *
 * Checks if arguments for the command is valid
 * Validates the name, start, increment, and end values
 *
 * if validation fail, exit with invalid_variable error
 */
void validate_range(const char* name, const char* tempStart,
        const char* tempIncrement, const char* tempEnd)
{
    if (!name || !tempStart || !tempIncrement || !tempEnd
            || !is_valid_value(tempStart) || !is_valid_value(tempIncrement)
            || !is_valid_value(tempEnd) || !is_valid_variable_name(name)) {
        invalid_variables();
    }
}

/* handle_range_command()
 *
 * process the @range command to create or update a loop variables
 * validates argument with validate_range()
 * converts the string into numeric values
 * creates or updates the specified loop variable using
 * update_or_create_loop
 * checks if non-loop variable with same name exist
 * if yes remove it with remove_variable()
 * output newly create loop variable
 */
void handle_range_command(GlobalVar* gv, char* rangeArg)
{
    // Seperate the arguments into name, tempStart, tempIncrement, and tempEnd
    char* name = strtok(rangeArg, ",");
    char* tempStart = strtok(NULL, ",");
    char* tempIncrement = strtok(NULL, ",");
    char* tempEnd = strtok(NULL, ",");

    // Validation
    validate_range(name, tempStart, tempIncrement, tempEnd);

    // Convert string into numeric values
    double start = atof(tempStart);
    double increment = atof(tempIncrement);
    double end = atof(tempEnd);

    // Check if increment is valid with given start and end value
    if ((start < end && increment <= 0) || (start > end && increment >= 0)) {
        fprintf(stderr,
                "Error in command, expression or assignment operation "
                "detected\n");
        return;
    }

    // Check if variable already exist
    int varIndex = find_variable_index(gv, name);
    int loopVarIndex = find_loop_variable_index(gv, name);

    // If non loop variable exist, remove it
    if (varIndex != 1) {
        remove_variable(gv, name);
    }

    // if loop variable exist update, else create a new loop variable.
    update_or_create_loop(gv, name, start, increment, end, loopVarIndex);

    printf("%s = %.*g (%.*g, %.*g, %.*g)\n", name, gv->significantFigures,
            start, gv->significantFigures, start, gv->significantFigures,
            increment, gv->significantFigures, end);
}

/* update_or_create_loop()
 *
 * either updates an existing loop variable
 * or creates a new one
 */
void update_or_create_loop(GlobalVar* gv, const char* name, double start,
        double increment, double end, int loopVarIndex)
{
    if (loopVarIndex != -1) {
        // Update loop variable
        gv->loopVariables[loopVarIndex].start = start;
        gv->loopVariables[loopVarIndex].increment = increment;
        gv->loopVariables[loopVarIndex].end = end;
        gv->loopVariables[loopVarIndex].value = start;
    } else {
        // Create new loop variable
        gv->loopVariables = realloc(gv->loopVariables,
                (gv->numLoopVariables + 1) * sizeof(LoopVariable));
        strcpy(gv->loopVariables[gv->numLoopVariables].name, name);
        gv->loopVariables[gv->numLoopVariables].start = start;
        gv->loopVariables[gv->numLoopVariables].increment = increment;
        gv->loopVariables[gv->numLoopVariables].end = end;
        gv->loopVariables[gv->numLoopVariables].value = start;
        gv->numLoopVariables++;
    }
}

/* process_expression()
 *
 * Handles special commands like @print and @range first
 * if not, extracts variable name (if present)
 * checks if variable name exist, first with loop variables
 * followed by non-loop variables
 * if doesnt exist, add the variable with add_variable()
 * evaluates the expession using tinyexpr
 * free memory
 */
void process_expression(GlobalVar* gv, char* expression)
{
    // Check if @print command
    if (strcmp(expression, "@print") == 0) {
        print_variables(gv);
        print_loop_variables(gv);
        return;
    }

    // Check if @range command
    if (strncmp(expression, "@range ", RANGE_INPUT) == 0) {
        handle_range_command(gv, expression + RANGE_INPUT);
        return;
    }

    // Extract variable name from expression
    char* remain;
    char* varName = extract_variable_name(expression, &remain);
    int storeResult = 1;

    if (varName != NULL) {
        // Check if it's a loop variable
        int loopVarIndex = find_loop_variable_index(gv, varName);

        if (loopVarIndex != -1) {
            // loop variable with this name exist, evaluate but dont store.
            storeResult = 0;
        } else {
            // If non-loop variable, add if it doesnt exist
            int varIndex = find_variable_index(gv, varName);
            if (varIndex == -1) {
                add_variable(gv, varName);
            }
        }
    }

    // Evaluate expression
    evaluate(gv, remain, varName, storeResult);

    if (varName != NULL) {
        free(varName);
    }
}

/* extract_variable_name()
 *
 * parses and expression to seperate the variable name (if present)
 * trims whitespaces and handles assignment
 * returns varName
 *
 * REF: This code is referencing the output i got from Claude AI
 */
char* extract_variable_name(char* expression, char** remain)
{
    // Trim leading spaces
    while (*expression == ' ') {
        expression++;
    }

    // Find '='
    char* equalPosition = strchr(expression, '=');
    if (equalPosition == NULL) {
        *remain = expression;
        return NULL;
    }

    // Trim trailing spaces before '='
    char* end = equalPosition - 1;
    while (end > expression && *end == ' ') {
        *end = '\0';
        end--;
    }

    // Extract variable name
    int varNameLength = equalPosition - expression;
    char* varName = (char*)malloc(varNameLength + 1);
    strncpy(varName, expression, varNameLength);
    varName[varNameLength] = '\0';

    // Move past '=' and trim leading spaces in the expression.
    *remain = equalPosition + 1;
    while (**remain == ' ') {
        (*remain)++;
    }

    return varName;
}

/* find_variable_index()
 *
 * Find the index of a variable in the variable array
 * returns index if found, -1 if not found
 */
int find_variable_index(GlobalVar* gv, const char* varName)
{
    for (int i = 0; i < gv->numVariables; i++) {
        if (strcmp(gv->variables[i].name, varName) == 0) {
            return i;
        }
    }
    return -1;
}

/* find_loop_variable_index()
 *
 * Find the index of a loop variable in the loop variable array
 * returns index if found, -1 if not found
 */
int find_loop_variable_index(GlobalVar* gv, const char* varName)
{
    for (int i = 0; i < gv->numLoopVariables; i++) {
        if (strcmp(gv->loopVariables[i].name, varName) == 0) {
            return i;
        }
    }
    return -1;
}

/* add_variable()
 *
 * Checks for duplicates
 * creates a new variable with the given name
 * Initialise its value to 0
 */
void add_variable(GlobalVar* gv, const char* varName)
{
    // Checks for duplicates
    if (is_duplicate_variable(gv, varName)) {
        duplicate_variables();
        return;
    }

    gv->variables
            = realloc(gv->variables, (gv->numVariables + 1) * sizeof(Variable));

    // Store variable
    strcpy(gv->variables[gv->numVariables].name, varName);
    gv->variables[gv->numVariables].value = 0;
    gv->numVariables++;
}

/* add_te_variables()
 *
 * Adds both non-loop and loop variables to tinyexpr variable array
 * Populate names and values for expression evaluation
 */
void add_te_variables(GlobalVar* gv, te_variable* teVars, double* teValues,
        char teNames[][MAX_VAR_NAME_LENGTH])
{
    int varIndex = 0;

    // Adds regular variables
    for (int i = 0; i < gv->numVariables; i++) {
        strcpy(teNames[varIndex], gv->variables[i].name);
        teValues[varIndex] = gv->variables[i].value;
        teVars[varIndex].name = teNames[varIndex];
        teVars[varIndex].address = &teValues[varIndex];
        teVars[varIndex].type = TE_VARIABLE;
        varIndex++;
    }

    // Add loop variables
    for (int i = 0; i < gv->numLoopVariables; i++) {
        strcpy(teNames[varIndex], gv->loopVariables[i].name);
        teValues[varIndex] = gv->loopVariables[i].value;
        teVars[varIndex].name = teNames[varIndex];
        teVars[varIndex].address = &teValues[varIndex];
        teVars[varIndex].type = TE_VARIABLE;
        varIndex++;
    }
}

/* evaluate()
 *
 * complies and evaluates the given expression
 * handles variabl assignment and result storing
 * prints result with specified significant figures
 *
 * REF: This function is referencing the output i had from claude AI
 * REF: This function references the code given in the spec
 */
void evaluate(GlobalVar* gv, char* expression, char* varName, int storeResult)
{
    // Remove leading spaces
    while (*expression == ' ') {
        expression++;
    }

    int errorPosition;
    int numTeVars = gv->numVariables + gv->numLoopVariables;
    te_variable teVars[numTeVars];
    double teValues[numTeVars];
    char teNames[numTeVars][MAX_VAR_NAME_LENGTH];

    // Add non-loop and loop variables to te varible
    add_te_variables(gv, teVars, teValues, teNames);

    // Complie and evaluate the expression
    te_expr* expr = te_compile(expression, teVars, numTeVars, &errorPosition);

    if (expr) {
        // Successfully compiled
        double result = te_eval(expr);

        if (varName != NULL) {
            // Check if its a loop variable
            int loopVarIndex = -1;
            for (int i = 0; i < gv->numLoopVariables; i++) {
                if (strcmp(gv->loopVariables[i].name, varName) == 0) {
                    loopVarIndex = i;
                    break;
                }
            }
            // Updates loop variable value, else regular variable value
            if (loopVarIndex != -1) {
                gv->loopVariables[loopVarIndex].value = result;
            } else if (storeResult) {
                int varIndex = find_variable_index(gv, varName);
                if (varIndex != -1) {
                    gv->variables[varIndex].value = result;
                }
            }
            printf("%s = %.*g\n", varName, gv->significantFigures, result);
        } else {
            printf("Result = %.*g\n", gv->significantFigures, result);
        }
        te_free(expr);
    } else {
        fprintf(stderr,
                "Error in command, expression or assignment operation "
                "detected\n");
    }
}

/* handle_file()
 *
 * reads and process expression line by line from the file using fgets
 * while it is not the end of file
 * closes file reading when reached end of file
 */
void handle_file(GlobalVar* gv)
{
    // Open file
    FILE* file = fopen(gv->file, "r");
    char line[MAX_BUFFER];

    // Read and process each line while not NULL
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove '\n' character
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Skip empty or comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // Process the expression line by line
        process_expression(gv, line);
    }

    // Check for file reading errors
    if (ferror(file)) {
        fprintf(stderr, "uqexpr: error reading file\n");
        fclose(file);
        free_memory(gv);
        exit(1);
    }

    // Close file
    fclose(file);
}

/* main()
 *
 * initialise global variables
 * parses command-line arguments into parse_arguments()
 * handles input from either user or a file
 * frees memory upon exit
 */
int main(int argc, char* argv[])
{
    // Allocate memory for global variables
    GlobalVar* gv = (GlobalVar*)malloc(sizeof(GlobalVar));
    if (gv == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    // Initilise structure
    gv->variables = NULL;
    gv->loopVariables = NULL;
    gv->numVariables = 0;
    gv->numLoopVariables = 0;
    gv->significantFigures = DEFAULT_SIG_FIGURES;
    gv->file = NULL;

    // Parse command line arguments
    parse_arguments(argc, argv, gv);
    // print data
    startup_message();
    print_variables(gv);
    print_loop_variables(gv);

    // Process input from user if file is NULL, else read from file
    if (gv->file == NULL) {
        handle_user_input(gv);
    } else {
        handle_file(gv);
    }
    // Print exit message and free memory
    print_thank_you();
    free_memory(gv);
    return 0;
}
