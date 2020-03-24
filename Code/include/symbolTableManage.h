#ifndef __SYMBOL_TABLE_MANAGE_H__
#define __SYMBOL_TABLE_MANAGE_H__
#include"stdbool.h"
#include"common.h"

#define TABLE_SIZE 256
#define STRUCT_TABLE_SIZE 32
//typedef struct Type_ *Type;
//typedef struct FieldList_ *FieldList;
typedef struct Vartype Vartype;
typedef struct FieldList FieldList;
typedef struct Varkind Varkind;
typedef struct ArrayList ArrayList;//在描述数组时使用，
typedef struct StructNode StructNode; //建立 Struct 符号表使用
typedef enum OBJKIND //符号表中一个节点的类型
{
    //NOTYPE,
    ANYKIND,  //和Vartyep 加入ANYVAR原因是一样的
    FUNCTION,
    VARIBLE,
    ARRAY,
    CONSTANT,
    EMPTY
} OBJKIND;
typedef enum BIND   //用于变量的可见范围，全局变量还是局部变量
{
    GLOBAL,
    LOCAL
}BIND;

struct Vartype{//这是 一个 描述Specifier的结构，目的是表达所有类型，包括int ,float,struct及其中的域，
    VARSpecifier specifier;
    int memory_size; //这种类型的变量所需空间，如Type a;那么变量a将占用memory_size byte空间，int float是4,

    //结构体类型信息是一个链表,当kond是STRUCTVAR时使用，
    char struct_id[32];
    FieldList *structure;  //用于描述struct 中的域
};
struct Varkind
{
    OBJKIND objkind; //这里只能是varible 或者 array;
    //这个union 只用于描述变量类型是数组 或 函数，
    union{
        //数组类型时使用
        struct{
            ArrayList* array; //这个链表，维数越低越靠近头部，如 int a[3][2][4];那么第一个节点的array_size 是4;
            int dimension;
            int objnum;
        } array;
        //函数类型时使用
        struct{
            FieldList *param;
            int param_num;
            bool isdefined;
        } function;
    };
};
struct StructNode{
    int num;
    char struct_id[32];
    FieldList *structure;
    int memory_size;
};
struct ArrayList
{
    int array_size;
    ArrayList *tail;
};

struct FieldList{
    char name[32];
    Vartype type;
    Varkind kind;
    FieldList *tail;
};
char symbol_str[2048]; //用于存储 所有变量的名称，集中存储来节约内存，
typedef struct VarAttribute{
    Vartype type;
    Varkind kind;
} VarAttribute;
typedef struct SymbolNode
{
    int num, value,size;
    Vartype type;
    Varkind kind;
    BIND bind;
    char name[32];
    int action_scope_deep;  //用于选做的作用域 深度

    //lab3:
    bool varible_is_arg;

    struct SymbolNode *conflict;
    struct SymbolNode *childscope;
    struct SymbolNode *next;
} SymbolNode;
int initSymbolNode();
unsigned int hash_pjw(char *name);
SymbolNode *allocSymNode();
SymbolNode *addSynNode(char *name);
SymbolNode* addVarible(char *name, Vartype *type);
SymbolNode* addLocalVarible(char *name, Vartype *type, int deep);
int addFunction(char *name, Vartype*type,Varkind *kind);
SymbolNode* addArray(char *name, Vartype *type,Varkind* kind,int deep);
//SymbolNode *findObject(char *name);
bool isPresence(char *name);
bool isFunDecPresence(char *name);
SymbolNode *findSymbol(char *name);
Vartype *findSymbolType(char *name);
Varkind *findSymbolKind(char *name);
bool isTypeEqual(Vartype *type1, Vartype *type2);
bool isNodeTypeEqual(SymbolNode *node1, SymbolNode *node2);
//bool isParamArguEqual(FieldList *param, FieldList *argu);
bool isFieldListEqual(FieldList *s1, FieldList *s2);
bool isTypeKindEqual(Vartype *type1, Varkind *kind1, Vartype *type2, Varkind *kind2);
int addStructNode(Vartype *struct_type);
int findStructNode(char *name, Vartype *type);
void printStructTab();
void printSymTab();

bool isVaribleArg(char* varname);
#endif