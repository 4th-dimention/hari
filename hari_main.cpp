////////////////////////////////
// NOTE(allen): Hari - An equation representation experiment

////////////////////////////////
// NOTE(allen): Base

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

typedef  uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef   int8_t S8;
typedef  int16_t S16;
typedef  int32_t S32;
typedef  int64_t S64;
typedef   int8_t B8;
typedef  int16_t B16;
typedef  int32_t B32;
typedef  int64_t B64;
typedef    float F32;
typedef   double F64;


////////////////////////////////
// NOTE(allen): Hari Types

enum HARI_ExpressionKind{
    HARI_ExpressionKind_Null,
    HARI_ExpressionKind_Number,
    HARI_ExpressionKind_Variable,
    HARI_ExpressionKind_Plus,
    HARI_ExpressionKind_Minus,
};

struct HARI_Expression{
    HARI_ExpressionKind kind;
    HARI_Expression *parent;
    HARI_Expression *left;
    HARI_Expression *right;
    F64 x;
    union{
        char var[8];
        U64 id;
    };
};

struct HARI_Equation{
    HARI_Expression *left;
    HARI_Expression *right;
};

struct HARI_Solve{
    HARI_Expression *unknown_ptr;
    
    U64 hit_counter;
    B32 error;
};

struct HARI_VariableBindings{
    HARI_VariableBindings *next;
    union{
        char var[8];
        U64 id;
    };
    F64 val;
};

struct HARI_StringizeCtx{
    char *ptr;
    char *opl;
};

////////////////////////////////
// NOTE(allen): Constructing & Solving

void
_hari_variable_id_fill(char *dst, char *src){
    U64 len = (U64)strlen(src);
    if (len > 8){
        len = 8;
    }
    memcpy(dst, src, len);
    if (len < 8){
        memset(dst + len, 0, 8 - len);
    }
}

void
hari_push_variable_binding(HARI_VariableBindings **bindings, char *var, F64 val){
    HARI_VariableBindings *new_bind = (HARI_VariableBindings*)malloc(sizeof(HARI_VariableBindings));
    new_bind->next = *bindings;
    *bindings = new_bind;
    _hari_variable_id_fill(new_bind->var, var);
    new_bind->val = val;
}

F64
hari_resolve_variable_binding(HARI_VariableBindings *bindings, U64 id){
    F64 result = 0.f;
    for (; bindings != 0;
         bindings = bindings->next){
        if (bindings->id == id){
            result = bindings->val;
            break;
        }
    }
    return(result);
}

HARI_Expression*
_hari_expr_zero(HARI_ExpressionKind kind, HARI_Expression *parent){
    HARI_Expression *expression = (HARI_Expression*)malloc(sizeof(HARI_Expression));
    memset(expression, 0, sizeof(*expression));
    expression->kind = kind;
    expression->parent = parent;
    return(expression);
}

HARI_Expression*
hari_number(HARI_Expression *parent, F64 x){
    HARI_Expression *expression = _hari_expr_zero(HARI_ExpressionKind_Number, parent);
    expression->x = x;
    return(expression);
}

HARI_Expression*
hari_variable(HARI_Expression *parent, char *var){
    HARI_Expression *expression = _hari_expr_zero(HARI_ExpressionKind_Variable, parent);
    _hari_variable_id_fill(expression->var, var);
    return(expression);
}

HARI_Expression*
hari_plus(HARI_Expression *parent){
    HARI_Expression *expression = _hari_expr_zero(HARI_ExpressionKind_Plus, parent);
    return(expression);
}

HARI_Expression*
hari_minus(HARI_Expression *parent){
    HARI_Expression *expression = _hari_expr_zero(HARI_ExpressionKind_Minus, parent);
    return(expression);
}

HARI_Equation*
hari_equate(HARI_Expression *left, HARI_Expression *right){
    HARI_Equation *equation = (HARI_Equation*)malloc(sizeof(HARI_Equation));
    equation->left = left;
    equation->right = right;
    return(equation);
}

F64
hari_evaluate(HARI_Expression *expression, HARI_VariableBindings *binds){
    F64 result = 0.f;
    switch (expression->kind){
        case HARI_ExpressionKind_Number:
        {
            result = expression->x;
        }break;
        case HARI_ExpressionKind_Variable:
        {
            result = hari_resolve_variable_binding(binds, expression->id);
        }break;
        case HARI_ExpressionKind_Plus:
        {
            F64 l = hari_evaluate(expression->left, binds);
            F64 r = hari_evaluate(expression->right, binds);
            result = l + r;
        }break;
        case HARI_ExpressionKind_Minus:
        {
            F64 l = hari_evaluate(expression->left, binds);
            F64 r = hari_evaluate(expression->right, binds);
            result = l - r;
        }break;
    }
    return(result);
}

