#include "maintainIR.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct InterCodes *intercode_begin = NULL,
                         *intercode_end = NULL,
                         *intercode_cur = NULL;
/**
 * 这是配合COMPARISON 这个enum使用的，它被定义在common.h中，
 * 在需要这些判断符号的字符串时，直接relop_str[EQUAL]即可获得，而不用先判断COMPARISON，再打印，提高微小的一点性能
 * 这依赖于COMPARISON 初始化时第一条一定是0，且其中的内容不能改变顺序;
 */
static char* relop_str[] = {
    "==",
    "!=",
    ">",
    "<",
    ">=",
    "<="
};
InterCodes* allocateInterCodes()
{
    InterCodes* s = malloc(sizeof(InterCodes));
    s->next = NULL;
    s->prev = NULL;
    return s;
}
void addInterCodes(InterCodes* node)
{
    assert(node != NULL);
    if (intercode_begin == NULL) {
        intercode_begin = node;
        intercode_cur = node;
        intercode_end = node;
    } else {
        intercode_cur->next = node;
        node->prev = intercode_cur;

        intercode_cur = node;
        intercode_end = node;
    }
}
void addIrFunction(char* funct_name)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = IRFUNCTION;
    strcpy(s->intercode.u.function.id, funct_name);
    addInterCodes(s);
}
void addIrParam(char* param_name)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = IRPARAM;
    strcpy(s->intercode.u.param.id, param_name);
    addInterCodes(s);
}
void addThreeInter(InterCodeKind kind, Operand* result, Operand* op1, Operand* op2)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = kind;
    s->intercode.u.binop.result = *result;
    s->intercode.u.binop.op1 = *op1;
    s->intercode.u.binop.op2 = *op2;
    addInterCodes(s);
}
void addTwoInter(InterCodeKind kind, Operand* left, Operand* right)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = kind;
    s->intercode.u.assign.left = *left;
    s->intercode.u.assign.right = *right;
    addInterCodes(s);
}
void addOneInter(InterCodeKind kind, Operand* op)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = RETURN;
    s->intercode.u.single.op = *op;
    addInterCodes(s);
}
void addIrIf(InterCodeKind kind, Operand* cond1, Operand* cond2, OperandOperator operate, Label* label)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = IF;
    s->intercode.u.ifstmt.cond1 = *cond1;
    s->intercode.u.ifstmt.cond2 = *cond2;
    s->intercode.u.ifstmt.label = *label;
    s->intercode.u.ifstmt.operate = operate;
    //LOG("%d", operate);
    addInterCodes(s);
}
void addIrLable(Label* label)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = LABEL;
    s->intercode.u.single.label = *label;

    addInterCodes(s);
}
void addIrGoto(Label* label)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = GOTO;
    s->intercode.u.single.label = *label;

    addInterCodes(s);
}
void addIrCall(Operand* op, char* funct_name)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = CALL;
    s->intercode.u.call.op1 = *op;
    strcpy(s->intercode.u.call.function_name, funct_name);
    addInterCodes(s);
}
void addIrArg(Operand* op)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = ARG;
    s->intercode.u.single.op = *op;
    addInterCodes(s);
}
void addIrWrite(Operand* op)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = WRITE;
    s->intercode.u.single.op = *op;
    addInterCodes(s);
}
void addIrRead(Operand* op)
{
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = READ;
    s->intercode.u.single.op = *op;
    addInterCodes(s);
}
bool isOperandEqual(Operand* op1, Operand* op2)
{
    bool result = false;
    if (op1->kind != op2->kind) {
        return false;
    }
    switch (op1->kind) {
    case OCONST:
        result = op1->u.value == op2->u.value;
        break;
    case OADDRESS:
    case OINFERENCE:
    case OVAR:
        result = !strcmp(op1->u.varname, op2->u.varname);
        break;
    }
    return result;
}
static int temp_varible_count = 0;
void addIrDEC(Operand* op, int size)
{
    //addOneInter(DEC, op);
    InterCodes* s = allocateInterCodes();
    s->intercode.kind = DEC;
    s->intercode.u.dec.op = *op;
    s->intercode.u.dec.size = size;
    addInterCodes(s);
}
Operand* newTemp()
{
    static char temp_str[32];
    Operand* t = malloc(sizeof(Operand));
    t->kind = OVAR;
    do {
        sprintf(temp_str, "t%d", temp_varible_count);
        temp_varible_count += 1;
    } while (isPresence(temp_str)); // 临时变量不能和源代码中的语法名称冲突
    strcpy(t->u.varname, temp_str);
    return t;
}
static int labelno = 0;

