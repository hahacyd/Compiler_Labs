#ifndef __GENERATEIR_H__
#define __GENERATEIR_H__
#include"symbolTableManage.h"
#include"buildtree.h"
#include"maintainIR.h"
typedef enum RELOP_TYPE {
    REVERSE
}RELOP_TYPE;
InterCodes* translate_Exp(TreeNode* exp, OperandOperator operator, Operand* op);
InterCodes* translate_FunDec(TreeNode* fundec);
InterCodes* translate_Program(TreeNode* program);
InterCodes* translate_ExtDefList(TreeNode* extdeflist);
InterCodes* translate_ExtDef(TreeNode* extdef); //只处理 Specifier FunDec CompSt的类型;
InterCodes* translate_CompSt(TreeNode* compst);
InterCodes* translate_VarDec(TreeNode* vardec);
InterCodes* translate_Def(TreeNode* def);
InterCodes* translate_Dec(TreeNode* dec);
InterCodes* translate_Stmt(TreeNode* stmt, Label* if_end);
InterCodes* translate_Cond(TreeNode* exp, Label* label_true, Label* label_false, RELOP_TYPE type);
InterCodes* translate_IF(TreeNode* ifstmt, Label* if_end);
InterCodes* translate_While(TreeNode* whilestmt, Label* if_end);
InterCodes* translate_Args(TreeNode* args);
int initialize_Array(TreeNode* array);
void getArrayName(TreeNode* array, char* arrayname);
Operand* getArrayAddressOffset(TreeNode* exp,Operand* array_address);
int getArrayNameFromExp(TreeNode* exp, char* arrayname);
int getRangeOfArray(ArrayList* arraylist, int no,int dimension);
void testrun();
void startGenerateIr(TreeNode* tree);
COMPARISON reverse_comp(COMPARISON relop);
InterCodes* translate_IF_ELSE(TreeNode* ifelsestmt, Label* if_end);
int calculateDomainAddress(Vartype* type, char* domain_name);
int getStructVaribleName(TreeNode* exp,char* name);
int getStructFieldOffset(char* variblename, char* fieldname);
int getKindTypeOfArray(TreeNode* exp,Operand* address, Vartype* type, Varkind* kind);
#endif