void
_hari_solve_find_unknown(HARI_Solve *solve, HARI_Expression *expression, U64 id){
    switch (expression->kind){
        case HARI_ExpressionKind_Number:break;
        
        case HARI_ExpressionKind_Variable:
        {
            if (expression->id == id){
                solve->unknown_ptr = expression;
                solve->hit_counter += 1;
            }
        }break;
        
        case HARI_ExpressionKind_Plus:
        case HARI_ExpressionKind_Minus:
        {
            _hari_solve_find_unknown(solve, expression->left, id);
            _hari_solve_find_unknown(solve, expression->right, id);
        }break;
    }
}

void
_hari_solve_find_unknown(HARI_Solve *solve, HARI_Equation *equation, U64 id){
    _hari_solve_find_unknown(solve, equation->left, id);
    _hari_solve_find_unknown(solve, equation->right, id);
}

HARI_Expression*
hari_solve_for(HARI_Equation *equation, char *var){
    union { char var[8]; U64 id; } v;
    _hari_variable_id_fill(v.var, var);
    
    HARI_Expression *result = 0;
    
    HARI_Solve solve = {};
    
    // NOTE(allen): Step 1: analyze
    _hari_solve_find_unknown(&solve, equation, v.id);
    
    // NOTE(allen): Step 2: select solve technique
    if (solve.hit_counter == 1){
        // NOTE(allen): solve by elementary algebra
        HARI_Equation eq = *equation;
        
        for (;solve.unknown_ptr->parent != 0;){
            HARI_Expression *child_with_unknown = solve.unknown_ptr;
            HARI_Expression *root = solve.unknown_ptr->parent;
            for (;root->parent != 0; root = root->parent){
                child_with_unknown = root;
            }
            
            if (root == eq.right){
                HARI_Expression *t = eq.left;
                eq.left = eq.right;
                eq.right = t;
            }
            
            switch (root->kind){
                default:
                {
                    solve.error = true;
                    goto quit;
                }break;
                
                case HARI_ExpressionKind_Plus:
                {
                    // [a + b = c] -> [a = c - b]
                    HARI_Expression *a = 0;
                    HARI_Expression *b = 0;
                    if (child_with_unknown == root->left){
                        a = root->left;
                        b = root->right;
                    }
                    else{
                        a = root->right;
                        b = root->left;
                    }
                    HARI_Expression *c = eq.right;
                    
                    HARI_Expression *new_right = hari_minus(0);
                    new_right->left = c;
                    new_right->right = b;
                    c->parent = new_right;
                    b->parent = new_right;
                    
                    HARI_Expression *new_left = a;
                    a->parent = 0;
                    
                    eq.left = new_left;
                    eq.right = new_right;
                }break;
                
                case HARI_ExpressionKind_Minus:
                {
                    // [a - b = c] -> [a = c + b]
                    // [b - a = c] -> [a = b - c]
                    
                    HARI_Expression *a = 0;
                    HARI_Expression *b = 0;
                    B32 invert = false;
                    if (child_with_unknown == root->left){
                        a = root->left;
                        b = root->right;
                    }
                    else{
                        a = root->right;
                        b = root->left;
                        invert = true;
                    }
                    HARI_Expression *c = eq.right;
                    
                    HARI_Expression *new_right = 0;
                    if (invert){
                        new_right = hari_minus(0);
                        new_right->left = b;
                        new_right->right = c;
                    }
                    else{
                        new_right = hari_plus(0);
                        new_right->left = c;
                        new_right->right = b;
                    }
                    b->parent = new_right;
                    c->parent = new_right;
                    
                    HARI_Expression *new_left = a;
                    a->parent = 0;
                    
                    eq.left = new_left;
                    eq.right = new_right;
                }break;
            }
        }
        
        result = eq.right;
    }
    
    quit:;
    
    return(result);
}


////////////////////////////////
// NOTE(allen): Stringizing

void
_hari_stringize_printfv(HARI_StringizeCtx *ctx, char *fmt, va_list args){
    ctx->ptr += vsnprintf(ctx->ptr, ctx->ptr - ctx->opl, fmt, args);
}

