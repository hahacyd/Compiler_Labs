#include "symbolTableManage.h"
#include "debug.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SymbolNode *hashtable[TABLE_SIZE];
static StructNode struct_table[STRUCT_TABLE_SIZE];
static int struct_table_index = 0, struct_node_num = 0;
static SymbolNode *firstnode = NULL, *currentnode = NULL;
#define LINK_BEFORE        \
    if (firstnode == NULL) \
    {                      \
    }
int initSymbolNode()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        hashtable[i] = NULL;
    }
    return 1;
}
unsigned int hash_pjw(char *name)
{
    unsigned int val = 0, i;
    for (; *name; ++name)
    {
        val = (val << 2) + *name;
        if (i = val & ~0x3ff)
            val = (val ^ (i >> 12)) & 0x3ff;
    }
    return val;
}
static int nodecount = 0;
SymbolNode *allocateSymNode()
{
    SymbolNode *t = malloc(sizeof(SymbolNode));


    t->kind.objkind = EMPTY;
    t->conflict = NULL;
    t->childscope = NULL;
    t->next = NULL;

    //lab3:
    t->varible_is_arg = 0;
    t->type.memory_size = 0;
    return t;
}
SymbolNode *addSynNode(char *name)
{ //根据名字找到应在散列表中的位置，并且有初始化的作用
    if (isPresence(name))
    {
        if(isFunDecPresence(name)){  //函数声明
            return findSymbol(name);
        }
        picnic("name conflict!");
    }
    int index = hash_pjw(name) % TABLE_SIZE;

    SymbolNode *node = hashtable[index];
    SymbolNode *t = allocateSymNode(); //最新的变量在冲突链表的最前面，

    strcpy(t->name, name);
    //检查是否冲突:
    t->conflict = node;
    hashtable[index] = t;

    //node->conflict = NULL;
    t->num = nodecount;

    nodecount += 1;

    //用链表把已经添加的节点链接起来
    if (firstnode == NULL)
    {
        firstnode = currentnode = node;
    }
    else
    {
        currentnode->next = node;
        currentnode = node;
    }
    return t;
}
SymbolNode *findSymbol(char *name)
{
    if (name == NULL)
    {
        return NULL;
    }
    int index = hash_pjw(name) % TABLE_SIZE;
    SymbolNode *t = hashtable[index];
    for (; t != NULL; t = t->conflict)
    {
            if (0 == strcmp(name, t->name))
            {
                return t;
            }
    }
    
    return NULL;
}
Vartype *findSymbolType(char *name)
{
    //TODO:返回一个符号 name 的类型指针
    SymbolNode *ptr = findSymbol(name);
    if (ptr == NULL)
    {
        return NULL;
    }
    else
    {
        return &(ptr->type);
    }
}
Varkind *findSymbolKind(char* name){
    SymbolNode *ptr = findSymbol(name);
    if (ptr == NULL)
    {
        return NULL;
    }
    else
    {
        return &(findSymbol(name)->kind);
    }
}
bool isTypeKindEqual(Vartype* type1,Varkind* kind1,Vartype* type2,Varkind* kind2){
    bool flag = false;
    if(isTypeEqual(type1,type2)){
        if(kind1->objkind == ANYKIND || kind2->objkind == ANYKIND){
            return true;   //和向 Vartype 中加入 ANYVAR 的作用相同
        }
    }
    if(isTypeEqual(type1,type2) && kind1->objkind == kind2->objkind){
        switch (kind1->objkind)
        {
        case ARRAY: //当是数组时，只要基本类型 和 维数相同 即是 类型等价的
            flag = kind1->array.dimension == kind2->array.dimension;
            break;
        case VARIBLE:
            flag = true;
            break;
        case FUNCTION:
            picnic("please completment the function equal judgement");
            break;
        default:
            flag = false;
            break;
        }
    }
    return flag;
}
bool isTypeEqual(Vartype *type1, Vartype *type2)
{
    if(type1->specifier == ANYVAR || type2->specifier == ANYVAR){
        return true;
    }
    if (type1->specifier != STRUCTVAR && type2->specifier != STRUCTVAR)
    {
        return type1->specifier == type2->specifier;
    }
    else if(type1->specifier == STRUCTVAR && type2->specifier == STRUCTVAR)
    { //两边都是struct 类型
        //return strcmp(type1->struct_id, type2->struct_id) == 0;
        FieldList *s1 = type1->structure, *s2 = type2->structure;
        for (; s1 != NULL && s2 != NULL;s1 = s1->tail,s2 = s2->tail){
            if(!isTypeKindEqual(&s1->type,&s1->kind,&s2->type,&s2->kind)){
                return false;
            }
        }
        if(s1 != s2){
            return false;
        }
    }
    return true;
}
/*bool isParamArguEqual(FieldList* param,FieldList* argu){
    //TODO:判断实参和形参在数目 和 类型上是否相同
    for (; param != NULL && argu != NULL;param = param->tail,argu = argu->tail){
        if(!isFieldListEqual(param,argu)){  //有一个位置不匹配则退出并返回false
            return false;
        }
    }
    if(param != argu)  //应该是相等的且 都是 NULL
    { //如果参数数目不匹配，也返回false;
        return false;
    }
    return true;
}*/
/*bool isStructDomainEqual(FieldList* s1,FieldList* s2){

}*/
bool isFieldListEqual(FieldList* s1,FieldList* s2){
    Vartype *type1 = &s1->type, *type2 = &s2->type;
    Varkind *kind1 = &s1->kind, *kind2 = &s2->kind;
    if (kind1->objkind == kind2->objkind)
    {
        if (type1->specifier != STRUCTVAR && type2->specifier != STRUCTVAR) //两个变量的种类需相同
        {
            return type1->specifier == type2->specifier;
        }
        else
        { //变量的类型是 struct,此时判断是否是类型等价
            return strcmp(type1->struct_id, type2->struct_id) == 0;
        }
    }
    else
    {
        return false;
    }
    return false;
}
bool isNodeTypeEqual(SymbolNode *node1, SymbolNode *node2)
{
    Vartype *type1 = &node1->type, *type2 = &node2->type;
    Varkind *kind1 = &node1->kind, *kind2 = &node2->kind;
    if (kind1->objkind == kind2->objkind)
    {
        if (type1->specifier != STRUCTVAR && type2->specifier != STRUCTVAR) //两个变量的种类需相同
        {
            return type1->specifier == type2->specifier;
        }
        else
        { //变量的类型是 struct,此时判断是否是类型等价
            return strcmp(type1->struct_id, type2->struct_id) == 0;
        }
    }
    else
    {
        return false;
    }
    return false;
}
SymbolNode *addVarible(char *name, Vartype *type)
{
    SymbolNode *node = addSynNode(name);
    strcpy(node->name, name);
    node->kind.objkind = VARIBLE; //执行此函数 即意味着原来的此法单元是一个变量
    node->type = *type;
    //node->type.specifier = specifier;
    return node;
}
SymbolNode *addLocalVarible(char *name, Vartype *type, int deep)
{
    SymbolNode *t = addVarible(name, type);
    t->action_scope_deep = deep;
    t->bind = (deep == 0) ? GLOBAL : LOCAL;
    return t;
}
int addFunction(char *name, Vartype *type, Varkind *kind)
{
    // SymbolNode *s = findSymbol(name);
    // if (s != NULL && s->kind.objkind == FUNCTION && s->kind.function.isdefined == 0){
    //     s->kind
    // }
    SymbolNode *node = addSynNode(name);
    strcpy(node->name, name);
    node->type = *type;
    node->kind = *kind;
    node->kind.objkind = FUNCTION;

    return 1;
}
SymbolNode* addArray(char *name, Vartype *type, Varkind *kind, int deep)
{
    SymbolNode *node = addSynNode(name);
    node->type = *type;
    node->kind = *kind;
    node->kind.objkind = ARRAY;

    node->action_scope_deep = deep;
    node->bind = (deep == 0) ? GLOBAL : LOCAL;

    return node;
}
// SymbolNode *findObject(char *name)
// {
//     int index = hash_pjw(name) % TABLE_SIZE;
//     if (hashtable[index].kind.objkind == EMPTY)
//     {
//         return NULL;
//     }
//     return &hashtable[index];
// }
bool isFunDecPresence(char *name) //若已在散列表中出现，返回true;
{
    if (name == NULL)
    {
        return false;
    }
    int index = hash_pjw(name) % TABLE_SIZE;
    SymbolNode *t = hashtable[index];
    for (; t != NULL; t = t->conflict)
    {
        if (0 == strcmp(name, t->name) && t->kind.objkind == FUNCTION && t->kind.function.isdefined == 0)
        {
            return true;
        }
    }
    return false;
}
bool isPresence(char *name) //若已在散列表中出现，返回true;
{
    if (name == NULL)
    {
        return false;
    }
    int index = hash_pjw(name) % TABLE_SIZE;
    SymbolNode *t = hashtable[index];
    for (; t != NULL; t = t->conflict)
    {
            if (0 == strcmp(name, t->name))
            {
                return true;
            }
    }
    return false;
}
int addStructNode(Vartype *struct_type)
{
    //TODO:add Struct Node

    strcpy(struct_table[struct_table_index].struct_id, struct_type->struct_id);
    struct_table[struct_table_index].structure = struct_type->structure;
    struct_table[struct_table_index].memory_size = struct_type->memory_size;
    struct_table_index += 1;

    return 1;
}
int findStructNode(char *name, Vartype *type)
{
    //TODO: find a Struct Node
    for (int i = 0; i < struct_table_index; i++)
    {
        if (strcmp(struct_table[i].struct_id, name) == 0)
        {
            type->structure = struct_table[i].structure;
            type->specifier = STRUCTVAR;
            type->memory_size = struct_table[i].memory_size;
            return 1;
        }
    }
    //type = NULL;
    return -1;
}
void printStructTab()
{
    FieldList *t = NULL;
    for (int i = 0; i < struct_table_index; i++)
    {
        printf("struct %s:\n", struct_table[i].struct_id);
        t = struct_table[i].structure;
        printf("Num\tValue\tVarKind\tVarType\tName\tBind\tDeep\n");

        for (; t != NULL; t = t->tail)
        {
            printf("%d\t%d\t", 0, 0);
            //LOG("%d", t->kind.objkind);
            switch (t->kind.objkind)
            {
            case VARIBLE:
                printf("VARIBLE\t"); //打印目标类型
                break;
            case FUNCTION:
                printf("FUNC\t");
                break;
            case ARRAY:
                printf("ARRAY\t");
                break;
            default:
                printf("\t");
                break;
            }
            switch (t->type.specifier)
            { //打印变量或函数类型，如int ,float,struct;
            case INTVAR:
                printf("int\t");
                break;
            case FLOATVAR:
                printf("float\t");
                break;
            case STRUCTVAR:
                printf("struct\t");
                break;
            default:
                printf("\t");
                break;
            }
            printf("%s\t", t->name);
            printf("%s\t", struct_table[i].struct_id);
            /*switch (t->bind)
            {
            case GLOBAL:
                printf("Global\t");
                break;
            case LOCAL:
                printf("Local\t");
                break;
            default:
                printf("\t");
                break;
            }*/
            //printf("%d", t->action_scope_deep);
            printf("\n");
        }
    }
}
void printSymTab()
{
    printf("符号表:\n");
    printf("Num\tValue\tVarKind\tVarType\tName\tBind\tDeep\n");
    SymbolNode *ptr = hashtable[0];
    SymbolNode *t = ptr;
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        t = hashtable[i];
        for (; t != NULL; t = t->conflict)
        { //打印冲突链表中的节点
            if (t->kind.objkind != EMPTY)
            {
                printf("%d\t%d\t", t->num, 0);
                //LOG("%d", t->kind.objkind);
                switch (t->kind.objkind)
                {
                case VARIBLE:
                    printf("VARIBLE\t"); //打印目标类型
                    break;
                case FUNCTION:
                    printf("FUNC\t");
                    break;
                case ARRAY:
                    printf("ARRAY\t");
                    break;
                default:
                    printf("\t");
                    break;
                }
                switch (t->type.specifier)
                { //打印变量或structure函数类型，如int ,float,struct;
                case INTVAR:
                    printf("int\t");
                    break;
                case FLOATVAR:
                    printf("float\t");
                    break;
                case STRUCTVAR:
                    printf("struct\t");
                    break;
                default:
                    printf("\t");
                    break;
                }
                printf("%s\t", t->name);
                switch (t->bind)
                {
                case GLOBAL:
                    printf("Global\t");
                    break;
                case LOCAL:
                    printf("Local\t");
                    break;
                default:
                    printf("\t");
                    break;
                }
                printf("%d", t->action_scope_deep);
                printf("\n");
            }
        }
    }
}
bool isVaribleArg(char* varname){
    assert(varname != NULL);
    SymbolNode* s = findSymbol(varname);
    return s->varible_is_arg;
}