Label* newLabel()
{
    Label* t = malloc(sizeof(Label));
    sprintf(t->labelname, "label%d", labelno);
    t->labelno = labelno;

    labelno += 1;

    return t;
}
void printVarOrConst(FILE* object, Operand* op)
{
    char temp[32];
    // LOG("%d", op->suffix);
    switch (op->kind) {
    case OADDRESS:
        fprintf(object, "&%s", op->u.varname);
        break;
    case OINFERENCE:
        fprintf(object, "*%s", op->u.varname);
        break;
    case OCONST:
        sprintf(temp, "%d", op->u.value);
        fprintf(object, "#%s", temp);
        break;
    //case OTEMPVAR:
    case OVAR:
        fprintf(object, "%s", op->u.varname);
        break;
    }
}
void printIR(FILE* object, InterCode* intercode)
{
    assert(intercode != NULL);
    char temp[32];
    char op = '\0';
    //LOG("%d", intercode->kind);
    switch (intercode->kind) {
    case IRFUNCTION:
        fprintf(object, "FUNCTION %s :", intercode->u.function.id);
        break;
    case IRPARAM:
        fprintf(object, "PARAM %s", intercode->u.param.id);
        break;
    case PLUS: //op 这样写 要比 单独 打印每个op 优雅
        op = (op == '\0') ? '+' : op;
    case STAR:
        op = (op == '\0') ? '*' : op;
    case MINUS:
        op = (op == '\0') ? '-' : op;
    case DIV:
        op = (op == '\0') ? '/' : op;
        printVarOrConst(object, &intercode->u.binop.result);
        fprintf(object, " := ");
        printVarOrConst(object, &intercode->u.binop.op1);
        fprintf(object, " %c ", op);
        printVarOrConst(object, &intercode->u.binop.op2);
        break;
    case ASSIGNOP:
        printVarOrConst(object, &intercode->u.assign.left);
        fprintf(object, " := ");
        printVarOrConst(object, &intercode->u.assign.right);
        break;
    case RETURN:
        fprintf(object, "RETURN ");
        printVarOrConst(object, &intercode->u.single.op);
        break;
    case IF:
        fprintf(object, "IF ");
        printVarOrConst(object, &intercode->u.ifstmt.cond1);
        fprintf(object, " %s ", relop_str[intercode->u.ifstmt.operate]);
        printVarOrConst(object, &intercode->u.ifstmt.cond2);
        fprintf(object, " GOTO %s", intercode->u.ifstmt.label.labelname);
        break;
    case LABEL:
        fprintf(object, "LABEL %s :", intercode->u.single.label.labelname);
        break;
    case GOTO:
        fprintf(object, "GOTO %s", intercode->u.single.label.labelname);
        break;
    case CALL:
        printVarOrConst(object, &intercode->u.call.op1);
        fprintf(object, " := CALL %s", intercode->u.call.function_name);
        break;
    case ARG:
        fprintf(object, "ARG ");
        printVarOrConst(object, &intercode->u.single.op);
        break;
    case WRITE:
        fprintf(object, "WRITE ");
        printVarOrConst(object, &intercode->u.single.op);
        break;
    case READ:
        fprintf(object, "READ ");
        printVarOrConst(object, &intercode->u.single.op);
        break;
    case DEC:
        fprintf(object, "DEC ");
        printVarOrConst(object, &intercode->u.dec.op);
        fprintf(object, " %d", intercode->u.dec.size);
        break;
    }
    fprintf(object, "\n");
}
void printIr2File(char* filename)
{
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        picnic("创建文件失败");
        return;
    }
    InterCodes* it = intercode_begin;
    for (; it != NULL; it = it->next) {
        printIR(file, &it->intercode);
        //printIR(stdout, &it->intercode);
    }
    //fprintf(file, "hello world!");
}
void addLocalInterCode(InterCodes* code1, InterCodes* code2)
{
    InterCodes* s = code1;
    while (s->next != NULL) {
        s = s->next;
    }
    s->next = code2;
    code2->prev = s;
}
InterCodes* deleteIr(InterCodes* s)
{
    //SHOW("删成功喽");
    InterCodes* result = NULL;
    if (s == intercode_begin) {
        intercode_begin = intercode_begin->next;
        intercode_begin->prev = NULL;
        result = intercode_begin;
        free(s);
    } else if (s == intercode_end) {
        intercode_end = intercode_end->prev;
        intercode_end->next = NULL;
        result = intercode_end;
        free(s);
    } else {
        s->prev->next = s->next;
        s->next->prev = s->prev;
        result = s->next;
        free(s);
    }
    return result;
}
bool isOperandInIr(InterCode* ir, Operand* op)
{
    bool result = false;
    switch (ir->kind) {
    case PLUS:
    case MINUS:
    case STAR:
    case DIV:
        result = isOperandEqual(op, &ir->u.binop.result)
            || isOperandEqual(op, &ir->u.binop.op1)
            || isOperandEqual(op, &ir->u.binop.op2);
        break;
    case ASSIGNOP:
        result = isOperandEqual(op, &ir->u.assign.left)
            || isOperandEqual(op, &ir->u.assign.right);
        //LOG("是不是相等呢?%d", result);
        break;
    case WRITE:
    case READ:
    case ARG:
    case RETURN:
        result = isOperandEqual(op, &ir->u.single.op);
        break;
    case IF:
        result = isOperandEqual(op, &ir->u.ifstmt.cond1)
            || isOperandEqual(op, &ir->u.ifstmt.cond2);
        break;
    case DEC:
        result = isOperandEqual(op, &ir->u.dec.op);
        break;
    }
    return result;
}
bool isOperandUsedLater(InterCodes* s, Operand* left)
{
    if (s == NULL) {
        return false;
    }
    InterCodes* it = s;
    for (; it != NULL; it = it->next) {
        if (isOperandInIr(&it->intercode, left)) {
            return true;
        }
    }
    return false;
}
InterCodes* findBroadCopy(InterCodes* s, Operand* op)
{
    InterCodes* it = s;
    for (; it != NULL; it = it->prev) {
        switch (it->intercode.kind) {
        case IRFUNCTION:
        case GOTO:
        case LABEL:
            return NULL;
        case RETURN:
            if(op->kind == OCONST){
                return NULL;
            }
        default:
            break;
        }
        if (it->intercode.kind == ASSIGNOP) {
            if(isOperandEqual(op,&it->intercode.u.assign.left) && op->kind != OCONST)
                return it;
        }
    }
    return NULL;
}
int dropBroadCopy()
{
    InterCodes* it = intercode_end;
    InterCodes* t = NULL;
    bool endflag = 0;
    while (it != NULL) {
        //SHOW("let");

        switch (it->intercode.kind) {
        case ASSIGNOP:
            t = findBroadCopy(it->prev, &it->intercode.u.assign.right);
            //LOG("%p", t);
            if(t != NULL)
            {
                SHOW("assign");
                printIR(stdout, &it->intercode);
                printIR(stdout, &t->intercode);
                it->intercode.u.assign.right = t->intercode.u.assign.right;
                deleteIr(t);
                endflag = 1;
            }
            break;
        case RETURN:

            t = findBroadCopy(it->prev, &it->intercode.u.single.op);
            if (t != NULL) {
                SHOW("return");
                printIR(stdout, &it->intercode);
                printIR(stdout, &it->prev->intercode);

                printIR(stdout, &t->intercode);
                
                it->intercode.u.single.op = t->intercode.u.assign.right;
                deleteIr(t);
                endflag = 1;
            }
            break;
        default:
            break;
        }
        if (endflag == 1) {
            endflag = 0;
        } else {
            it = it->prev;
        }
    }
}
int dropDeadIr()
{
    //deleteIr(intercode_begin);
    InterCodes* it = intercode_end;
    bool endflag = 0;
    while (it != NULL) {
        switch (it->intercode.kind) {
        case ASSIGNOP: {
            if (!isOperandUsedLater(it->next, &it->intercode.u.assign.left)) {
                //说明it所指的代码是死代码，可以删除了
                it = deleteIr(it);
                //SHOW("haha");
                endflag = 1;
            };
            break;
        case PLUS:
        case MINUS:
        case STAR:
        case DIV:
            if (!isOperandUsedLater(it->next, &it->intercode.u.binop.result)) {
                //说明it所指的代码是死代码，可以删除了
                it = deleteIr(it);
                continue;
            };
            break;

        }
        }
        if (endflag == 1) {
            endflag = 0;
            continue;
        } else {
            it = it->prev;
        }
    }
}
int optimizeIR()
{
    //for (int i = 0; i < 3;i++){
        dropDeadIr();
        //dropBroadCopy();
   // }

}