void
_hari_stringize_printf(HARI_StringizeCtx *ctx, char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    _hari_stringize_printfv(ctx, fmt, args);
    va_end(args);
}

void
_hari_stringize(HARI_StringizeCtx *ctx, HARI_Expression *expression){
    switch (expression->kind){
        case HARI_ExpressionKind_Number:
        {
            _hari_stringize_printf(ctx, "%f", expression->x);
        }break;
        case HARI_ExpressionKind_Variable:
        {
            _hari_stringize_printf(ctx, "%.*s", sizeof(expression->var), expression->var);
        }break;
        case HARI_ExpressionKind_Plus:
        {
            _hari_stringize(ctx, expression->left);
            _hari_stringize_printf(ctx, " + ");
            _hari_stringize(ctx, expression->right);
        }break;
        case HARI_ExpressionKind_Minus:
        {
            _hari_stringize(ctx, expression->left);
            _hari_stringize_printf(ctx, " - ");
            _hari_stringize(ctx, expression->right);
        }break;
    }
}

void
_hari_stringize(HARI_StringizeCtx *ctx, HARI_Equation *equation){
    _hari_stringize(ctx, equation->left);
    _hari_stringize_printf(ctx, " = ");
    _hari_stringize(ctx, equation->right);
}

static U64 buffer_max = 1 << 20;
static char *buffer = 0;

HARI_StringizeCtx
_hari_touch_buffer(void){
    if (buffer == 0){
        buffer = (char*)malloc(buffer_max);
    }
    HARI_StringizeCtx ctx = {buffer, buffer + buffer_max};
    return(ctx);
}

char*
_hari_copy_buffer(HARI_StringizeCtx *ctx){
    U64 size = (U64)(ctx->ptr - buffer);
    char *string = (char*)malloc(size + 1);
    memcpy(string, buffer, size);
    string[size] = 0;
    return(string);
}

char*
hari_stringize(HARI_Expression *expression){
    HARI_StringizeCtx ctx = _hari_touch_buffer();
    _hari_stringize(&ctx, expression);
    char *string = _hari_copy_buffer(&ctx);
    return(string);
}

char*
hari_stringize(HARI_Equation *equation){
    HARI_StringizeCtx ctx = _hari_touch_buffer();
    _hari_stringize(&ctx, equation);
    char *string = _hari_copy_buffer(&ctx);
    return(string);
}


////////////////////////////////
// NOTE(allen): Entry Point

int
main(int argc, char **argv){
    {
        HARI_Expression *plus = hari_plus(0);
        plus->left = hari_number(plus, 1);
        plus->right = hari_number(plus, 2);
        
        char *expr_string = hari_stringize(plus);
        F64 n = hari_evaluate(plus, 0);
        printf("Hari Says: %s => %f\n", expr_string, n);
    }
    
    {
        HARI_Expression *plus = hari_plus(0);
        plus->left = hari_variable(plus, "x");
        plus->right = hari_number(plus, 2);
        
        HARI_VariableBindings *binds = 0;
        hari_push_variable_binding(&binds, "x", 1);
        
        char *expr_string = hari_stringize(plus);
        F64 n = hari_evaluate(plus, binds);
        printf("Hari Says: %s => %f\n", expr_string, n);
    }
    
    {
        HARI_Expression *plus = hari_plus(0);
        plus->left = hari_variable(plus, "x");
        plus->right = hari_number(plus, 2);
        
        HARI_Expression *right = hari_number(0, 3);
        
        HARI_Equation *equation = hari_equate(plus, right);
        HARI_Expression *expression = hari_solve_for(equation, "x");
        
        char *equ_string = hari_stringize(equation);
        char *expr_string = hari_stringize(expression);
        F64 n = hari_evaluate(expression, 0);
        printf("Hari Says: solve x [%s] => %s => %f\n", equ_string, expr_string, n);
    }
    
    {
        HARI_Expression *left = hari_number(0, 2);
        
        HARI_Expression *minus = hari_minus(0);
        minus->left = hari_number(minus, 3);
        minus->right = hari_variable(minus, "x");
        
        HARI_Equation *equation = hari_equate(left, minus);
        HARI_Expression *expression = hari_solve_for(equation, "x");
        
        char *equ_string = hari_stringize(equation);
        char *expr_string = hari_stringize(expression);
        F64 n = hari_evaluate(expression, 0);
        printf("Hari Says: solve x [%s] => %s => %f\n", equ_string, expr_string, n);
    }
    
    return(0);
}

