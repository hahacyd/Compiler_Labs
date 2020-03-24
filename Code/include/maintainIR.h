#ifndef __MAINTAINIR_H__
#define __MAINTAINIR_H__
#include<stdio.h>
#include "buildtree.h"
#include "symbolTableManage.h"
typedef struct Operand Operand;
typedef struct InterCode InterCode;
typedef struct InterCodes InterCodes;
typedef struct Label Label;
struct Operand {
    enum { 
        //OTEMPVAR,  //临时变量
        OVAR,     //变量
        OCONST,   //常数
        OADDRESS,  //地址变量
        OINFERENCE    //引用变量
    } kind;
    union {
        int value;   //const varible
        char varname[32];  //id varible
    } u;
};
struct Label{
    int labelno;
    char labelname[32];
};
typedef enum OperandOperator {
    NONE,
    ACCEPT
} OperandOperator;
typedef enum InterCodeKind {
    WRITE,
    READ,
    IRFUNCTION,
    IRPARAM,
    ARG,
    LABEL,
    GOTO,
    CALL,
    DEC
} InterCodeKind;
struct InterCode {
    InterCodeKind kind;
    union {
        struct {
            union{
             Operand op;
             Label label;
            };
        } single;   //为write , read , return 使用的
        struct {
            char id[32];
        } function;
        struct {
            char id[32];
        } param;
        struct {
            Operand right;
        } address;
        struct {
            Operand right;
        } reference;
        struct {
            Operand right, left;
        } assign;
        struct {
            Operand result, op1, op2;
        } binop;
        struct{
            Operand cond1, cond2;
            Label label;
            OperandOperator operate;
        } ifstmt;
        struct{
            Operand op1;
            char function_name[32];
        } call;
        struct{
            Operand op;
            int size;
        } dec;
    } u;
};
struct InterCodes{
    struct InterCode intercode;
    struct InterCodes *prev, *next;
};
void printIr2File(char* filename);
void printIR(FILE* object, InterCode* intercode);
InterCodes* allocateInterCodes();
void addInterCodes(InterCodes* node);
void addIrFunction(char* funct_name);
void addIrParam(char* param_name);
void addIrWrite(Operand* op);
void addIrRead(Operand* op);
void addLocalInterCode(InterCodes* code1, InterCodes* code2);
void addThreeInter(InterCodeKind kind, Operand* result, Operand* op1, Operand* op2);
void addTwoInter(InterCodeKind kind, Operand* left, Operand* right);
void addOneInter(InterCodeKind kind, Operand* op);
void addIrIf(InterCodeKind kind, Operand* cond1, Operand* cond2, OperandOperator operate, Label* label);
void addIrLable(Label* label);
void addIrGoto(Label* label);
void addIrCall(Operand* op, char* funct_name);
void addIrArg(Operand* op);
void addIrDEC(Operand* op, int size);
bool isOperandEqual(Operand* op1, Operand* op2);
Operand* newTemp();
Label* newLabel();
void printVarOrConst(FILE* object, Operand* op);
bool isOperandInIr(InterCode* ir, Operand* op);
bool isOperandUsedLater(InterCodes* s, Operand* left);
int dropDeadIr();
InterCodes* deleteIr(InterCodes* s);
int optimizeIR();
int dropBroadCopy();
InterCodes* findBroadCopy(InterCodes* s, Operand* op);
#endif