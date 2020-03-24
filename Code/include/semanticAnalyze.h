#ifndef __SEMANTIC_ANALYZE_H__
#define __SEMANTIC_ANALYZE_H__


#include "buildtree.h"
#include "symbolTableManage.h"

typedef enum ErrorType
{
    USING_VARIBLE_BUT_NOT_DEFINED = 0,
    USING_FUNC_BUT_NOT_DEFINED = 1,
    VARBILE_REDECLARATION = 2,
    FUNC_REDECLARATION = 3,
    MISMATCHED_FOR_ASSIGNMENT = 4,
    LEFT_HAND_MUST_BE_VARIBLE = 5,
    MISMATCHED_FOR_OPERANDS = 6,
    MISMATCHED_FOR_RETURN = 7,
    FUNCTION_NOT_APPLICABLE = 8,
    NOT_AN_ARRAY = 9,
    NOT_AN_FUNCTION = 10,
    NOT_AN_INT_FOR_ARRAY_INDEX = 11,
    ILLEGAL_USE_OF_DOT = 12,
    NON_EXISTENT_FIELD = 13,
    REDEFINED_FIELD = 14,
    DUPLICATED_NAME = 15,
    UNDEFINED_STRUCTURE = 16
}
ErrorType;

TreeNode *semantic_tree_root;
int analyzeCheck();
int analyzeTree(TreeNode *node);
int dealExtDef(TreeNode *node);
VARSpecifier getSpecifier(TreeNode *node);
int dealVartype(TreeNode *node, Vartype *type);
int dealCompSt(TreeNode *node, int deep);

int dealDecList(TreeNode *node, Vartype *type, int deep);
int dealExp(TreeNode *node,Vartype* type,Varkind* kind);
int dealVarDec(TreeNode *node, Vartype *type,int deep);
int dealVarDecAssign(TreeNode *node, Vartype *type,Varkind *kind, int deep);
int dealArray(TreeNode *node, Varkind *kind, char *arrayname);
int dealFunction(TreeNode *node, Varkind *kind,char* funct_name);
int addStructDomain(TreeNode *node, Vartype *type);
bool findStructDomain(char *name, FieldList *field);
bool getStructDomain(char* fieldname, FieldList* field, Vartype* type, Varkind* kind);
int dealStructVarDec(TreeNode* node, Vartype* domain_type, Vartype* global_type);
int dealFunctionVarDec(TreeNode *node, Vartype *param_type, Varkind *kind);
bool isExpLeftValue(TreeNode *node);
int dealStmt(TreeNode *node, int deep);
bool isParamArguEqual(FieldList *param, TreeNode *args);
int getStructDomainTypeKind(char *domain_name, Vartype *struct_type, Vartype *type, Varkind *kind);
void printError(ErrorType type, int line, char *link);
void printError1(ErrorType type, int line);
void printError2(ErrorType type, int line, char *comment, char *link);
void printError3(ErrorType type, int line, char *link);
void printArgs(TreeNode *args);
void printExp(TreeNode *exp);

/**
 * debug部分的函数 

*/
void printFunctionParam(char *func_name, Varkind *kind,Vartype* type);
#endif